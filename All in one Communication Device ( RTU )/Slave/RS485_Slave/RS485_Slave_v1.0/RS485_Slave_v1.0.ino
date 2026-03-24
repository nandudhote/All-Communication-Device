#include <OneWire.h>
#include <DallasTemperature.h>

/* RS485 */
#define RS485_RX 16
#define RS485_TX 17
#define RS485_DIR 4

#define SLAVE_ID 1

/* DS18B20 */
#define ONE_WIRE_BUS 15

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

char rxBuffer[50];
int rxIndex = 0;

float temperature = 0;

void setup() {
  Serial.begin(9600);

  Serial2.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  pinMode(RS485_DIR, OUTPUT);
  digitalWrite(RS485_DIR, LOW);  // receive mode

  sensors.begin();

  Serial.println("RS485 DS18B20 Slave Ready");
}

void loop() {
  readRS485();
}

/* ---------- READ MASTER COMMAND ---------- */

void readRS485() {
  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      rxBuffer[rxIndex] = '\0';

      String cmd = String(rxBuffer);
      cmd.trim();

      processCommand(cmd);

      rxIndex = 0;
    } else {
      if (rxIndex < sizeof(rxBuffer) - 1) {
        rxBuffer[rxIndex++] = c;
      }
    }
  }
}

/* ---------- PROCESS COMMAND ---------- */

void processCommand(String cmd) {
  int id = cmd.substring(0, cmd.length() - 1).toInt();

  if (cmd.endsWith("?") && id == SLAVE_ID) {
    sendTemperature();
  }
}

/* ---------- READ TEMPERATURE ---------- */

float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

/* ---------- SEND RS485 RESPONSE ---------- */

void sendTemperature() {
  temperature = readTemperature();

  Serial.println("temperature : " + String(temperature));

  digitalWrite(RS485_DIR, HIGH);

  delayMicroseconds(200);

  Serial2.print("TEMP:");
  Serial2.println(temperature, 2);

  Serial2.flush();

  delayMicroseconds(200);

  digitalWrite(RS485_DIR, LOW);
}