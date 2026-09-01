#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <cstdint>

namespace indicators
{
    /// Identifies one of the four sequential boot verification stages.
    ///
    /// Each stage is associated with a pair of front-panel button LEDs.  The
    /// SPI LED bit indices below are zero-based and derived from the physical
    /// panel-label → firmware-button-index mapping documented in
    /// docs/wiring-reference.md.
    enum class BootStage : uint8_t
    {
        Vcc3v3Relay  = 0,  ///< Panel LEDs  7 & 10  →  SPI bits  1, 4
        DacRelay     = 1,  ///< Panel LEDs  3 &  6  →  SPI bits 12, 11
        OutputStage  = 2,  ///< Panel LEDs  2 &  5  →  SPI bits 15, 6
        RpiComms     = 3,  ///< Panel LEDs  1 &  4  →  SPI bits  7, 10
    };

    /// Structured boot diagnostic LED display.
    ///
    /// At the start of a power-on sequence call begin() to illuminate eight
    /// front-panel LEDs, indicating that the system is booting and no stage
    /// has yet been verified.  As each subsystem initialises successfully call
    /// stageSuccess() to extinguish its pair of LEDs.  If a subsystem fails
    /// call stageFailure() — this turns off any remaining unprocessed
    /// diagnostic LEDs and begins flashing only the failing stage's pair at
    /// 3 Hz until the system is reset.
    ///
    /// If the firmware fails before reaching the relay power-on sequence call
    /// firmwareInitFailed() to flash all eight diagnostic LEDs at 3 Hz.
    ///
    /// begin() may be called again after a failure to reset the display for a
    /// retry attempt; it stops any running flash task first.
    class BootDiagnosticLeds
    {
    public:
        /// Illuminate all eight boot diagnostic LEDs.
        /// Stops any running failure-flash task before setting the LEDs.
        /// Call at the start of each power-on sequence.
        void begin();

        /// Record a successful boot stage: extinguish its LED pair.
        void stageSuccess(BootStage stage);

        /// Record a failed boot stage.
        /// Turns off all remaining diagnostic LEDs, then flashes only the
        /// failing stage's LED pair at 3 Hz continuously until reset.
        /// Has no effect if a failure is already being indicated.
        void stageFailure(BootStage stage);

        /// Firmware initialisation failed before the relay power-on sequence.
        /// Flashes all eight diagnostic LEDs at 3 Hz continuously until reset.
        /// Has no effect if a failure is already being indicated.
        void firmwareInitFailed();

    private:
        // ~3 Hz flash: half-period = 1 / (2 × 3) s ≈ 167 ms.
        static constexpr uint32_t k_flashHalfPeriodMs = 167u;
        static constexpr const char *k_logTag = "BootDiagLeds";

        // SPI LED bit indices (0-based, matching ledIndex in SpiLedDriver)
        // for each boot stage.  Row index == BootStage cast to uint8_t.
        //
        // Panel label → firmware button index → SPI LED bit (ledIndex = buttonId):
        //   Panel  7 (Previous Track)  → firmware  1  → SPI bit  1
        //   Panel 10 (Skip Backwards)  → firmware  4  → SPI bit  4
        //   Panel  3 (Toggle Meter)    → firmware 12  → SPI bit 12
        //   Panel  6 (Next Panel)      → firmware 11  → SPI bit 11
        //   Panel  2 (Brightness)      → firmware 15  → SPI bit 15
        //   Panel  5 (Toggle Display)  → firmware  6  → SPI bit  6
        //   Panel  1 (Cover View)      → firmware  7  → SPI bit  7
        //   Panel  4 (Toggle DAC)      → firmware 10  → SPI bit 10
        static constexpr uint8_t k_stageBits[4][2] = {
            {  1,  4 },   // Vcc3v3Relay : panel  7 → bit 1,  panel 10 → bit 4
            { 12, 11 },   // DacRelay    : panel  3 → bit 12, panel  6 → bit 11
            { 15,  6 },   // OutputStage : panel  2 → bit 15, panel  5 → bit 6
            {  7, 10 },   // RpiComms    : panel  1 → bit 7,  panel  4 → bit 10
        };

        // Combined bitmask of all eight diagnostic SPI LED bits.
        // = (1<<1)|(1<<4)|(1<<6)|(1<<7)|(1<<10)|(1<<11)|(1<<12)|(1<<15)
        static constexpr uint16_t k_allDiagnosticBits =
            static_cast<uint16_t>((1u << 1u) | (1u << 4u) | (1u << 6u)  | (1u <<  7u) |
                                   (1u << 10u) | (1u << 11u) | (1u << 12u) | (1u << 15u));

        std::atomic<bool>     m_failureActive{false};
        std::atomic<uint16_t> m_flashMask{0u};
        TaskHandle_t          m_task{nullptr};

        static uint16_t stageMaskFor(BootStage stage);
        void            startFlashTask(uint16_t mask);
        static void     flashTask(void *arg);
    };

} // namespace indicators
