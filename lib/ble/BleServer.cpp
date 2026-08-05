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

// ---------------------------------------------------------------------------
// UUIDs  (128-bit, little-endian byte order for BLE_UUID128_INIT)
//   Service:          4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d0e
//   CMD char:         4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d01
//   Status char:      4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d02
//   Now-playing char: 4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d03
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

static constexpr const char *k_logTag = "BLE_Server";

// ---------------------------------------------------------------------------
// File-scope state (avoids NimBLE types in the class header)
// ---------------------------------------------------------------------------
static controlSystem::ActionProcessor *s_processor       = nullptr;
static controlSystem::SystemState     *s_state            = nullptr;
static uint16_t                        s_connHandle       = BLE_HS_CONN_HANDLE_NONE;
static uint16_t                        s_statusValHandle  = 0;
static uint16_t                        s_nowPlayingValHandle = 0;
static volatile bool                   s_subscribed       = false; // set true only after CCCD write confirmed

// ---------------------------------------------------------------------------
// Forward declarations for the GATT table
// ---------------------------------------------------------------------------
static int cmdChrAccess(uint16_t conn, uint16_t attr,
                        struct ble_gatt_access_ctxt *ctxt, void *arg);
static int statusChrAccess(uint16_t conn, uint16_t attr,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);
static int nowPlayingChrAccess(uint16_t conn, uint16_t attr,
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
        if (event->subscribe.cur_notify)
        {
            // Mark subscribed so the notify task will push on its next cycle.
            // Do NOT push here — the CCCD write response may still be in flight,
            // and some Android stacks drop notifications received while a write
            // response is pending on the same connection.
            ESP_LOGI(k_logTag, "BLE notifications subscribed (attr=%u) — task will push shortly",
                     event->subscribe.attr_handle);
            s_subscribed = true;
        }
        else
        {
            s_subscribed = false;
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(k_logTag, "BLE client disconnected (reason=%d)", event->disconnect.reason);
        s_connHandle = BLE_HS_CONN_HANDLE_NONE;
        s_subscribed = false;
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
// Status notification task â€” polls SystemState every 500 ms
// ---------------------------------------------------------------------------
static void statusNotifyTask(void *arg)
{
    uint16_t lastBitmask       = 0xFFFF;
    uint8_t  lastPowerState    = 0xFF;
    uint8_t  lastNowPlayingVer = 0xFF;
    bool     wasSubscribed     = false;

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(200));  // 200 ms poll — fast enough, less aggressive

        if (!s_state || !s_subscribed)
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
            ESP_LOGI(k_logTag, "Subscription confirmed — sending initial status");
        }

        const auto     ps  = static_cast<uint8_t>(s_state->powerState.load());
        const uint16_t bm  = s_state->buttonLedBitmask.load();
        const uint8_t  npv = s_state->nowPlayingVersion.load(std::memory_order_acquire);

        if (ps != lastPowerState || bm != lastBitmask)
        {
            ESP_LOGI(k_logTag, "Pushing status: powerState=%u bitmask=0x%04X", ps, bm);
            if (pushStatusNotification())
            {
                lastPowerState = ps;
                lastBitmask    = bm;
            }
        }

        if (npv != lastNowPlayingVer)
        {
            ESP_LOGI(k_logTag, "Pushing now-playing: %s", s_state->nowPlayingText);
            if (pushNowPlayingNotification())
                lastNowPlayingVer = npv;
        }

        // If we lose the subscription between cycles, reset so the next
        // subscription gets a fresh initial push.
        if (!s_subscribed)
            wasSubscribed = false;
    }
}

// ---------------------------------------------------------------------------
// BleServer::start
// ---------------------------------------------------------------------------
void ble::BleServer::start(controlSystem::ActionProcessor &processor,
                           controlSystem::SystemState     &state)
{
    s_processor = &processor;
    s_state     = &state;

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
