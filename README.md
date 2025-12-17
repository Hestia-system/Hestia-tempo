# HestiaTempo

> **HestiaTempo is not about what you time — it is about how clearly you express time.**

HestiaTempo is a small, allocation-free, non-blocking timing library for embedded systems  
(Arduino / ESP32 class microcontrollers).

It replaces direct use of `millis()` with **explicit, readable, and named time primitives**,  
without blocking execution or hiding state.

HestiaTempo is designed to scale naturally:
- from simple sketches (LED blinking, debounce)
- to structured firmware cores and SDKs

---

## Key features

- ⏱️ Non-blocking timers (no `delay()`)
- 🧠 Symbolic timer identifiers (compile-time hashed)
- 🔁 Periodic timers with drift-resistant behavior
- 🎯 One-shot timers (timeouts, watchdogs, delays)
- 🧾 Human-readable time input (`HH:MM:SS`)
- 📤 Human-readable time output (multiple formats)
- 🧩 No dynamic allocation
- 🧵 No `String`
- 🔕 No logging, no exceptions
- 📦 Suitable for sketches, libraries, cores, and SDKs

---

## Design philosophy

HestiaTempo follows a few strict principles:

- **Time is a service**, not an object you own
- **Milliseconds are the internal truth**
- **Time must be named, not inferred**
- **Formatting and parsing are separate from the engine**
- **Errors are reported, never imposed**
- **Nothing blocks, nothing allocates**

The goal is not to provide helpers for specific domains,  
but to make **time itself explicit and readable** everywhere it is used.

---

## Installation

Copy the `HestiaTempo` folder into your Arduino or PlatformIO libraries directory.

Then include:

```cpp
#include <HestiaTempo.h>
```

## Basic usage
### ID literal

Timer identifiers are symbolic and hashed at compile time.
```cpp
using Tempo::literals::operator"" _id;
```

Example:
```cpp
constexpr Tempo::Id HEARTBEAT = "HEARTBEAT"_id;
```
- Identifiers are opaque uint32_t values.
- Hash collisions are theoretically possible but extremely unlikely when using
- short, distinct names.

## Periodic timer (Interval)
```cpp
if (Tempo::interval("HEARTBEAT"_id).every(1000)) {
    // executed every 1000 ms
}
```

Human-readable input:
```cpp
if (Tempo::interval("HEARTBEAT"_id).every("00:00:01")) {
    // executed every second
}
```
## Behavior notes

- The first call initializes the timer and returns false
- The timer automatically rearms
- The interval is drift-resistant: late calls realign to the next expected boundary

## One-shot timer
```cpp
Tempo::oneShot("WATCHDOG"_id).start(5000);

if (Tempo::oneShot("WATCHDOG"_id).done()) {
    // timeout expired
}
```

Human-readable input:
```cpp
Tempo::oneShot("WATCHDOG"_id).start("00:00:05");
```
## Typical use cases

- watchdogs
- timeouts
- deferred actions
- long-press detection
- provisioning delays

## Reading elapsed and remaining time
```cpp
uint32_t elapsed   = Tempo::oneShot("WATCHDOG"_id).elapsed();
uint32_t remaining = Tempo::oneShot("WATCHDOG"_id).remaining();
```

Returned values are always in **milliseconds**.

If the timer is inactive, both functions return 0.

## Formatted output (diagnostics)

For logging and diagnostics, formatted helpers are provided:
```cpp
Serial.println(Tempo::remainingStr("WATCHDOG"_id));
Serial.println(Tempo::remainingStr("WATCHDOG"_id, Tempo::Format::HMS));
Serial.println(Tempo::remainingStr("WATCHDOG"_id, Tempo::Format::HMS_MS));
```
## Available formats
|Format |	Example |
|----------|----------|
|HMS_MS	| 00:00:03.512|
|HMS	| 00:00:03 |
|MS	| 3512 |
|AUTO_SHORT |	3 sec |

⚠️ These helpers use internal static buffers and are not re-entrant.
They are intended for diagnostics and logging only.

## Error handling

HestiaTempo uses a **non-intrusive error reporting model.**

- No logging
- No exceptions
- No blocking
- Fully optional

## Reading the last error
```cpp
Tempo::Error err = Tempo::lastError();
```
|Error |	Meaning |
|----------|----------|
|none	| No error|
|SlotTableFull | Maximum number of timers exceeded |
|InvalidFormat | Invalid HH:MM:SS duration string |
|IdKindMismatch |	Same Id used for Interval and OneShot |

Notes:
- Errors are overwritten on each new error
- Errors are non-fatal
- If you never query them, HestiaTempo remains silent

## Slot limit

HestiaTempo uses a fixed internal slot table:
```cpp
MAX_SLOTS = 32
```
This guarantees deterministic memory usage, suitable for embedded systems.

## Important rules
**Do not reuse an Id with different timer types**
```cpp
// ❌ Undefined behavior
Tempo::interval("TIMER"_id);
Tempo::oneShot("TIMER"_id);
```


Each identifier must be associated with exactly one timer kind.

## Where HestiaTempo fits

- HestiaTempo can be used at any level of a firmware:
- LED blinking
- debounce and long-press detection
- watchdogs and timeouts
- OTA and provisioning limits
- background heartbeats
- core orchestration and supervision

The difference is not what you time,
but how clearly time is expressed in the code.

## What HestiaTempo is not
- ❌ A scheduler
- ❌ A real-time clock
- ❌ A task manager
- ❌ A pin-control library

HestiaTempo measures durations, not dates or wall-clock time.

## License

MIT License
Use freely, modify responsibly.

## Status

**HestiaTempo API 1.0 is stable.**

Future versions may add:
- additional input formats
- optional diagnostics hooks