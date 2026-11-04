// Mini Mando (Spotify remote) - Wokwi prototype
// Same wiring as the real build:
//   Sharp display: VIN->3V3, GND->GND, CLK->D8, DI->D10, CS->D3
//   Buttons: D0 = previous, D1 = play/pause, D2 = next (other leg to GND)
// The songs are fake: Bluetooth comes in the real firmware (firmware/mini_mando).

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#ifndef IRAM_ATTR
#define IRAM_ATTR  // not needed on the XIAO nRF52840
#endif

#define BLACK 0
#define WHITE 1

// Same constructor as the real firmware
Adafruit_SharpMem display(D8, D10, D3, 144, 168);

const int PIN_PREV = D0;
const int PIN_PLAY = D1;
const int PIN_NEXT = D2;

struct Song {
  const char* title;
  const char* artist;
  int duration;  // in seconds
};

Song playlist[] = {
  { "Blinding Lights", "The Weeknd", 200 },
  { "Despacito", "Luis Fonsi", 229 },
  { "Shape of You", "Ed Sheeran", 233 },
  { "Flowers", "Miley Cyrus", 200 },
  { "Tusa", "Karol G", 201 },
};
const int TOTAL = sizeof(playlist) / sizeof(playlist[0]);

int current = 0;          // index of the current song
bool playing = true;     // true = playing, false = paused
int second = 0;         // playback position in seconds
unsigned long lastTick = 0;

// Buttons use interrupts, so no press is lost
// while the display is redrawing.
const int pins[3] = { PIN_PREV, PIN_PLAY, PIN_NEXT };
volatile bool pressed[3] = { false, false, false };
volatile unsigned long lastPress[3] = { 0, 0, 0 };

void IRAM_ATTR markPress(int i) {
  unsigned long now = millis();
  if (now - lastPress[i] > 150) {  // debounce
    pressed[i] = true;
    lastPress[i] = now;
  }
}
void IRAM_ATTR onPrevPressed() { markPress(0); }
void IRAM_ATTR onPlayPressed() { markPress(1); }
void IRAM_ATTR onNextPressed() { markPress(2); }

bool wasPressed(int i) {
  if (pressed[i]) {
    pressed[i] = false;
    return true;
  }
  return false;
}

void printTime(int x, int y, int s) {
  display.setCursor(x, y);
  display.print(s / 60);
  display.print(':');
  if (s % 60 < 10) display.print('0');
  display.print(s % 60);
}

// Prints centered text; falls back to the small font if it is too long
void printCentered(const char* text, int y, int size) {
  if (strlen(text) * 6 * size > 144) size = 1;
  display.setTextSize(size);
  int width = strlen(text) * 6 * size;
  display.setCursor(max(0, (144 - width) / 2), y);
  display.print(text);
}

void drawDisc(int cx, int cy) {
  display.fillCircle(cx, cy, 26, BLACK);
  display.drawCircle(cx, cy, 20, WHITE);
  display.drawCircle(cx, cy, 14, WHITE);
  display.fillCircle(cx, cy, 6, WHITE);
  display.fillCircle(cx, cy, 2, BLACK);
  // A dot that spins while the song is playing
  float angle = second * 0.8;
  int mx = cx + cos(angle) * 23;
  int my = cy + sin(angle) * 23;
  display.fillCircle(mx, my, 2, WHITE);
}

void drawButtons(int y) {
  // Previous |<<
  display.fillRect(22, y - 6, 3, 13, BLACK);
  display.fillTriangle(34, y - 6, 34, y + 6, 26, y, BLACK);
  // Play or pause, inside a circle
  display.drawCircle(72, y, 12, BLACK);
  if (playing) {
    display.fillRect(66, y - 6, 4, 13, BLACK);
    display.fillRect(74, y - 6, 4, 13, BLACK);
  } else {
    display.fillTriangle(68, y - 7, 68, y + 7, 79, y, BLACK);
  }
  // Next >>|
  display.fillTriangle(110, y - 6, 110, y + 6, 118, y, BLACK);
  display.fillRect(119, y - 6, 3, 13, BLACK);
}

void draw() {
  Song& c = playlist[current];
  display.clearDisplayBuffer();
  display.setTextColor(BLACK);
  display.setTextWrap(false);

  // Top bar (black with white text)
  display.fillRect(0, 0, 144, 14, BLACK);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print("Spotify");
  display.setCursor(98, 3);
  display.print(current + 1);
  display.print(" of ");
  display.print(TOTAL);
  display.setTextColor(BLACK);

  drawDisc(72, 46);

  printCentered(c.title, 80, 2);
  printCentered(c.artist, 99, 1);

  // Progress bar
  display.drawRect(4, 113, 136, 7, BLACK);
  int fill = (long)134 * second / c.duration;
  display.fillRect(5, 114, fill, 5, BLACK);
  display.setTextSize(1);
  printTime(4, 124, second);
  printTime(110, 124, c.duration);

  drawButtons(150);

  display.refresh();
}

void changeSong(int step) {
  current = (current + step + TOTAL) % TOTAL;
  second = 0;
  Serial.print("Song: ");
  Serial.println(playlist[current].title);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) pinMode(pins[i], INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_PREV), onPrevPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_PLAY), onPlayPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_NEXT), onNextPressed, FALLING);

  display.begin();
  display.clearDisplay();
  draw();
  Serial.println("Remote ready");
}

void loop() {
  bool changed = false;

  if (wasPressed(0)) {
    Serial.println("Button PREVIOUS (D0)");
    // Like Spotify: after 3 s, previous restarts the current song
    if (second > 3) second = 0;
    else changeSong(-1);
    changed = true;
  }
  if (wasPressed(1)) {
    playing = !playing;
    Serial.println(playing ? "Button PLAY (D1): playing" : "Button PLAY (D1): paused");
    changed = true;
  }
  if (wasPressed(2)) {
    Serial.println("Button NEXT (D2)");
    changeSong(1);
    changed = true;
  }

  // Advance one second while playing
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    if (playing) {
      second++;
      if (second >= playlist[current].duration) changeSong(1);
      changed = true;
    }
  }

  if (changed) draw();
  delay(10);
}
