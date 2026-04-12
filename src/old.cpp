#include <Arduino.h>
#include <avr8-stub.h>
#include <app_api.h> //only needed with flash breakpoints
#include <Wire.h>

#define SLAVE_ADDR 0x04
#define BUFFER_SIZE 64

// ================= TYPEDEFS =================
typedef void (*CommandHandler)(uint8_t*, uint8_t*);

// ================= ULTRASONIC =================
struct Ultrasonic {
  uint8_t trig;
  uint8_t echo;
  bool active;
};

Ultrasonic us[10]; // support up to 10 ultrasonic sensors

uint16_t readUS(Ultrasonic &d) {
  digitalWrite(d.trig, LOW);
  delayMicroseconds(2);
  digitalWrite(d.trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(d.trig, LOW);

  long dur = pulseIn(d.echo, HIGH, 30000);
  return (dur * 0.0343) / 2;
}

// ================= BUFFERS =================
uint8_t rawBuffer[BUFFER_SIZE];
char textBuffer[BUFFER_SIZE];
char responseBuffer[BUFFER_SIZE];

volatile bool newI2CMessage = false;
int rawLen = 0;

// ================= REGISTRY =================
struct CommandEntry {
  uint8_t cmd;
  CommandHandler handler;
};

#define MAX_COMMANDS 10
CommandEntry registry[MAX_COMMANDS];
uint8_t registryCount = 0;

void registerCommand(uint8_t cmd, CommandHandler handler) {
  if (registryCount < MAX_COMMANDS) {
    registry[registryCount++] = {cmd, handler};
  }
}


void dispatchCommand(uint8_t* req, uint8_t* res) {
  uint8_t cmd = req[2];
  Serial.println("dispatchCommand: " + String(cmd, HEX));
  for (int i = 0; i < registryCount; i++) {
    if (registry[i].cmd == cmd) {
      registry[i].handler(req, res);
      return;
    }
  }

  res[0] = 0xBB;
  res[2] = 1; // error
}

// ================= COMMAND IDS =================
#define CMD_PIN          0x01
#define CMD_ULTRASONIC   0x10



// ================= HANDLERS =================
void handlePin(uint8_t* req, uint8_t* res) {

  uint8_t cmdId   = req[1];
  uint8_t action  = req[3]; // 0=read,1=write
  uint8_t pinType = req[4];
  uint8_t pin     = req[5];
  uint8_t value   = req[6];

  uint16_t result = 0;
  uint8_t status = 0;

  if (action == 0) {
    result = (pinType == 1) ? analogRead(pin) : digitalRead(pin);
  } else {
    pinMode(pin, OUTPUT);
    if (pinType == 1)
      analogWrite(pin, value ? 255 : 0);
    else
      digitalWrite(pin, value);
  }

  res[0] = 0xBB;
  res[1] = cmdId;
  res[2] = status;
  res[3] = result >> 8;
  res[4] = result & 0xFF;

  Serial.print("Raw: Reply: ");
for (int i = 0; i < 4; i++) {
  Serial.print(res[i], HEX);
  Serial.print(" ");
}
Serial.println();
}

void handleUltrasonic(uint8_t* req, uint8_t* res) {

  uint8_t cmdId  = req[1];
  uint8_t action = req[3]; // 0=init,1=read
  uint8_t id     = req[4];

  uint16_t result = 0;
  uint8_t status = 0;

  if (action == 0) {
    us[id].trig = req[5];
    us[id].echo = req[6];
    us[id].active = true;

    pinMode(req[5], OUTPUT);
    pinMode(req[6], INPUT);

  } else if (action == 1) {
    if (us[id].active) {
      result = readUS(us[id]);
    } else {
      status = 1;
    }
  }

  res[0] = 0xBB;
  res[1] = cmdId;
  res[2] = status;
  // Split 16-bit result into two bytes
  res[3] = result >> 8;
  res[4] = result & 0xFF;
}

// ================= BINARY =================
void processBinary(uint8_t* data, int len) {
  dispatchCommand(data, (uint8_t*)responseBuffer);
}

// ================= TEXT (LEGACY + BATCH) =================

void processSingle(char* cmd, char* out) {

  char* saveptr2;
  char* token = strtok_r(cmd, ",", &saveptr2);

  // ===== DEVICE =====
  if (strcmp(token, "DEV") == 0) { //checks if first 3 chars are "DEV"

    char* devName = strtok_r(NULL, ",", &saveptr2);  // US1
    char* action  = strtok_r(NULL, ",", &saveptr2);

    int id = devName[2] - '0';

    if (strcmp(action, "init") == 0) {
      int trig = atoi(strtok_r(NULL, ",", &saveptr2));
      int echo = atoi(strtok_r(NULL, ",", &saveptr2));

      us[id].trig = trig;
      us[id].echo = echo;
      us[id].active = true;

      pinMode(trig, OUTPUT);
      pinMode(echo, INPUT);

      strcpy(out, "OK");

    } else if (strcmp(action, "read") == 0) {
      int dist = readUS(us[id]);
      itoa(dist, out, 10);
    }

    return;
  }

  // ===== PIN (LEGACY) =====
  char* pinStr = token;
  char* type   = strtok_r(NULL, ",", &saveptr2);
  char* value  = strtok_r(NULL, ",", &saveptr2);

  int pin = atoi(pinStr + 1);

  if (strcmp(type, "read") == 0) {

    int val = (pinStr[0] == 'A') ? analogRead(pin) : digitalRead(pin);
    itoa(val, out, 10);

  } else {

    int output = (value && strcmp(value, "On") == 0) ? HIGH : LOW;

    pinMode(pin, OUTPUT);

    if (pinStr[0] == 'A')
      analogWrite(pin, output ? 255 : 0);
    else
      digitalWrite(pin, output);

    strcpy(out, "OK");
  }
}

void processBatch(char* input, char* output) {

  output[0] = '\0';
  char* saveptr;
  char* cmd = strtok_r(input, ";", &saveptr);

  while (cmd != NULL) {

    char single[16];
    processSingle(cmd, single);

    if (strlen(output) > 0) strcat(output, ";");
    strcat(output, single);

    cmd = strtok_r(NULL, ";", &saveptr);
  }
}
//TODO Why does serial not also share this?
// ================= INPUT ROUTER =================
void processInput(uint8_t* data, int len) {

Serial.print("Len: ");
Serial.print(len);
Serial.print(" Raw: ");
for (int i = 0; i < len; i++) {
  Serial.print(data[i], HEX);
  Serial.print(" ");
}
Serial.println();

  int offset = 0;
  // Skip SMBus command byte if present
  if (len > 0 && data[0] != 0xAA && len > 1 && data[1] == 0xAA) {
    offset = 1;
  }

  if (len > offset && data[offset] == 0xAA) {
    Serial.println("Detected binary command");
    processBinary(data + offset, len - offset);
  } else {
    Serial.println("Detected text command");
    Serial.println(data[offset]);
    strncpy(textBuffer, (char*)(data + offset), BUFFER_SIZE);
    processBatch(textBuffer, responseBuffer);
  }

}

// ================= I2C =================
void receiveEvent(int count) {

  Serial.println("I2C message received");

  rawLen = 0;

  while (Wire.available() && rawLen < BUFFER_SIZE) {
    rawBuffer[rawLen++] = Wire.read();
  }

  processInput(rawBuffer, rawLen);
}

void requestEvent() {

  Serial.print("Raw: Reply From requestEvent: ");
for (int i = 0; i < sizeof(responseBuffer); i++) {
  Serial.print(responseBuffer[i], HEX);
  Serial.print(" ");
}
Serial.println();

  if (responseBuffer[0] == 0xBB) {
    Wire.write((uint8_t*)responseBuffer, 5);
  } else {
    Wire.write((uint8_t*)responseBuffer, strlen(responseBuffer));
  }
}

// ================= SERIAL =================

void handleSerial() {

  static uint8_t buffer[BUFFER_SIZE];
  static int index = 0;

  
  while (Serial.available()) {

    uint8_t b = Serial.read();

    // Text mode first (newline-terminated)
    if (b == '\n') {
      buffer[index] = '\0';
      processBatch((char*)buffer, responseBuffer);
      Serial.println(responseBuffer);
      index = 0;
      continue;
    }

    // Binary mode detection
    if (index == 0 && b == 0xAA) {
      buffer[index++] = b;
      continue;
    }

    if (index > 0 && buffer[0] == 0xAA) { // continue binary packet
      buffer[index++] = b;

      if (index == 7) { // full binary packet
        processBinary(buffer, index);
        Serial.write((uint8_t*)responseBuffer, 5);
        index = 0;
      }
      continue;
    }

    // Text accumulation
    buffer[index++] = b;

    // prevent overflow
    if (index >= BUFFER_SIZE) index = 0;
  }

  
}


// ================= SETUP =================
void setup() {
  //debug_init(); // Initialize the AVR debug stub
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.begin(9600); //--> debug_init uses serial

  registerCommand(CMD_PIN, handlePin);
  registerCommand(CMD_ULTRASONIC, handleUltrasonic);
}

// ================= LOOP =================
void loop() {


  
  handleSerial();
  //char cmd[] = "DEV,US0,init,9,10;DEV,US0,read";
  //processBatch(cmd, responseBuffer);
  //delay(2000);
}