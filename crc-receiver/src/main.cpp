#include <Arduino.h>
#include "preamble.h"

#define TX_CLOCK 2
#define TX_DATA 3

#include <Crc16.h>
#include <LiquidCrystal595.h>
//Crc 16 library (XModem)
Crc16 crc;

// initialize the library with the numbers of the interface pins
// LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
LiquidCrystal595 lcd(5, 6, 7);
volatile bool updateLcd = false;
volatile bool initCompleted = false;
volatile byte rcvData = 0b0;
volatile unsigned short rcvCrc = 0;
volatile int bitIndex = 0;
char message[16];

unsigned short messageSize = 0;
int crcByteReceived = 0;
volatile int messageByteReceived = 0;
volatile bool readyForMessageSize = true;
volatile bool readyForMessage = false;
volatile bool readyForCrc = false;
volatile int crcState = 0;

byte tickChar[] = {
    0b00000,
    0b00001,
    0b00011,
    0b10110,
    0b11100,
    0b01000,
    0b00000,
    0b00000};

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

volatile bool preambleConcluded = false;

void onClockRising() {
  if (!preambleConcluded) {
    if (detectPreamble(TX_DATA)) {
      if (detectEnd(TX_DATA)) {
        preambleConcluded = true;
      }
    }
  } else {
    // process message
    int bitIn = digitalRead(TX_DATA);

    if (readyForMessageSize) {
      messageSize = messageSize | (bitIn << (15 - bitIndex));

      if (bitIndex == 15) {
        bitIndex = 0;
        readyForMessageSize = false;
        readyForMessage = true;
        byte data[2]; 
        shortToBytes(messageSize, data, 0);
        crc.updateCrc(data[0]);
        crc.updateCrc(data[1]);

        Serial.println(messageSize);
      } else {
        bitIndex++;
      }

    } else if (readyForMessage) {
      rcvData = rcvData | (bitIn << (7 - bitIndex));
      
      if (bitIndex == 7) {
        messageByteReceived++;
        strncat(message, (const char *)&rcvData, 1);
        crc.updateCrc(rcvData);
        bitIndex = 0;
        rcvData = 0b0;
      } else {
        bitIndex++;
      }
      if (messageByteReceived * 8 == messageSize) {
        readyForMessage = false;
        readyForCrc = true;
        messageByteReceived = 0;
      }
      updateLcd = true;
    } else if (readyForCrc) {
      rcvData = rcvData | (bitIn << (7 - bitIndex));

      if (bitIndex == 7) {
        crc.updateCrc(rcvData);
        bitIndex = 0;
        rcvData = 0b0;
        crcByteReceived++;
      } else {
        bitIndex++;
      }

      if (crcByteReceived == 2) {
        readyForCrc = false;
        updateLcd = true;

        if (crc.getCrc() == 0) {
          // CRC ok
          crcState = 1;
        } else {
          // CRC !ok
          crcState = 2;
        }
      }
    }
  }
}

// waitStartTransmission waits until the CLOCK is HIGH for 500 ms.
// This is the signal that the transmitter is ready to start
// transmitting data.
// void waitStartTransmission() {
//   int maxCount = 20;
//   while (!initCompleted) {
//     for (int i = 1; i <= maxCount; i++) {
//       int data = digitalRead(TX_CLOCK);
//       if (data == LOW) {
//         break;
//       }
//       delay(50);
//       if (i == maxCount) {
//         initCompleted = true;
//         Serial.println("Init completed");
//       }
//     }
//   }
// }

void setup() {
  // clear interrupt register, to make sure the handler does not get called
  // because of some garbage in the register
  EIFR = (1 << INTF1);
  attachInterrupt(digitalPinToInterrupt(TX_CLOCK), onClockRising, FALLING);
  pinMode(TX_CLOCK, INPUT);
  pinMode(TX_DATA, INPUT);

  Serial.begin(9600);
  while (!Serial);
  lcd.setLED1Pin(HIGH);
  lcd.createChar(0, tickChar);
  lcd.begin(16, 2);

  //waitStartTransmission();
}

void loop() {
  if (updateLcd) {
    updateLcd = false;
    lcd.setCursor(0, 0);
    lcd.print(message);
    Serial.println(message);
    lcd.setCursor(0, 1);

    for (int i = 0; i < 8; i += 1) {
      if (i < bitIndex) {
        lcd.print(rcvData & (0x80 >> i) ? "1" : "0");
      } else {
        lcd.print(" ");
      }
    }

    if (crcState == 1) {
      lcd.setCursor(15, 1);
      lcd.write(byte(0));
    } else if (crcState == 2) {
      lcd.setCursor(15, 1);
      lcd.print("x");
    }
  }
}