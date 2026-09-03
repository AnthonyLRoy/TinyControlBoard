#include "ble/BleServer.hpp"
#include "app/actionProcessor.hpp"
#include "app/SystemState.hpp"
#include "app/ActionFactory.hpp"
#include "board/boardConfig.hpp"
#include "protocol/uartProtocol.hpp"

#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstring>
#include <cstdint>
#include <functional>

// ---------------------------------------------------------------------------
// UUIDs  (128-bit, little-endian byte order for BLE_UUID128_INIT)
//   Service:           4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d0e
//   CMD char:          4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d01
//   Status char:       4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d02
//   Now-playing char:  4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d03
//   Track-progress char:4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d04
//   Library char:      4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d05
//   Library-cmd char:  4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d06
//   Playlist-cmd char: 4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d07
// ---------------------------------------------------------------------------
static const ble_uuid128_t k_svcUuid =
    BLE_UUID128_INIT(0x0e, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_cmdChrUuid =
    BLE_UUID128_INIT(0x01, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_statusChrUuid =
    BLE_UUID128_INIT(0x02, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_nowPlayingChrUuid =
    BLE_UUID128_INIT(0x03, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_trackProgressChrUuid =
    BLE_UUID128_INIT(0x04, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_libraryChrUuid =
    BLE_UUID128_INIT(0x05, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_libraryCmdChrUuid =
    BLE_UUID128_INIT(0x06, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static const ble_uuid128_t k_playlistCmdChrUuid =
    BLE_UUID128_INIT(0x07, 0x9d, 0x8c, 0x7b, 0x6a, 0x5f, 0x3e, 0x1d,
                     0x2c, 0x4b, 0x9a, 0x8f, 0x7d, 0x6e, 0x5c, 0x4a);

static constexpr const char *k_logTag = "BLE_Server";

// ---------------------------------------------------------------------------
// File-scope state (avoids NimBLE types in the class header)
// ---------------------------------------------------------------------------
static controlSystem::ActionProcessor *s_processor       = nullptr;
static controlSystem::SystemState     *s_state            = nullptr;
static uint16_t                        s_connHandle       = BLE_HS_CONN_HANDLE_NONE;
static uint16_t                        s_statusValHandle  = 0;
static uint16_t                        s_nowPlayingValHandle = 0;
static uint16_t                        s_trackProgressValHandle = 0;
static uint16_t                        s_libraryValHandle = 0;
static volatile bool                   s_statusSubscribed = false;
static volatile bool                   s_nowPlayingSubscribed = false;
static volatile bool                   s_trackProgressSubscribed = false;
static volatile bool                   s_librarySubscribed = false;
static std::function<void(uint16_t, uint16_t)> s_onLibraryCommand;
static std::function<void(uint16_t, const uint8_t *, uint8_t)> s_onPlaylistCommand;

// ---------------------------------------------------------------------------
// Forward declarations for the GATT table
// ---------------------------------------------------------------------------
static int cmdChrAccess(uint16_t conn, uint16_t attr,
                        struct ble_gatt_access_ctxt *ctxt, void *arg);
static int statusChrAccess(uint16_t conn, uint16_t attr,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);
static int nowPlayingChrAccess(uint16_t conn, uint16_t attr,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);
static int trackProgressChrAccess(uint16_t conn, uint16_t attr,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg);
static int libraryChrAccess(uint16_t conn, uint16_t attr,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int libraryCmdChrAccess(uint16_t conn, uint16_t attr,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);
static int playlistCmdChrAccess(uint16_t conn, uint16_t attr,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

// ---------------------------------------------------------------------------
// GATT service table
// ---------------------------------------------------------------------------
static const struct ble_gatt_svc_def k_gattSvcs[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &k_svcUuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid      = &k_cmdChrUuid.u,
                .access_cb = cmdChrAccess,
                .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid       = &k_statusChrUuid.u,
                .access_cb  = statusChrAccess,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_statusValHandle,
            },
            {
                .uuid       = &k_nowPlayingChrUuid.u,
                .access_cb  = nowPlayingChrAccess,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_nowPlayingValHandle,
            },
            {
                .uuid       = &k_trackProgressChrUuid.u,
                .access_cb  = trackProgressChrAccess,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_trackProgressValHandle,
            },
            {
                .uuid       = &k_libraryChrUuid.u,
                .access_cb  = libraryChrAccess,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_libraryValHandle,
            },
            {
                .uuid      = &k_libraryCmdChrUuid.u,
                .access_cb = libraryCmdChrAccess,
                .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid      = &k_playlistCmdChrUuid.u,
                .access_cb = playlistCmdChrAccess,
                .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            { 0 },
        },
    },
    { 0 },
};


// ---------------------------------------------------------------------------
// Characteristic access callbacks
// ---------------------------------------------------------------------------
static int cmdChrAccess(uint16_t conn, uint16_t attr,
                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return 0;
    if (OS_MBUF_PKTLEN(ctxt->om) < 2)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    uint16_t cmdId = 0;
    os_mbuf_copydata(ctxt->om, 0, sizeof(cmdId), &cmdId);
    ESP_LOGI(k_logTag, "BLE command received: 0x%04X", cmdId);

    if (s_processor)
        s_processor->injectCommand(static_cast<CommandId>(cmdId));
    return 0;
}

static int statusChrAccess(uint16_t conn, uint16_t attr,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR || !s_state)
        return BLE_ATT_ERR_UNLIKELY;

    const auto     ps = static_cast<uint8_t>(s_state->powerState.load());
    const uint16_t bm = s_state->buttonLedBitmask.load();
    const uint8_t  buf[3] = { ps,
                               static_cast<uint8_t>(bm & 0xFF),
                               static_cast<uint8_t>((bm >> 8) & 0xFF) };
    os_mbuf_append(ctxt->om, buf, sizeof(buf));
    return 0;
}

static int nowPlayingChrAccess(uint16_t conn, uint16_t attr,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR || !s_state)
        return BLE_ATT_ERR_UNLIKELY;
    const char *text = s_state->nowPlayingText;
    os_mbuf_append(ctxt->om, text, strlen(text));
    return 0;
}

static int trackProgressChrAccess(uint16_t conn, uint16_t attr,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR || !s_state)
        return BLE_ATT_ERR_UNLIKELY;
    uint16_t elapsed = 0;
    uint16_t duration = 0;
    bool isPlaying = false;
    do
    {
        while (s_state->trackProgressUpdating.load(std::memory_order_acquire))
        {
        }
        elapsed = s_state->trackElapsedSeconds.load(std::memory_order_relaxed);
        duration = s_state->trackDurationSeconds.load(std::memory_order_relaxed);
        isPlaying = s_state->trackIsPlaying.load(std::memory_order_relaxed);
    } while (s_state->trackProgressUpdating.load(std::memory_order_acquire));
    const uint8_t buf[5] = { static_cast<uint8_t>(elapsed & 0xFF),
                             static_cast<uint8_t>(elapsed >> 8),
                             static_cast<uint8_t>(duration & 0xFF),
                             static_cast<uint8_t>(duration >> 8),
                             static_cast<uint8_t>(isPlaying) };
    os_mbuf_append(ctxt->om, buf, sizeof(buf));
    return 0;
}

// Read access is not meaningful for library entries (they're push-only); just report empty.
static int libraryChrAccess(uint16_t conn, uint16_t attr,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
        return BLE_ATT_ERR_UNLIKELY;
    return 0;
}

static int libraryCmdChrAccess(uint16_t conn, uint16_t attr,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return 0;
    if (OS_MBUF_PKTLEN(ctxt->om) < 4)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    uint8_t payload[4] = {};
    os_mbuf_copydata(ctxt->om, 0, sizeof(payload), payload);
    const uint16_t cmdId = static_cast<uint16_t>(payload[0]) | (static_cast<uint16_t>(payload[1]) << 8);
    const uint16_t param = static_cast<uint16_t>(payload[2]) | (static_cast<uint16_t>(payload[3]) << 8);
    ESP_LOGI(k_logTag, "BLE library command received: cmd=0x%04X param=%u", cmdId, param);

    if (s_onLibraryCommand)
        s_onLibraryCommand(cmdId, param);
    return 0;
}

// Payload: [cmdId_lo, cmdId_hi, name bytes...] — name is the remainder of the write, UTF-8, no terminator.
static int playlistCmdChrAccess(uint16_t conn, uint16_t attr,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return 0;
    const uint16_t pktLen = OS_MBUF_PKTLEN(ctxt->om);
    if (pktLen < 2)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    uint8_t cmdBytes[2] = {};
    os_mbuf_copydata(ctxt->om, 0, sizeof(cmdBytes), cmdBytes);
    const uint16_t cmdId = static_cast<uint16_t>(cmdBytes[0]) | (static_cast<uint16_t>(cmdBytes[1]) << 8);

    uint8_t nameBuf[protocol::k_maxLibraryNameLen];
    uint8_t nameLen = static_cast<uint8_t>(pktLen - 2);
    if (nameLen > protocol::k_maxLibraryNameLen)
        nameLen = protocol::k_maxLibraryNameLen;
    if (nameLen > 0)
        os_mbuf_copydata(ctxt->om, 2, nameLen, nameBuf);
    ESP_LOGI(k_logTag, "BLE playlist command received: cmd=0x%04X nameLen=%u", cmdId, nameLen);

    if (s_onPlaylistCommand)
        s_onPlaylistCommand(cmdId, nameBuf, nameLen);
    return 0;
}


// Helper: build and send a single status notification to the current connection
static bool pushStatusNotification()
{
    if (!s_state || s_connHandle == BLE_HS_CONN_HANDLE_NONE)
        return false;
    if (s_statusValHandle == 0)
    {
        ESP_LOGE(k_logTag, "pushStatus: val_handle is 0 — GATT service not registered correctly!");
        return false;
    }
    const auto     ps = static_cast<uint8_t>(s_state->powerState.load());
    const uint16_t bm = s_state->buttonLedBitmask.load();
    const uint8_t  buf[3] = { ps,
                               static_cast<uint8_t>(bm & 0xFF),
                               static_cast<uint8_t>((bm >> 8) & 0xFF) };
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (!om)
    {
        ESP_LOGW(k_logTag, "pushStatus: ble_hs_mbuf_from_flat returned NULL");
        return false;
    }
    int rc = ble_gatts_notify_custom(s_connHandle, s_statusValHandle, om);
    if (rc != 0)
    {
        ESP_LOGW(k_logTag, "ble_gatts_notify_custom failed: %d (conn=%u val=%u)",
                 rc, s_connHandle, s_statusValHandle);
        return false;
    }
    ESP_LOGI(k_logTag, "Status notification sent: powerState=%u bitmask=0x%04X", ps, bm);
    return true;
}

static bool pushNowPlayingNotification()
{
    if (!s_state || s_connHandle == BLE_HS_CONN_HANDLE_NONE || s_nowPlayingValHandle == 0)
        return false;
    const char *text = s_state->nowPlayingText;
    const size_t len = strlen(text);
    struct os_mbuf *om = ble_hs_mbuf_from_flat(text, len);
    if (!om)
        return false;
    int rc = ble_gatts_notify_custom(s_connHandle, s_nowPlayingValHandle, om);
    if (rc != 0)
        ESP_LOGW(k_logTag, "now-playing notify failed: %d", rc);
    return rc == 0;
}

static bool pushTrackProgressNotification()
{
    if (!s_state || s_connHandle == BLE_HS_CONN_HANDLE_NONE || s_trackProgressValHandle == 0)
        return false;
    uint16_t elapsed = 0;
    uint16_t duration = 0;
    bool isPlaying = false;
    do
    {
        while (s_state->trackProgressUpdating.load(std::memory_order_acquire))
        {
        }
        elapsed = s_state->trackElapsedSeconds.load(std::memory_order_relaxed);
        duration = s_state->trackDurationSeconds.load(std::memory_order_relaxed);
        isPlaying = s_state->trackIsPlaying.load(std::memory_order_relaxed);
    } while (s_state->trackProgressUpdating.load(std::memory_order_acquire));
    const uint8_t buf[5] = { static_cast<uint8_t>(elapsed & 0xFF),
                             static_cast<uint8_t>(elapsed >> 8),
                             static_cast<uint8_t>(duration & 0xFF),
                             static_cast<uint8_t>(duration >> 8),
                             static_cast<uint8_t>(isPlaying) };
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (!om)
        return false;
    int rc = ble_gatts_notify_custom(s_connHandle, s_trackProgressValHandle, om);
    if (rc != 0)
        ESP_LOGW(k_logTag, "track-progress notify failed: %d", rc);
    return rc == 0;
}

// Builds the MSG_LIBRARY_ENTRY wire payload [entryType, index_lo/hi, total_lo/hi, name]
// and pushes it immediately — called directly from the UART receive path, not the poll task.
void ble::notifyLibraryEntry(const UartMessage &rMsg)
{
    if (s_connHandle == BLE_HS_CONN_HANDLE_NONE || s_libraryValHandle == 0)
        return;

    uint8_t buf[5 + protocol::k_maxLibraryNameLen];
    buf[0] = rMsg.libraryEntryType;
    buf[1] = static_cast<uint8_t>(rMsg.libraryEntryIndex & 0xFF);
    buf[2] = static_cast<uint8_t>(rMsg.libraryEntryIndex >> 8);
    buf[3] = static_cast<uint8_t>(rMsg.libraryEntryTotal & 0xFF);
    buf[4] = static_cast<uint8_t>(rMsg.libraryEntryTotal >> 8);
    memcpy(buf + 5, rMsg.libraryEntryName, rMsg.libraryEntryNameLen);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, 5 + rMsg.libraryEntryNameLen);
    if (!om)
        return;
    int rc = ble_gatts_notify_custom(s_connHandle, s_libraryValHandle, om);
    if (rc != 0)
        ESP_LOGW(k_logTag, "library-entry notify failed: %d", rc);
}

// ---------------------------------------------------------------------------
// GAP event handler
// ---------------------------------------------------------------------------
static int gapEventHandler(struct ble_gap_event *event, void *arg);

static uint8_t s_ownAddrType = BLE_OWN_ADDR_PUBLIC;

static void startAdvertising()
{
    struct ble_hs_adv_fields adv = {};
    adv.flags                = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv.uuids128             = &k_svcUuid;
    adv.num_uuids128         = 1;
    adv.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&adv);
    if (rc != 0)
    {
        ESP_LOGE(k_logTag, "ble_gap_adv_set_fields: %d", rc);
        return;
    }

    struct ble_hs_adv_fields rsp = {};
    const char *name   = board::ble::k_deviceName;
    rsp.name           = reinterpret_cast<const uint8_t *>(name);
    rsp.name_len       = static_cast<uint8_t>(strlen(name));
    rsp.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp);
    if (rc != 0)
    {
        ESP_LOGE(k_logTag, "ble_gap_adv_rsp_set_fields: %d", rc);
        return;
    }

    struct ble_gap_adv_params params = {};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_ownAddrType, nullptr, BLE_HS_FOREVER,
                           &params, gapEventHandler, nullptr);
    if (rc != 0 && rc != BLE_HS_EALREADY)
        ESP_LOGE(k_logTag, "ble_gap_adv_start: %d", rc);
    else
        ESP_LOGI(k_logTag, "BLE advertising as \"%s\" (addr_type=%d)",
                 board::ble::k_deviceName, s_ownAddrType);
}

static int gapEventHandler(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            s_connHandle = event->connect.conn_handle;
            ESP_LOGI(k_logTag, "BLE client connected (handle=%d)", s_connHandle);
        }
        else
        {
            ESP_LOGW(k_logTag, "BLE connect failed (status=%d)", event->connect.status);
            startAdvertising();
        }
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_statusValHandle)
            s_statusSubscribed = event->subscribe.cur_notify;
        else if (event->subscribe.attr_handle == s_nowPlayingValHandle)
            s_nowPlayingSubscribed = event->subscribe.cur_notify;
        else if (event->subscribe.attr_handle == s_trackProgressValHandle)
            s_trackProgressSubscribed = event->subscribe.cur_notify;
        else if (event->subscribe.attr_handle == s_libraryValHandle)
            s_librarySubscribed = event->subscribe.cur_notify;
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(k_logTag, "BLE client disconnected (reason=%d)", event->disconnect.reason);
        s_connHandle = BLE_HS_CONN_HANDLE_NONE;
        s_statusSubscribed = false;
        s_nowPlayingSubscribed = false;
        s_trackProgressSubscribed = false;
        s_librarySubscribed = false;
        startAdvertising();
        break;

    default:
        break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// on_sync: BT controller ready
// ---------------------------------------------------------------------------
static void onSync()
{
    // Determine the correct address type (public if available, random otherwise)
    int rc = ble_hs_id_infer_auto(0, &s_ownAddrType);
    if (rc != 0)
    {
        ESP_LOGW(k_logTag, "ble_hs_id_infer_auto failed (%d); using PUBLIC", rc);
        s_ownAddrType = BLE_OWN_ADDR_PUBLIC;
    }
    startAdvertising();
}

// ---------------------------------------------------------------------------
// NimBLE host task
// ---------------------------------------------------------------------------
static void bleHostTask(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// ---------------------------------------------------------------------------
// Status notification task — polls SystemState every 200 ms
// ---------------------------------------------------------------------------
static void statusNotifyTask(void *arg)
{
    uint16_t lastBitmask       = 0xFFFF;
    uint8_t  lastPowerState    = 0xFF;
    uint8_t  lastNowPlayingVer = 0xFF;
    uint8_t  lastTrackProgressVer = 0xFF;
    bool     wasSubscribed     = false;

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(200));  // 200 ms poll — fast enough, less aggressive

        if (!s_state || !(s_statusSubscribed || s_nowPlayingSubscribed || s_trackProgressSubscribed))
            continue;

        // Freshly subscribed: wait an extra 100 ms to let the CCCD write response
        // fully clear from Android's ATT queue, then force an initial push.
        if (!wasSubscribed)
        {
            wasSubscribed = true;
            vTaskDelay(pdMS_TO_TICKS(100));
            lastBitmask       = 0xFFFF;  // force push
            lastPowerState    = 0xFF;
            lastNowPlayingVer = 0xFF;
            lastTrackProgressVer = 0xFF;
            ESP_LOGI(k_logTag, "Subscription confirmed — sending initial status");
        }

        const auto     ps  = static_cast<uint8_t>(s_state->powerState.load());
        const uint16_t bm  = s_state->buttonLedBitmask.load();
        const uint8_t  npv = s_state->nowPlayingVersion.load(std::memory_order_acquire);
        const uint8_t  tpv = s_state->trackProgressVersion.load(std::memory_order_acquire);

        if (s_statusSubscribed && (ps != lastPowerState || bm != lastBitmask))
        {
            ESP_LOGI(k_logTag, "Pushing status: powerState=%u bitmask=0x%04X", ps, bm);
            if (pushStatusNotification())
            {
                lastPowerState = ps;
                lastBitmask    = bm;
            }
        }

        if (s_nowPlayingSubscribed && npv != lastNowPlayingVer)
        {
            ESP_LOGI(k_logTag, "Pushing now-playing: %s", s_state->nowPlayingText);
            if (pushNowPlayingNotification())
                lastNowPlayingVer = npv;
        }

        if (s_trackProgressSubscribed && tpv != lastTrackProgressVer)
        {
            if (pushTrackProgressNotification())
                lastTrackProgressVer = tpv;
        }

        // If we lose the subscription between cycles, reset so the next
        // subscription gets a fresh initial push.
        if (!(s_statusSubscribed || s_nowPlayingSubscribed || s_trackProgressSubscribed))
            wasSubscribed = false;
    }
}

// ---------------------------------------------------------------------------
// BleServer::start
// ---------------------------------------------------------------------------
void ble::BleServer::start(controlSystem::ActionProcessor &processor,
                           controlSystem::SystemState     &state,
                           std::function<void(uint16_t cmdId, uint16_t param)> onLibraryCommand,
                           std::function<void(uint16_t cmdId, const uint8_t *p_name, uint8_t nameLen)> onPlaylistCommand)
{
    s_processor = &processor;
    s_state     = &state;
    s_onLibraryCommand = std::move(onLibraryCommand);
    s_onPlaylistCommand = std::move(onPlaylistCommand);

    int rc = nimble_port_init();
    if (rc != ESP_OK)
    {
        ESP_LOGE(k_logTag, "nimble_port_init failed: %d", rc);
        return;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(k_gattSvcs);
    if (rc != 0) { ESP_LOGE(k_logTag, "ble_gatts_count_cfg: %d", rc); return; }

    rc = ble_gatts_add_svcs(k_gattSvcs);
    if (rc != 0) { ESP_LOGE(k_logTag, "ble_gatts_add_svcs: %d", rc); return; }

    ESP_LOGI(k_logTag, "GATT services registered — status val_handle=%u", s_statusValHandle);

    ble_hs_cfg.sync_cb = onSync;
    ble_svc_gap_device_name_set(board::ble::k_deviceName);

    nimble_port_freertos_init(bleHostTask);
    xTaskCreate(statusNotifyTask, "ble_status", 4096, nullptr, 3, nullptr);

    ESP_LOGI(k_logTag, "BLE server initialised");
}


// ---------------------------------------------------------------------------
// UUIDs  (128-bit, little-endian byte order for BLE_UUID128_INIT)
