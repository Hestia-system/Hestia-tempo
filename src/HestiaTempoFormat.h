#pragma once
#include <stdint.h>
#include <stddef.h>
#include "HestiaTempo.h"

/**
 * @file    HestiaTempoFormat.h
 * @brief   Formatting and parsing utilities for HestiaTempo.
 *
 * @details
 * This header declares the formatting/parsing layer used by HestiaTempo.
 *
 * Responsibilities:
 *  - Convert raw millisecond durations to human-readable strings
 *  - Parse human-readable duration strings into milliseconds
 *
 * Non-responsibilities:
 *  - No timing logic
 *  - No slot management
 *  - No use of millis()
 *  - No dynamic allocation
 *
 * This layer exists to keep the core timing engine minimal, deterministic,
 * and independent from presentation concerns.
 *
 * @note
 * This header is not intended to be included directly by end users.
 * Public access is provided through Tempo::remainingStr() and
 * Tempo::elapsedStr().
 */

/**
 * @brief Internal alias for time formatting policy.
 *
 * @details
 * This alias ensures that the formatting layer uses the same enum
 * as the public API (Tempo::Format), avoiding duplication or casts.
 */
using TimeFormat = Tempo::Format;

namespace HestiaTempoFormat {

  // ============================================================================
  // Formatting
  // ============================================================================

  /**
   * @brief Format a duration expressed in milliseconds.
   *
   * @param ms   Duration in milliseconds.
   * @param out  Output buffer (must be provided by caller).
   * @param len  Size of output buffer.
   * @param fmt  Formatting policy.
   *
   * @details
   * The function guarantees:
   *  - Null-terminated output (if len > 0)
   *  - No buffer overflow
   *
   * Formatting policies:
   *  - HMS_MS     → "HH:MM:SS.mmm"
   *  - HMS        → "HH:MM:SS"
   *  - MS         → raw milliseconds (implementation-defined text)
   *  - AUTO_SHORT → "123 ms", "5 sec", "2 min"
   */
  void format(uint32_t ms, char* out, size_t len, TimeFormat fmt);

  // ============================================================================
  // Parsing
  // ============================================================================

  /**
   * @brief Parse a strict "HH:MM:SS" duration string.
   *
   * @param str     Input string (must not be null).
   * @param out_ms  Output duration in milliseconds.
   * @return true on successful parsing, false otherwise.
   *
   * @details
   * Accepted format:
   *  - "HH:MM:SS"
   *
   * Constraints:
   *  - MM and SS must be in range [0..59]
   *  - HH is unbounded (within uint32_t arithmetic limits)
   *
   * Rejected examples:
   *  - "1:2:3"
   *  - "01:02"
   *  - "10s"
   *  - "01:02:03.5"
   *
   * This function performs no allocation and does not modify global state.
   */
  bool parseHMS(const char* str, uint32_t& out_ms);

  // ============================================================================
  // Internal string helpers (used by Tempo facade)
  // ============================================================================

  /**
   * @brief Return remaining time as a formatted string.
   *
   * @warning
   * Uses a static internal buffer. Not re-entrant.
   */
  const char* remainingStr(Tempo::Id id, TimeFormat fmt);

  /**
   * @brief Return elapsed time as a formatted string.
   *
   * @warning
   * Uses a static internal buffer. Not re-entrant.
   */
  const char* elapsedStr(Tempo::Id id, TimeFormat fmt);

} // namespace HestiaTempoFormat
