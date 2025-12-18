
/*
  Example0Tempo

  This example shows the basic usage of the HestiaTempo library.

  It demonstrates:
  - How to create named timers using symbolic identifiers
  - How to use a periodic timer (Interval)
  - How to use a one-shot timer (OneShot)
  - How to write non-blocking timing code without delay() or millis()

  HestiaTempo makes time explicit and readable.
  Instead of manual millis() calculations, timers are expressed declaratively.

  --------------------------------------------------------------------

  Interval example:
    Executes a block of code at a fixed period.

      if (Tempo::interval("HEARTBEAT"_id).every(1000)) {
        // runs every second
      }

  OneShot example:
    Executes a block of code once, after a delay.

      Tempo::oneShot("STARTUP"_id).start(5000);

      if (Tempo::oneShot("STARTUP"_id).done()) {
        // runs once after 5 seconds
      }

  Timers are non-blocking:
    The loop() function keeps running normally.

  Time can also be expressed in a human-readable format:

      Tempo::oneShot("WATCHDOG"_id).start("00:00:10");

  --------------------------------------------------------------------

  This example is suitable for beginners and advanced users alike.
  It can be used in simple sketches or as a starting point for
  more structured firmware designs.
*/

#include "HestiaTempo.h"
using Tempo::literals::operator"" _id;
#include <Arduino.h>

void setup() {
  
  Serial.begin(115200);
  delay(1000);

}

void loop() {
  if (!Tempo::oneShot("test"_id).running()) Tempo::oneShot("test"_id).start("00:02:10");
  if (Tempo::lastError() == Tempo::Error::InvalidFormat) {
    // fallback, log, assert, etc.
  }

  if (Tempo::interval("HEARTBEAT"_id).every(1000)) {

    Serial.println(Tempo::oneShot("test"_id).remaining());
    Serial.println(Tempo::remainingStr("test"_id, Tempo::Format::AUTO_SHORT));
    Serial.println(Tempo::remainingStr("test"_id, Tempo::Format::HMS_MS));
    Serial.println(Tempo::remainingStr("test"_id, Tempo::Format::HMS));
    Serial.println(Tempo::remainingStr("test"_id, Tempo::Format::MS));

  }


}
