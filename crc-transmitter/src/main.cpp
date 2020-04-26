#include <Arduino.h>
#include <Crc16.h>

// CRC 16 library (XModem)
Crc16 crc;
// bits per seconds
#define TX_RATE 500
#define TX_CLOCK 2
#define TX_DATA 3

const char* message = "Hello, world";
unsigned short crcValue = crc.XModemCrc(message, 0, strlen(message));

int charsToBytes(const char* message, byte dest[], int start) {
  int bytesWritten = 0;
  for (byte* t = message; *t != '\0'; t++) {
    dest[start++] = *t;
    bytesWritten++;
  }
  return bytesWritten;
}

int shortToBytes(unsigned short data, byte dest[], int start) {
  int sizeInBytes = sizeof(data);
  int bytesWritten = 0;

  for (int i = sizeInBytes; i > 0; i--) {
    unsigned short shiftedData = data >> (8 * (i - 1));
    byte dataByte = shiftedData & 0b11111111;
    dest[start++] = dataByte;
    bytesWritten++;
  }
  return bytesWritten;
}

struct out {
  byte* data;
  int size;
};

out toByte(const char* message) {
  const int messageLengthSizeBytes = 2;
  const int crcSizeBytes = 2;
  const int messageSizeBytes = strlen(message);
  int packetSizeBytes = messageLengthSizeBytes + messageSizeBytes + crcSizeBytes;
  byte* packet = (byte*)malloc(sizeof(byte) * packetSizeBytes);

  int w1 = shortToBytes((unsigned short)messageSizeBytes * 8, packet, 0);
  int w2 = charsToBytes(message, packet, w1);
  unsigned short crcData = crc.XModemCrc(packet, 0, w1 + w2);
  int w3 = shortToBytes(crcData, packet, w1 + w2);

  return out{packet, packetSizeBytes};
}

void setup() {
  pinMode(TX_CLOCK, OUTPUT);
  pinMode(TX_DATA, OUTPUT);
  digitalWrite(TX_CLOCK, LOW);
  digitalWrite(TX_DATA, LOW);
  Serial.begin(9600);
  while (!Serial)
    ;
}

// sendByte sends the input byte to TX_DATA one bit at the time
void sendByte(byte input) {
  for (int i = 0; i < 8; i++) {
    bool toSend = input & (0b10000000 >> i);

    digitalWrite(TX_DATA, toSend == false ? LOW : HIGH);
    delay((1000 / TX_RATE) / 2);

    digitalWrite(TX_CLOCK, HIGH);
    delay((1000 / TX_RATE) / 2);
    digitalWrite(TX_CLOCK, LOW);
  }
}

void sendPacket(byte packet[], int size) {
  for (int i = 0; i < size; i++) {
    sendByte(packet[i]);
  }
}

bool done = false;

void loop() {
  if (!done) {
    // init
    digitalWrite(TX_DATA, LOW);
    digitalWrite(TX_CLOCK, HIGH);
    delay(1100);
    digitalWrite(TX_CLOCK, LOW);
    done = true;

    out o = toByte(message);
    for (int i = 0; i < o.size; i++) {
      Serial.print(o.data[i], DEC);
      Serial.print(" ");
    }
    sendPacket(o.data, o.size);

    digitalWrite(TX_DATA, LOW);
    digitalWrite(TX_CLOCK, LOW);
  }
}
