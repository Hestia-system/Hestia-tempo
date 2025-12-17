
#include "HestiaTempo.h"
using Tempo::literals::operator"" _id;
#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
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
