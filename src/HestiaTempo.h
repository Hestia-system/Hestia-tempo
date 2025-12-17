#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @file    HestiaTempo.h
 * @brief   HestiaTempo — non-blocking timers with symbolic IDs.
 *
 * @details
 * HestiaTempo provides a small, allocation-free timing service designed for
 * embedded systems (Arduino / ESP32 class MCUs).
 *
 * Core goals:
 *  - Replace direct use of millis() with readable, intention-driven code
 *  - Avoid dynamic allocation and String usage
 *  - Allow timers to be addressed symbolically (by name, hashed at compile time)
 *  - Keep the timing engine independent from formatting / presentation
 *
 * Two timer primitives are provided:
 *  - Interval : periodic timer (auto-rearming, drift-resistant)
 *  - OneShot  : single-shot timer (delay / timeout / watchdog)
 *
 * Timers are identified by a Tempo::Id (uint32_t), typically created at
 * compile time using the user-defined literal `"NAME"_id`.
 *
 * Example:
 * @code
 * using Tempo::literals::operator"" _id;
 *
 * if (Tempo::interval("HEARTBEAT"_id).every(1000)) {
 *     // called every second
 * }
 *
 * Tempo::oneShot("WATCHDOG"_id).start("00:00:05");
 * if (Tempo::oneShot("WATCHDOG"_id).done()) {
 *     // timeout expired
 * }
 * @endcode
 *
 * @note
 * Formatting helpers (elapsedStr / remainingStr) are provided for diagnostics
 * and logging. The core engine always operates on milliseconds internally.
 *
 * @warning
 * Formatting helpers return pointers to internal static buffers and are not
 * re-entrant. Do not use concurrently from multiple threads/ISRs.
 */

namespace Tempo {

  /**
   * @brief Timer identifier type.
   *
   * @details
   * IDs are opaque 32-bit values. Collisions are possible in theory but extremely
   * unlikely when using short, distinct symbolic names.
   */
  using Id = uint32_t;

  // ============================================================================
  // Compile-time ID generation (FNV-1a)
  // ============================================================================
  namespace literals {

    /**
     * @brief Compute a 32-bit FNV-1a hash at compile time.
     *
     * @param str Pointer to the string literal.
     * @param len Length of the string.
     * @return 32-bit hash value.
     */
    constexpr Id fnv1a(const char* str, size_t len) {
      Id hash = 0x811C9DC5u;
      for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<Id>(str[i]);
        hash *= 0x01000193u;
      }
      return hash;
    }

    /**
     * @brief User-defined literal to generate a Tempo::Id.
     *
     * Example:
     * @code
     * constexpr Tempo::Id WATCHDOG = "WATCHDOG"_id;
     * @endcode
     */
    constexpr Id operator"" _id(const char* str, size_t len) {
      return fnv1a(str, len);
    }

  } // namespace literals

  /**
   * @brief Optional convenience import for `_id` literal.
   *
   * Users may alternatively import it explicitly:
   * @code
   * using Tempo::literals::operator"" _id;
   * @endcode
   */
  using namespace literals;

  // ============================================================================
  // Time formatting policy (public API)
  // ============================================================================
  /**
   * @brief Output formatting policy for elapsed / remaining durations.
   */
  enum class Format : uint8_t {
    /** HH:MM:SS.mmm (e.g. "00:00:03.512") */
    HMS_MS,

    /** HH:MM:SS */
    HMS,

    /** Raw milliseconds (implementation-defined textual form) */
    MS,

    /** Human-friendly short format ("123 ms", "5 sec", "2 min") */
    AUTO_SHORT
  };

  /**
 * @brief Tempo runtime error codes.
 *
 * @details
 * Tempo uses a lightweight, non-intrusive error reporting mechanism.
 * Errors are recorded internally and can be queried by the application.
 *
 * Characteristics:
 *  - Errors are non-fatal
 *  - No logging is performed by the library
 *  - No exceptions are used
 *  - The last error overwrites any previous one
 *
 * If the application never queries the error state, Tempo behaves silently.
 */
enum class Error : uint8_t {
  /** No error occurred. */
  None = 0,

  /** Internal slot table is full (MAX_SLOTS exceeded). */
  SlotTableFull,

  /** Invalid time format (e.g. malformed "HH:MM:SS"). */
  InvalidFormat,

  /**
   * A timer Id was reused with a different timer kind
   * (Interval vs OneShot).
   *
   * @warning
   * This condition indicates a programming error.
   * Behavior is undefined if ignored.
   */
  IdKindMismatch
};

/**
 * @brief Return the last Tempo error.
 *
 * @return Last recorded error code.
 *
 * @note
 * Calling this function does not clear the error.
 * The error state is overwritten on the next error occurrence.
 */
Error lastError();


  // ============================================================================
  // Interval timer
  // ============================================================================
  /**
   * @brief Periodic non-blocking timer.
   *
   * @details
   * `every()` returns true exactly once per period and automatically rearms.
   * If the call is late, the timer realigns to the next expected boundary
   * (drift-resistant behavior).
   */
  class Interval {
  public:
    /**
     * @brief Construct an Interval facade bound to a timer Id.
     */
    explicit Interval(Id id);

    /**
     * @brief Check whether the interval has expired.
     *
     * @param period_ms Interval duration in milliseconds.
     * @return true if the interval just expired, false otherwise.
     */
    bool every(uint32_t period_ms);

    /**
     * @brief Same as every(uint32_t) but accepts a "HH:MM:SS" string.
     */
    bool every(const char* hms);

  private:
    Id _id;
  };

  // ============================================================================
  // One-shot timer
  // ============================================================================
  /**
   * @brief Single-shot non-blocking timer.
   *
   * @details
   * Typical use cases: delays, watchdogs, timeouts, deferred actions.
   */
  class OneShot {
  public:
    /**
     * @brief Construct a OneShot facade bound to a timer Id.
     */
    explicit OneShot(Id id);

    /**
     * @brief Start the timer with a duration in milliseconds.
     */
    void start(uint32_t duration_ms);

    /**
     * @brief Start the timer using a strict "HH:MM:SS" duration string.
     */
    void start(const char* hms);

    /**
     * @brief Restart the timer using the previously configured duration.
     */
    void restart();

    /**
     * @brief Cancel the timer.
     */
    void cancel();

    /**
     * @brief Check whether the timer is currently running.
     */
    bool running() const;

    /**
     * @brief Check whether the timer has expired.
     */
    bool done() const;

    /**
     * @brief Elapsed time since start, in milliseconds.
     */
    uint32_t elapsed() const;

    /**
     * @brief Remaining time before expiration, in milliseconds.
     */
    uint32_t remaining() const;
    

  private:
    Id _id;
  };

  // ============================================================================
  // Facade entry points
  // ============================================================================
  /**
   * @brief Obtain an Interval facade for a given Id.
   */
  Interval interval(Id id);

  /**
   * @brief Obtain a OneShot facade for a given Id.
   */
  OneShot oneShot(Id id);

  // ============================================================================
  // Formatting helpers (diagnostics / logging)
  // ============================================================================
  /**
   * @brief Get remaining time as a formatted string.
   *
   * @warning Uses an internal static buffer (not re-entrant).
   */
  const char* remainingStr(Id id, Format fmt = Format::AUTO_SHORT);

  /**
   * @brief Get elapsed time as a formatted string.
   *
   * @warning Uses an internal static buffer (not re-entrant).
   */
  const char* elapsedStr(Id id, Format fmt = Format::AUTO_SHORT);

} // namespace Tempo
