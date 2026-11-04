// Mini Mando - hardware test (no Bluetooth)
// Checks the Sharp Memory display and the three buttons on the breadboard.
// Wiring: CLK->D8, DI->D10, CS->D3, VIN->3V3, GND->GND
//         buttons: D0 (previous), D1 (play/pause), D2 (next), other leg to GND

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#define PIN_SCK   D8
#define PIN_MOSI  D10
#define PIN_CS    D3

#define BTN_PREV  D0
#define BTN_PLAY  D1
#define BTN_NEXT  D2

#define BLACK 0
#define WHITE 1

Adafruit_SharpMem display(PIN_SCK, PIN_MOSI, PIN_CS, 144, 168);

const uint8_t buttons[3] = { BTN_PREV, BTN_PLAY, BTN_NEXT };
const char* names[3] = { "PREVIOUS", "PLAY/PAUSE", "NEXT" };
uint16_t presses[3] = { 0, 0, 0 };
bool wasDown[3] = { false, false, false };

void draw(int last) {
  display.clearDisplayBuffer();  // clear only in memory, so the screen doesn't flash
  display.setTextColor(BLACK);

  display.setTextSize(2);
  display.setCursor(4, 4);
  display.print("HW test");
  display.drawFastHLine(0, 24, 144, BLACK);

  display.setTextSize(1);
  for (int i = 0; i < 3; i++) {
    display.setCursor(4, 36 + i * 16);
    display.print(names[i]);
    display.setCursor(110, 36 + i * 16);
    display.print(presses[i]);
  }

  display.drawFastHLine(0, 90, 144, BLACK);
  display.setCursor(4, 100);
  display.print("Last button:");
  display.setTextSize(2);
  display.setCursor(4, 116);
  display.print(last < 0 ? "-" : names[last]);

  // Checkerboard corner: if it shows, every pixel row is being drawn
  for (int y = 150; y < 168; y += 2)
    for (int x = 0; x < 144; x += 2)
      display.drawPixel(x + ((y / 2) % 2), y, BLACK);

  display.refresh();
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) pinMode(buttons[i], INPUT_PULLUP);
  display.begin();
  draw(-1);
}

void loop() {
  for (int i = 0; i < 3; i++) {
    bool down = digitalRead(buttons[i]) == LOW;
    if (down && !wasDown[i]) {
      presses[i]++;
      Serial.print("Pressed: ");
      Serial.println(names[i]);
      draw(i);
    }
    wasDown[i] = down;
  }
  delay(20);  // simple debounce
}
