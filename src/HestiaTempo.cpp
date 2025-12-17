#include "HestiaTempo.h"
#include "HestiaTempoFormat.h"
#include <Arduino.h>

/**
 * @file    HestiaTempo.cpp
 * @brief   Implementation of the HestiaTempo timing engine.
 *
 * @details
 * This file implements the internal timing engine backing the public
 * Interval and OneShot facades defined in HestiaTempo.h.
 *
 * Design characteristics:
 *  - Centralized slot table (static, fixed size)
 *  - No dynamic allocation
 *  - No heap usage
 *  - No String usage
 *  - All time measurements based on millis()
 *
 * The engine is intentionally hidden behind lightweight facade objects
 * (Interval / OneShot). These objects are stateless wrappers that reference
 * internal slots via a symbolic Tempo::Id.
 *
 * The engine guarantees:
 *  - Deterministic behavior
 *  - Non-blocking operation
 *  - Drift-resistant periodic timers (Interval)
 *
 * The engine does NOT:
 *  - Track wall-clock time
 *  - Perform formatting or parsing of time values
 *  - Provide thread-safety or ISR-safety
 */

namespace Tempo {

  // ============================================================================
  // Internal types
  // ============================================================================

  /**
   * @brief Internal timer kind.
   *
   * @note
   * A slot is permanently associated with its kind (Interval or OneShot)
   * once allocated.
   */
  enum class Kind : uint8_t {
    None,
    Interval,
    OneShot
  };

  /**
   * @brief Internal timer slot.
   *
   * @details
   * Each slot represents one logical timer identified by a Tempo::Id.
   * Slots are allocated lazily on first use and never freed.
   */
  struct Slot {
    Id       id      = 0;
    Kind     kind    = Kind::None;
    uint32_t start   = 0;   ///< Start timestamp (millis)
    uint32_t period  = 0;   ///< Duration or interval (ms)
    bool     active  = false;
  };

  /**
  * @brief Last recorded Tempo error.
  *
  * @note
  * This variable is intentionally file-local.
  */
  static Error g_lastError = Error::None;


  // ============================================================================
  // Slot storage
  // ============================================================================

  /**
   * @brief Maximum number of concurrent timers.
   *
   * @note
   * This value bounds memory usage deterministically.
   */
  static constexpr size_t MAX_SLOTS = 32;

  /**
   * @brief Static slot table.
   */
  static Slot slots[MAX_SLOTS];

  /**
   * @brief Retrieve or allocate a slot for a given Id and Kind.
   *
   * @param id Timer identifier.
   * @param expected Expected timer kind.
   * @return Pointer to slot or nullptr if table is full.
   *
   * @warning
   * If an existing slot with the same Id but a different Kind exists,
   * behavior is undefined. Users must not reuse the same Id for different
   * timer kinds.
   */
  static Slot* getSlot(Id id, Kind expected) {
    // Lookup existing slot
    for (auto& s : slots) {
      if (s.id == id) {
          if (s.kind != expected) {
            g_lastError = Error::IdKindMismatch;
          }
        return &s;
      }
    }

    // Allocate new slot
    for (auto& s : slots) {
      if (s.kind == Kind::None) {
        s.id   = id;
        s.kind = expected;
        return &s;
      }
    }

    g_lastError = Error::SlotTableFull;

    // No free slot
    return nullptr;
  }

  // ============================================================================
  // Interval implementation
  // ============================================================================

  Interval::Interval(Id id) : _id(id) {}

  bool Interval::every(uint32_t period_ms) {
    Slot* s = getSlot(_id, Kind::Interval);
    if (!s) return false;

    const uint32_t now = millis();

    // First call: initialize
    if (!s->active) {
      s->period = period_ms;
      s->start  = now;
      s->active = true;
      return false;
    }

    // Expiration check (unsigned arithmetic handles wrap-around)
    if ((uint32_t)(now - s->start) >= s->period) {
      // Drift-resistant realignment
      s->start += s->period;
      return true;
    }

    return false;
  }

  bool Interval::every(const char* hms) {
    uint32_t ms;
    if (!HestiaTempoFormat::parseHMS(hms, ms)) {
      g_lastError = Error::InvalidFormat;
      return false;
    }
    return every(ms);
  }

  // ============================================================================
  // OneShot implementation
  // ============================================================================

  OneShot::OneShot(Id id) : _id(id) {}

  void OneShot::start(uint32_t duration_ms) {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s) return;

    s->period = duration_ms;
    s->start  = millis();
    s->active = true;
  }

  void OneShot::start(const char* hms) {
    uint32_t ms;
    if (!HestiaTempoFormat::parseHMS(hms, ms)) {
      g_lastError = Error::InvalidFormat;
      return;
    }
    start(ms);
  }

  void OneShot::restart() {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s || !s->active) return;

    s->start = millis();
  }

  void OneShot::cancel() {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s) return;

    s->active = false;
  }

  bool OneShot::running() const {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s || !s->active) return false;

    return (uint32_t)(millis() - s->start) < s->period;
  }

  bool OneShot::done() const {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s || !s->active) return false;

    return (uint32_t)(millis() - s->start) >= s->period;
  }

  uint32_t OneShot::elapsed() const {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s || !s->active) return 0;

    return (uint32_t)(millis() - s->start);
  }

  uint32_t OneShot::remaining() const {
    Slot* s = getSlot(_id, Kind::OneShot);
    if (!s || !s->active) return 0;

    const uint32_t e = (uint32_t)(millis() - s->start);
    return (e >= s->period) ? 0 : (s->period - e);
  }

  // ============================================================================
  // Facade entry points
  // ============================================================================

  Interval interval(Id id) {
    return Interval(id);
  }

  OneShot oneShot(Id id) {
    return OneShot(id);
  }

  // ============================================================================
  // Formatting facade
  // ============================================================================

  const char* remainingStr(Id id, Format fmt) {
    return HestiaTempoFormat::remainingStr(
      id,
      static_cast<Tempo::Format>(fmt)
    );
  }

  const char* elapsedStr(Id id, Format fmt) {
    return HestiaTempoFormat::elapsedStr(
      id,
      static_cast<Tempo::Format>(fmt)
    );
  }

  // ============================================================================
  // Error
  // ============================================================================
  Error lastError() {
  return g_lastError;
  }


} // namespace Tempo
