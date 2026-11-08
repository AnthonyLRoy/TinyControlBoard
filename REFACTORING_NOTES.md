# ActionFactory Routing

`createAction()` classifies a command with the power state set to `ON` and
constructs the corresponding action in a switch. The action processor checks
whether an action is allowed in the current power state before executing it.

- `CMD_NO_ACTION` returns `nullptr`.
- Power commands create a `PowerTransitionAction`, preserving the release time.
- System, relay, and brightness commands create their respective action types.
- Commands without an explicit route create a `UartDispatchAction`. The
  existing fallback for `IgnoreWhileNotOn` and unknown routes is also UART.

To add a command to an existing category, add its mapping to
`lib/app/ActionCommandRoutingPolicy.hpp` and extend the factory host test. A
new action category also requires a route enum value and a switch case in
`lib/app/ActionFactory.cpp`.

The host test in `host_tests/test_logic.cpp` covers the explicitly mapped
commands, an unmapped UART fallback, no-action handling, and power release time.
