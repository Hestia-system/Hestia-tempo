#include "HestiaTempoFormat.h"
#include <stdio.h>

/**
 * @file    HestiaTempoFormat.cpp
 * @brief   Formatting and parsing implementation for HestiaTempo.
 *
 * @details
 * This file implements the presentation layer of HestiaTempo.
 * It converts raw millisecond durations into human-readable strings
 * and parses strict textual durations into milliseconds.
 *
 * Design goals:
 *  - No dynamic allocation
 *  - No Arduino String
 *  - Deterministic output
 *  - Small code size
 *
 * This layer is intentionally independent from the timing engine
 * (no access to slots, no millis(), no state).
 */

namespace HestiaTempoFormat {

  // ============================================================================
  // Formatting helpers
  // ============================================================================

  /**
   * @brief Format milliseconds as "HH:MM:SS.mmm".
   */
  static void formatHMSms(uint32_t ms, char* out, size_t len) {
    const uint32_t totalSeconds = ms / 1000UL;
    const uint32_t milliseconds = ms % 1000UL;

    const uint32_t seconds = totalSeconds % 60UL;
    const uint32_t totalMinutes = totalSeconds / 60UL;
    const uint32_t minutes = totalMinutes % 60UL;
    const uint32_t hours   = totalMinutes / 60UL;

    snprintf(
      out,
      len,
      "%02lu:%02lu:%02lu.%03lu",
      hours,
      minutes,
      seconds,
      milliseconds
    );
  }

  /**
   * @brief Format milliseconds as "HH:MM:SS".
   */
  static void formatHMS(uint32_t ms, char* out, size_t len) {
    const uint32_t totalSeconds = ms / 1000UL;

    const uint32_t seconds = totalSeconds % 60UL;
    const uint32_t totalMinutes = totalSeconds / 60UL;
    const uint32_t minutes = totalMinutes % 60UL;
    const uint32_t hours   = totalMinutes / 60UL;

    snprintf(
      out,
      len,
      "%02lu:%02lu:%02lu",
      hours,
      minutes,
      seconds
    );
  }

  /**
   * @brief Format milliseconds as a raw millisecond string.
   */
  static void formatMS(uint32_t ms, char* out, size_t len) {
    snprintf(out, len, "%lu", ms);
  }

  /**
   * @brief Format milliseconds using a human-friendly short representation.
   */
  static void formatAutoShort(uint32_t ms, char* out, size_t len) {
    if (ms < 1000UL) {
      snprintf(out, len, "%lu ms", ms);
      return;
    }

    if (ms < 60000UL) {
      snprintf(out, len, "%lu sec", ms / 1000UL);
      return;
    }

    snprintf(out, len, "%lu min", ms / 60000UL);
  }

  // ============================================================================
  // Public formatting entry point
  // ============================================================================

  void format(uint32_t ms, char* out, size_t len, TimeFormat fmt) {
    if (!out || len == 0) {
      return;
    }

    switch (fmt) {
      case TimeFormat::HMS_MS:
        formatHMSms(ms, out, len);
        break;

      case TimeFormat::HMS:
        formatHMS(ms, out, len);
        break;

      case TimeFormat::MS:
        formatMS(ms, out, len);
        break;

      case TimeFormat::AUTO_SHORT:
      default:
        formatAutoShort(ms, out, len);
        break;
    }
  }

  // ============================================================================
  // Parsing
  // ============================================================================

  bool parseHMS(const char* str, uint32_t& out_ms) {
    if (!str) return false;

    uint32_t hh = 0;
    uint32_t mm = 0;
    uint32_t ss = 0;
    char tail = 0;

    // Strict "HH:MM:SS" parsing.
    // The trailing character ensures full-string validation.
    const int n = sscanf(str, "%lu:%lu:%lu%c", &hh, &mm, &ss, &tail);
    if (n != 3) return false;
    if (mm > 59 || ss > 59) return false;

    out_ms =
      hh * 3600000UL +
      mm *   60000UL +
      ss *    1000UL;

    return true;
  }

  // ============================================================================
  // Internal string helpers
  // ============================================================================

  const char* remainingStr(Tempo::Id id, TimeFormat fmt) {
    static char buf[16];
    format(
      Tempo::oneShot(id).remaining(),
      buf,
      sizeof(buf),
      fmt
    );
    return buf;
  }

  const char* elapsedStr(Tempo::Id id, TimeFormat fmt) {
    static char buf[16];
    format(
      Tempo::oneShot(id).elapsed(),
      buf,
      sizeof(buf),
      fmt
    );
    return buf;
  }

} // namespace HestiaTempoFormat
