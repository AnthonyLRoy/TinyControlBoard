# ActionFactory Refactoring: Template Specialization

## Overview
Refactored `ActionFactory.cpp` to use template specializations for action creation, with a switch statement for runtime dispatch.

## Changes Made

### ActionFactory.hpp
- Added template declaration: `template<ActionCommandRoute Route> std::unique_ptr<actions::IAction> createActionForRoute(...)`
- Kept the main `createAction()` function signature unchanged

### ActionFactory.cpp
- Added 7 template specializations:
  - `PowerStateTransition` → `PowerTransitionAction`
  - `System` → `SystemAction`
  - `Relay` → `RelayAction`
  - `Brightness` → `BrightnessAction`
  - `UartDispatch` → `UartDispatchAction`
  - `None` → `nullptr`
  - `IgnoreWhileNotOn` → `UartDispatchAction` (fallback)

- Main `createAction()` now dispatches to appropriate template specialization

## Benefits

Each route has a separate specialization. Adding a route also requires updating the dispatcher switch.

The compiler checks the syntax and types of the specializations used by the dispatcher. The switch's default case means it does not enforce exhaustive handling of new routes.

✅ **Clear Separation** - Each action type's creation logic is isolated in its own specialization

The dispatcher still selects a route at runtime; performance equivalence has not been measured.

Future actions require both a specialization and a dispatcher case.

## Example: Adding a New Action Type

To add support for a new `CustomAction`:

1. Add to `ActionCommandRoute.hpp` enum:
   ```cpp
   enum class ActionCommandRoute { 
       // ... existing ...
       Custom,
   };
   ```

2. Add new specialization in `ActionFactory.cpp`:
   ```cpp
   template<>
   std::unique_ptr<actions::IAction> 
   createActionForRoute<ActionCommandRoute::Custom>(CommandId command, uint16_t)
   {
       return std::make_unique<actions::CustomAction>(command);
   }
   ```

3. Add mapping in `ActionCommandRoutingPolicy.hpp`:
   ```cpp
   { CMD_CUSTOM, ActionCommandRoute::Custom }
   ```

4. Add dispatch case in `createAction()`:
   ```cpp
   case ActionCommandRoute::Custom:
       return createActionForRoute<ActionCommandRoute::Custom>(command, releaseTimeMs);
   ```

## Validation
The host ActionFactory test passed for the existing action types, no-action
command, and power transition release time. The template declaration also adds
a new public API to the header; the existing `createAction()` signature is unchanged.
