#pragma once
#include <cstddef>
#include <cstdint>

namespace support
{
    /**
     * Lightweight wrapper around the ESP-IDF NVS (non-volatile storage) API.
     * Each instance is bound to one NVS namespace; read/write methods operate
     * on named keys within that namespace.  Writes commit immediately.
     *
     * Usage:
     *   support::NvsStorage store{"my_component"};
     *   store.writeInt32("counter", 42);
     *   int32_t v{};
     *   if (store.readInt32("counter", v)) { ... }
     *
     * All read methods return false if the key does not exist (first boot) or
     * if the NVS partition is unavailable.  Callers should write a default
     * value in that case.
     */
    class NvsStorage
    {
    public:
        // Creates a storage wrapper bound to a single NVS namespace name.
        explicit NvsStorage(const char* namespaceName);

        // Reads an 8-bit signed integer from the NVS namespace.
        bool readInt8  (const char* key, int8_t&   out) const;
        // Writes an 8-bit signed integer to the NVS namespace.
        bool writeInt8 (const char* key, int8_t    value);

        // Reads an 8-bit unsigned integer from the NVS namespace.
        bool readUInt8 (const char* key, uint8_t&  out) const;
        // Writes an 8-bit unsigned integer to the NVS namespace.
        bool writeUInt8(const char* key, uint8_t   value);

        // Reads a 16-bit signed integer from the NVS namespace.
        bool readInt16 (const char* key, int16_t&  out) const;
        // Writes a 16-bit signed integer to the NVS namespace.
        bool writeInt16(const char* key, int16_t   value);

        // Reads a 32-bit signed integer from the NVS namespace.
        bool readInt32 (const char* key, int32_t&  out) const;
        // Writes a 32-bit signed integer to the NVS namespace.
        bool writeInt32(const char* key, int32_t   value);

        // Reads a 32-bit unsigned integer from the NVS namespace.
        bool readUInt32 (const char* key, uint32_t& out) const;
        // Writes a 32-bit unsigned integer to the NVS namespace.
        bool writeUInt32(const char* key, uint32_t  value);

        /**
         * Read a null-terminated string into caller-provided buffer.
         * @param buf   Destination buffer.
         * @param len   Buffer capacity (including null terminator).
         * @return false if key not found or buffer is too small.
         */
        bool readString (const char* key, char* buf, size_t len) const;
        // Writes a null-terminated string value to the NVS namespace.
        bool writeString(const char* key, const char* value);

    private:
        const char* m_namespace;
    };

} // namespace support
