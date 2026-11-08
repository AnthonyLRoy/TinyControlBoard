# Test Results - ActionFactory Routing

## Results

- The ESP32 firmware build succeeded with the project's PlatformIO executable.
- The C++ host test executable was compiled with a session-local Zig C++
  compiler using the source files and include paths from
  `host_tests/CMakeLists.txt`. All 62 tests passed, including the expanded
  ActionFactory test for every explicitly mapped command, the UART fallback,
  no-action handling, and power release time.
- In the previous test run, `behavior_test.py` passed its initial library-browser
  checks but failed while importing `uart5_listener`: its mocked `command_ids`
  module does not define `CMD_MOVE_TRACK`. It was not rerun for this C++ change.

The host test does not exercise the `IgnoreWhileNotOn` factory case: the factory
classifies commands with the power state set to `ON`.
