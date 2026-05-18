#include "nvsStorage.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static constexpr const char *k_logTag = "NvsStorage      ";

namespace support
{
    NvsStorage::NvsStorage(const char* namespaceName)
        : m_namespace(namespaceName)
    {
    }

    // -------------------------------------------------------------------------
    // int8_t
    // -------------------------------------------------------------------------
    bool NvsStorage::readInt8(const char* key, int8_t& out) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_i8(h, key, &out) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readInt8 '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeInt8(const char* key, int8_t value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_i8(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeInt8 '%s'=%d %s", m_namespace, key, (int)value, ok ? "ok" : "fail");
        return ok;
    }

    // -------------------------------------------------------------------------
    // uint8_t
    // -------------------------------------------------------------------------
    bool NvsStorage::readUInt8(const char* key, uint8_t& out) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_u8(h, key, &out) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readUInt8 '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeUInt8(const char* key, uint8_t value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_u8(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeUInt8 '%s'=%u %s", m_namespace, key, (unsigned)value, ok ? "ok" : "fail");
        return ok;
    }

    // -------------------------------------------------------------------------
    // int16_t
    // -------------------------------------------------------------------------
    bool NvsStorage::readInt16(const char* key, int16_t& out) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_i16(h, key, &out) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readInt16 '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeInt16(const char* key, int16_t value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_i16(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeInt16 '%s'=%d %s", m_namespace, key, (int)value, ok ? "ok" : "fail");
        return ok;
    }

    // -------------------------------------------------------------------------
    // int32_t
    // -------------------------------------------------------------------------
    bool NvsStorage::readInt32(const char* key, int32_t& out) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_i32(h, key, &out) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readInt32 '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeInt32(const char* key, int32_t value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_i32(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeInt32 '%s'=%d %s", m_namespace, key, (int)value, ok ? "ok" : "fail");
        return ok;
    }

    // -------------------------------------------------------------------------
    // uint32_t
    // -------------------------------------------------------------------------
    bool NvsStorage::readUInt32(const char* key, uint32_t& out) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_u32(h, key, &out) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readUInt32 '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeUInt32(const char* key, uint32_t value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_u32(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeUInt32 '%s'=%u %s", m_namespace, key, (unsigned)value, ok ? "ok" : "fail");
        return ok;
    }

    // -------------------------------------------------------------------------
    // string
    // -------------------------------------------------------------------------
    bool NvsStorage::readString(const char* key, char* buf, size_t len) const
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READONLY, &h) != ESP_OK) return false;
        bool ok = (nvs_get_str(h, key, buf, &len) == ESP_OK);
        nvs_close(h);
        if (!ok) ESP_LOGD(k_logTag, "[%s] readString '%s' not found", m_namespace, key);
        return ok;
    }

    bool NvsStorage::writeString(const char* key, const char* value)
    {
        nvs_handle_t h;
        if (nvs_open(m_namespace, NVS_READWRITE, &h) != ESP_OK) return false;
        bool ok = (nvs_set_str(h, key, value) == ESP_OK);
        if (ok) nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(k_logTag, "[%s] writeString '%s' %s", m_namespace, key, ok ? "ok" : "fail");
        return ok;
    }

} // namespace support
