#include <Arduino.h>
#include <Crc16.h>

// CRC 16 library (XModem)
Crc16 crc;
// bits per seconds
#define TX_RATE 10
#define TX_CLOCK 2
#define TX_DATA 3

const char* message = "Hello, world";
unsigned short crcValue = crc.XModemCrc(message, 0, strlen(message));

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

// sendShort sends the input short to TX_DATA one bit at the time
void sendShort(unsigned short data) {
  int sizeInBytes = sizeof(data);

  for (int i = sizeInBytes; i > 0; i--) {
    unsigned short shiftedData = data >> (8 * (i - 1));
    byte dataByte = shiftedData & 0b11111111;

    sendByte(dataByte);
  }
}

// sendMessage sends the array of characters to TX_DATA one 
// bit at the time
void sendMessage(const char* message) {
  for (byte* t = message; *t != '\0'; t++) {
      sendByte(*t);
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

    // send message size (number of bits)
    sendShort(8 * strlen(message));
    // send the actual message
    sendMessage(message);
    // send CRC of the message (size is not include in the CRC, but it should)
    sendShort(crcValue);

    digitalWrite(TX_DATA, LOW);
    digitalWrite(TX_CLOCK, LOW);
  }
}
