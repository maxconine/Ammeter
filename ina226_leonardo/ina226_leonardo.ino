#include <Wire.h>
#include <INA226.h>

// Pro Micro / Leonardo (ATmega32U4): hardware I2C is always D2=SDA, D3=SCL.
// Common INA226 breakout addresses depend on A0/A1 strapping.
constexpr uint8_t INA226_ADDR_CANDIDATES[] = {0x40, 0x41, 0x44, 0x45};

INA226 ina40(0x40);
INA226 ina41(0x41);
INA226 ina44(0x44);
INA226 ina45(0x45);
INA226* ina = &ina40;

// Change these values if your external shunt is not 100 A / 75 mV.
// Current is I = Vshunt / Rshunt. Supply voltage (e.g. 24 V) is NOT part of
// this formula — it is only used later for power = Vbus * I.
constexpr float SHUNT_RATED_CURRENT_A = 100.0f;
constexpr float SHUNT_RATED_VOLTAGE_V = 0.075f;
constexpr float SHUNT_RESISTANCE_OHM =
    SHUNT_RATED_VOLTAGE_V / SHUNT_RATED_CURRENT_A;

// Change to -1.0 if the displayed current has the wrong sign.
constexpr float CURRENT_DIRECTION = 1.0f;

constexpr unsigned long SAMPLE_INTERVAL_MS = 100;
constexpr unsigned long SENSOR_RETRY_INTERVAL_MS = 2000;

bool sensorReady = false;
bool measuring = false;
unsigned long measurementStartMs = 0;
unsigned long lastSampleMs = 0;
unsigned long lastSensorRetryMs = 0;
float zeroOffsetV = 0.0f;

String commandBuffer;

void configureSensor() {
  // Extra averaging helps on a 100 A shunt where low currents are only ~mV.
  ina->setAverage(INA226_64_SAMPLES);
  ina->setShuntVoltageConversionTime(INA226_2100_us);
  ina->setBusVoltageConversionTime(INA226_1100_us);
  ina->setModeShuntBusContinuous();
}

void reportI2CScan() {
  Serial.print(F("I2C_SCAN"));
  uint8_t found = 0;

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print(',');
      Serial.print(F("0x"));
      if (address < 16) {
        Serial.print('0');
      }
      Serial.print(address, HEX);
      ++found;
    }
  }

  if (found == 0) {
    Serial.print(F(",NONE"));
  }
  Serial.println();
}

bool tryInitSensor() {
  INA226* candidates[] = {&ina40, &ina41, &ina44, &ina45};

  for (uint8_t i = 0; i < 4; ++i) {
    if (candidates[i]->begin()) {
      ina = candidates[i];
      configureSensor();

      Serial.print(F("INFO,INA226_ADDR,0x"));
      Serial.println(INA226_ADDR_CANDIDATES[i], HEX);
      return true;
    }
  }

  return false;
}

float measureZeroOffset() {
  constexpr int SAMPLE_COUNT = 200;
  double sum = 0.0;

  for (int i = 0; i < SAMPLE_COUNT; ++i) {
    sum += ina->getShuntVoltage();
    delay(5);
  }

  return static_cast<float>(sum / SAMPLE_COUNT);
}

void sendStatus(const __FlashStringHelper* message) {
  Serial.print(F("STATUS,"));
  Serial.println(message);
}

void reportSensorMissing() {
  sendStatus(F("INA226_MISSING"));
  Serial.println(F("ERROR,INA226_NOT_FOUND"));
  reportI2CScan();
}

void handleCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command == "START") {
    if (!sensorReady) {
      reportSensorMissing();
      return;
    }
    measuring = true;
    measurementStartMs = millis();
    lastSampleMs = measurementStartMs - SAMPLE_INTERVAL_MS;
    sendStatus(F("MEASURING"));
  } else if (command == "STOP") {
    measuring = false;
    sendStatus(F("STOPPED"));
  } else if (command == "ZERO") {
    if (!sensorReady) {
      reportSensorMissing();
      return;
    }

    bool wasMeasuring = measuring;
    measuring = false;
    sendStatus(F("ZEROING"));

    zeroOffsetV = measureZeroOffset();

    Serial.print(F("ZERO,"));
    Serial.println(zeroOffsetV * 1000.0f, 6);

    measuring = wasMeasuring;
    if (measuring) {
      measurementStartMs = millis();
      lastSampleMs = measurementStartMs - SAMPLE_INTERVAL_MS;
      sendStatus(F("MEASURING"));
    } else {
      sendStatus(F("STOPPED"));
    }
  } else if (command == "PING" || command == "SCAN") {
    if (sensorReady) {
      sendStatus(F("READY"));
      Serial.print(F("INFO,INA226_ADDR,0x"));
      Serial.println(ina->getAddress(), HEX);
    } else {
      reportSensorMissing();
    }
  } else if (command.length() > 0) {
    Serial.print(F("ERROR,UNKNOWN_COMMAND,"));
    Serial.println(command);
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());

    if (c == '\n') {
      handleCommand(commandBuffer);
      commandBuffer = "";
    } else if (c != '\r') {
      if (commandBuffer.length() < 40) {
        commandBuffer += c;
      } else {
        commandBuffer = "";
        Serial.println(F("ERROR,COMMAND_TOO_LONG"));
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Pro Micro / Leonardo use native USB; wait briefly so early lines are seen.
  const unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < 3000) {
    // wait for host
  }

  Wire.begin();
  // 100 kHz is more tolerant of breadboards, long jumpers, and weak pull-ups.
  Wire.setClock(100000);

  sensorReady = tryInitSensor();
  lastSensorRetryMs = millis();

  if (sensorReady) {
    delay(100);
    lastSampleMs = millis() - SAMPLE_INTERVAL_MS;
    sendStatus(F("READY"));
  } else {
    reportSensorMissing();
  }
}

void loop() {
  readSerialCommands();

  if (!sensorReady) {
    measuring = false;
    const unsigned long now = millis();
    if (now - lastSensorRetryMs >= SENSOR_RETRY_INTERVAL_MS) {
      lastSensorRetryMs = now;
      sensorReady = tryInitSensor();
      if (sensorReady) {
        lastSampleMs = millis() - SAMPLE_INTERVAL_MS;
        sendStatus(F("READY"));
      } else {
        reportI2CScan();
      }
    }
    return;
  }

  unsigned long now = millis();

  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return;
  }

  lastSampleMs += SAMPLE_INTERVAL_MS;

  float shuntVoltageV = ina->getShuntVoltage() - zeroOffsetV;
  float currentA =
      CURRENT_DIRECTION * shuntVoltageV / SHUNT_RESISTANCE_OHM;
  float busVoltageV = ina->getBusVoltage();

  // On a 100 A / 75 mV shunt, ~0.75 mV ≈ 1 A. Small residual offset looks huge.
  if (currentA > -0.05f && currentA < 0.05f) {
    currentA = 0.0f;
  }

  if (measuring) {
    unsigned long elapsedMs = now - measurementStartMs;

    // DATA,elapsedMs,currentA,busV,shuntmV
    Serial.print(F("DATA,"));
    Serial.print(elapsedMs);
    Serial.print(',');
    Serial.print(currentA, 4);
    Serial.print(',');
    Serial.print(busVoltageV, 3);
    Serial.print(',');
    Serial.println(shuntVoltageV * 1000.0f, 4);
  } else {
    // LIVE,currentA,busV,shuntmV
    Serial.print(F("LIVE,"));
    Serial.print(currentA, 4);
    Serial.print(',');
    Serial.print(busVoltageV, 3);
    Serial.print(',');
    Serial.println(shuntVoltageV * 1000.0f, 4);
  }
}
