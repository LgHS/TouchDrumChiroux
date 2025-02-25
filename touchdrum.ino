#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <usb_midi.h>

Adafruit_MPR121 mpr1 = Adafruit_MPR121();
Adafruit_MPR121 mpr2 = Adafruit_MPR121();

const int numPositions = 8;
const int pins[numPositions] = {26, 27, 28, 29, 30, 31, 32, 33};

int lastStablePosition = -1;  
int currentPosition = -1;     
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 50;  
int noteOffset = 0; 

const int mappedKeys1[8] = {0, 2, 1, 3, 4, 5, 6, 7};
const int mappedKeys2[8] = {0, 1, 2, 3, 4, 5, 6, 7};

const int firstNote = 0; 
uint16_t lastTouch1 = 0, lastTouch2 = 0;
const int midiChannel = 1; 

void setup() {
  Serial.begin(115200);
  usbMIDI.begin();
  Wire.begin();       
  Wire1.begin();      
  for (int i = 0; i < numPositions; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
  if (!mpr1.begin(0x5A, &Wire)) {
    Serial.println("MPR121 #1 non détecté !");
  }
  if (!mpr2.begin(0x5A, &Wire1)) {
    Serial.println("MPR121 #2 non détecté !");
  }
}

void loop() {
  handleRotary();  
  handleTouch();   
  usbMIDI.read();  
}

void handleRotary() {
  int detectedPosition = -1;
  for (int i = 0; i < numPositions; i++) {
    if (digitalRead(pins[i]) == LOW) {
      detectedPosition = i;
      break;  
    }
  }

  if (detectedPosition != currentPosition) {
    currentPosition = detectedPosition;
    lastChangeTime = millis();
  }

  if ((millis() - lastChangeTime) > debounceDelay && currentPosition != lastStablePosition) {
    lastStablePosition = currentPosition;
    if (lastStablePosition >= 0) {
      noteOffset = lastStablePosition * 16;
      usbMIDI.sendControlChange(123, 0, midiChannel);
      Serial.print("Rotatif: ");
      Serial.print(lastStablePosition + 1);
      Serial.print(" | Notes: ");
      Serial.println(noteOffset);
    }
  }
}

void handleTouch() {
  uint16_t touch1 = mpr1.touched();
  uint16_t touch2 = mpr2.touched();
  for (int i = 0; i < 8; i++) {
    bool isTouched1 = touch1 & (1 << mappedKeys1[i]);
    bool wasTouched1 = lastTouch1 & (1 << mappedKeys1[i]);
    if (isTouched1 && !wasTouched1) {
      usbMIDI.sendNoteOn(firstNote + noteOffset + i, 127, midiChannel);
    } else if (!isTouched1 && wasTouched1) {
      usbMIDI.sendNoteOff(firstNote + noteOffset + i, 0, midiChannel);
    }
  }
  for (int i = 0; i < 8; i++) {
    bool isTouched2 = touch2 & (1 << mappedKeys2[i]);
    bool wasTouched2 = lastTouch2 & (1 << mappedKeys2[i]);
    if (isTouched2 && !wasTouched2) {
      int note = firstNote + noteOffset + 8 + i;
      usbMIDI.sendNoteOn(note, 127, midiChannel);
    } else if (!isTouched2 && wasTouched2) {
      int note = firstNote + noteOffset + 8 + i;
      usbMIDI.sendNoteOff(note, 0, midiChannel);
    }
  }
  lastTouch1 = touch1;
  lastTouch2 = touch2;
}
