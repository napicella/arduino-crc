#include <Arduino.h>

volatile bool expected = true;
const int preambleBitsLength = 8;
volatile int preambleBitsToReceive = preambleBitsLength;

bool detectPreamble(int pin) {
  if (preambleBitsToReceive == 0) {
    return true;
  }

  bool data = digitalRead(pin);
  if (data == expected) {
    preambleBitsToReceive--;
    expected = !expected;
  } else if (data == HIGH) {
    preambleBitsToReceive = preambleBitsLength - 1;
    expected = false;
  } else if (data == LOW) {
    preambleBitsToReceive = preambleBitsLength;
    expected = true;
  }
  return false;
}

const int endPreambleBit = 5;
volatile int endPreambleBitToReceive = endPreambleBit;

bool detectEnd(int pin) {
  if (endPreambleBitToReceive == 0) {
    return true;
  }
  
  bool data = digitalRead(pin);
  if (data) {
    endPreambleBitToReceive--;
    return endPreambleBitToReceive == 0;
  } else {
    endPreambleBitToReceive = endPreambleBit;
    return false;
  }
}