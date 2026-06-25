# Power Sequencing

This document explains the current power-state behavior implemented in the firmware.

Source files:

- [lib/power/PowerStateTransitionHandler.cpp](../lib/power/PowerStateTransitionHandler.cpp)
- [lib/power/PowerStateTransitionPolicy.hpp](../lib/power/PowerStateTransitionPolicy.hpp)
- [lib/power/DelayedBootRecoveryState.hpp](../lib/power/DelayedBootRecoveryState.hpp)
- [lib/power/RPIBootManager.cpp](../lib/power/RPIBootManager.cpp)
- [lib/power/RelayController.cpp](../lib/power/RelayController.cpp)
- [lib/power/powerState.hpp](../lib/power/powerState.hpp)
- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- [lib/indicators/SpiBootIndicator.cpp](../lib/indicators/SpiBootIndicator.cpp)

## 1. Current Power States

The current power state enum is:

- `OFF`
- `SHUTTING_DOWN`
- `ON`
- `TURNING_ON`
- `SLEEP`
- `GOING_TO_SLEEP`
- `DEEPSLEEP`
- `GOING_INTO_DEEP_SLEEP`

In practice, the code paths currently used most clearly are:

- `TURNING_ON`
- `SHUTTING_DOWN`
- `ON`
- `GOING_TO_SLEEP`
- `GOING_INTO_DEEP_SLEEP`
- `SLEEP`
- `DEEPSLEEP`

## 2. Timing Constants Used Today

All power-related timing constants live in the `board::timing` namespace in [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp).
The long-press threshold lives in [lib/power/PowerStateTransitionPolicy.hpp](../lib/power/PowerStateTransitionPolicy.hpp).

| Constant | Value (ms) | Meaning |
|---|---:|---|
| `board::timing::k_powerSettleDelayMs` | 1500 | delay used after enabling DAC and output-stage relays |
| `board::timing::k_screenOnDelayMs` | 1000 | delay used for screen and Pi relay steps during power-on |
| `board::timing::k_rpiBootTimeoutMs` | 60000 | max time to wait for heartbeat after power-on |
| `board::timing::k_rpiShutdownTimeoutMs` | 60000 | max time to wait for heartbeat timeout during shutdown |
| `board::timing::k_rpiShutdownSettleDelayMs` | 500 | delay between Pi relay off and screen relay off |
| `board::timing::k_screenPowerOffDelayMs` | 5000 | delay after screen relay off before LED state changes |
| `board::timing::k_heartbeatTimeoutMs` | 30000 | inactivity window after which heartbeat is considered lost |
| `kLongPressThresholdMs` | 3000 | separates sleep from deep sleep on power-button release |

## 3. How The Power Button Works

The power button uses a timed action.

Behavior:

1. button press stores the current time,
2. button release computes the hold duration,
3. release emits `CMD_SYS_POWER` plus `releaseTimeMillis`,
4. `ActionProcessor` chooses the power transition based on current power state and held duration.

Reference:

- [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp)

## 4. Power-On Sequence

If the current state is `OFF`, `SLEEP`, or `DEEPSLEEP`, a power-button release enters the power-on path.

Current sequence:

1. power LED state is set to `TURNING_ON`,
2. screen relay is enabled with a 1000 ms delay,
3. DAC relay is enabled with a 1500 ms delay,
4. output stage relay is enabled with a 1500 ms delay,
5. Raspberry Pi relay is enabled with a 1000 ms delay,
6. `SpiBootIndicator::startWaiting()` is called — all SPI LEDs begin flashing slowly (~1 Hz) to indicate the firmware is waiting for the Pi,
7. firmware waits up to 60 seconds for Pi heartbeat,
8. if heartbeat received: `SpiBootIndicator::notifySuccess()` is called, flashing stops and all LEDs clear,
9. if timeout: `SpiBootIndicator::notifyFailure()` is called, LEDs switch to fast flashing (~3.3 Hz),
10. on success: power LED moves to `ON`, activity status becomes `Active`,
11. on timeout: power LED moves to `SLEEP`, activity status becomes `sleeping`, and the fast-flashing SPI LEDs remain as the ongoing degraded-mode indicator while the Pi is still allowed to finish booting,
12. if a delayed heartbeat arrives after the timeout, the firmware clears the failure indicator, promotes the power state to `ON`, and resumes normal active operation without re-running board initialization.

## 5. Sleep Sequence

If the current state is `ON` and the power button hold time is less than 3000 ms, the firmware enters the sleep path.

Current sequence:

1. power LED state becomes `SHUTTING_DOWN`,
2. `CMD_SYS_RPI_SHUTDOWN` is sent over UART,
3. firmware waits up to 60 seconds for shutdown confirmation via heartbeat timeout,
4. power LED state becomes `GOING_TO_SLEEP`,
5. Raspberry Pi power relay is turned off,
6. firmware delays 500 ms,
7. screen power relay is turned off,
8. firmware delays 5000 ms,
9. SPI LEDs are cleared and the button-status LED is set back to `Idle`,
10. power LED state becomes `SLEEP`,
11. activity status becomes `sleeping`.

Practical result:

- the Raspberry Pi and screen are shut down,
- DAC and output stage power remain on,
- the board stays in a lower-power standby style state rather than a full deep power-down.

## 6. Deep-Sleep Sequence

If the current state is `ON` and the power button hold time is 3000 ms or more, the firmware enters the deep-sleep path.

Current sequence:

1. power LED state becomes `SHUTTING_DOWN`,
2. `CMD_SYS_RPI_SHUTDOWN` is sent over UART,
3. firmware waits up to 60 seconds for shutdown confirmation via heartbeat timeout,
4. power LED state becomes `GOING_INTO_DEEP_SLEEP`,
5. Raspberry Pi power relay is turned off,
6. firmware delays 500 ms,
7. screen power relay is turned off,
8. DAC relay is turned off,
9. output stage relay is turned off,
10. SPI LEDs are cleared and the button-status LED is set back to `Idle`,
11. power LED state becomes `DEEPSLEEP`,
12. activity status becomes `sleeping`.

Practical result:

- Raspberry Pi power is removed,
- screen power is removed,
- DAC power is removed,
- output stage power is removed,
- board logic still remains present enough to respond later, so this is a project-specific deep sleep state, not necessarily ESP-IDF chip deep sleep.

## 7. Heartbeat Role In Power Sequencing

Heartbeat is the synchronization signal between the ESP32 and the Raspberry Pi.

Current behavior:

- heartbeat received sets the event-group bit used for boot completion,
- heartbeat timeout sets the event-group bit used for shutdown confirmation,
- power-on waits for heartbeat reception,
- sleep and deep-sleep wait for heartbeat timeout.

This means the system uses absence of heartbeat as a shutdown/offline signal.

## 8. Relay Ownership In Power Paths

Current relay assignments from [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp):

| Relay Purpose | ESP32 GPIO |
|---|---:|
| Screen power | 13 |
| DAC power | 12 |
| Raspberry Pi power | 11 |
| Output stage power | 9 |
| Prototype DAC enable | 10 |
| General 1 | 47 |
| General 2 | 39 |

`StandardRelay::setRelayState()` (in [lib/hal/relay/relay.cpp](../lib/hal/relay/relay.cpp)) returns `true` on success and `false` if the ESP-IDF GPIO driver reports an error. A failure indicates a firmware misconfiguration (wrong pin, unconfigured GPIO) rather than a physically faulted relay — physical contact state is not detectable without dedicated feedback hardware.

Current dedicated relay helper methods:

- `shutdownRpi()` turns off the Raspberry Pi relay,
- `shutdownScreen()` turns off the screen relay,
- `handleToggleDac()` toggles the DAC relay.

## 9. Current Behavior Caveats

- Shutdown waits are advisory today: `waitForRpiShutdown()` is called, but the result is not checked before power is removed.
- The state machine uses `SHUTTING_DOWN` before both sleep and deep-sleep, then diverges into `GOING_TO_SLEEP` or `GOING_INTO_DEEP_SLEEP` after the Pi shutdown wait.
- `waitForRpiShutdown()` is satisfied by heartbeat timeout, which is a practical signal but not a strong explicit shutdown acknowledgment packet.
- On boot timeout, the power LED transitions to `SLEEP`. The fast-flashing SPI LEDs from `SpiBootIndicator::notifyFailure()` remain as the persistent degraded-mode indicator until a later heartbeat arrives.

## 10. Debug Flag: Simulating Pi Boot

To test the full firmware on the bench without a connected Raspberry Pi, use the debug flag in [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp):

```cpp
namespace board::debug {
    inline constexpr bool k_simulateRpiBoot = true;
}
```

When `true`:

- `RpiBootManager::waitForRpiToBoot()` returns `true` immediately without waiting for a heartbeat,
- `SpiBootIndicator::notifySuccess()` is called and the boot flash stops normally,
- a warning is logged: `Debug mode enabled: skipping RPi heartbeat wait (k_simulateRpiBoot=true)`.

Current default behavior:

- debug builds default `k_simulateRpiBoot` to `true`,
- release builds default `k_simulateRpiBoot` to `false`,
- defining `SIMULATE_RPI_BOOT=1` or `SIMULATE_RPI_BOOT=0` overrides either default at compile time,
- the branch is still compiled as an `if constexpr`, so the selected behavior is fixed at build time.

## 11. Recommended Future Documentation Additions

Useful follow-ups for this file later:

1. add a timing diagram for ON -> SLEEP and ON -> DEEPSLEEP,
2. document expected relay electrical loads,
3. document which subsystems remain powered in each state,
4. document recovery behavior if heartbeat never appears during boot.

## 12. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/firmware-technical-design.md](./firmware-technical-design.md)
- [docs/wiring-reference.md](./wiring-reference.md)