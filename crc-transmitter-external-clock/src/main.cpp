#include <Arduino.h>
#include <Crc16.h>

// CRC 16 library (XModem)
Crc16 crc;

#define TX_CLOCK 2
#define TX_DATA 3

const char* message = "TVB Churra!";
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
} packet;

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

struct streamDescriptor {
  int currentByte;
  int currentBit;
};

streamDescriptor *streamDsc = (struct streamDescriptor*) malloc(sizeof(streamDescriptor));


volatile bool shouldSendPreamble = true;
const int preambleLength = 25;
volatile bool preamble[preambleLength] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,   1, 1, 1, 1, 1};
volatile int currentPreambleIndex = 0;

void onClockRising() {
  if (shouldSendPreamble) {
    digitalWrite(TX_DATA, preamble[currentPreambleIndex] == false ? LOW : HIGH);
    Serial.println(preamble[currentPreambleIndex] == false ? 0 : 1);
    currentPreambleIndex++;
    if (currentPreambleIndex == preambleLength) {
      Serial.println("Preamble sent");
      shouldSendPreamble = false;
    }
    return;
  }
  if (streamDsc->currentByte >= packet.size) {
    return;
  }
  byte* data = packet.data;
  bool toSend = data[streamDsc->currentByte] & (0b10000000 >> streamDsc->currentBit);
  streamDsc->currentBit = (streamDsc->currentBit + 1) % 8;
  Serial.print(toSend ? 1 : 0);
  if (streamDsc->currentBit == 0) {
    streamDsc->currentByte++;
    Serial.println();
  }
  digitalWrite(TX_DATA, toSend == false ? LOW : HIGH);
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  pinMode(TX_CLOCK, INPUT);
  streamDsc->currentBit = 0;
  streamDsc->currentByte = 0;

  // clear interrupt register, to make sure the handler does not get called
  // because of some garbage in the register
  EIFR = (1 << INTF1);
  attachInterrupt(digitalPinToInterrupt(TX_CLOCK), onClockRising, RISING);

  //pinMode(TX_CLOCK, INPUT);
  pinMode(TX_DATA, OUTPUT);
  digitalWrite(TX_DATA, LOW);

  packet = toByte(message);
}

void loop() {
  // put your main code here, to run repeatedly:
}