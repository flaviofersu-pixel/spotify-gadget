// Mini Mando - Spotify remote for iPhone
// Board: Seeed XIAO nRF52840 ("Seeed nRF52 Boards" core, not the mbed one)
//
// Uses Apple Media Service (AMS), which every iPhone has built in:
//   - reads the title, artist and play/pause state of whatever is playing (Spotify too)
//   - sends play/pause, next and previous back to the phone
// No app, no Spotify Premium and no Wi-Fi needed.
//
// Wiring: CLK->D8, DI->D10, CS->D3, VIN->3V3, GND->GND
//         buttons: D0 (previous), D1 (play/pause), D2 (next), other leg to GND
//
// First pairing: iPhone Settings > Bluetooth > "Mini Mando" > Pair.

#include <bluefruit.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#define DEVICE_NAME "Mini Mando"

// Cover picture: black cat with headphones, 60 x 60 pixels, 1 bit per pixel
// (1 = black). Made from the drawing Flavio picked, for Adafruit GFX drawBitmap().
#define CAT_W 60
#define CAT_H 60

const uint8_t catBitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x38, 0x00, 0x01, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x3C, 0x00, 0x03, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x3C, 0x00, 0x03, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x3E, 0x00, 0x07, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x3E, 0x00, 0x07, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x7E, 0x00, 0x0F, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x37, 0x01, 0x5D, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x3B, 0xAA, 0xAE, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x75, 0x55, 0x55, 0xC0, 0x00, 0x00,
  0x00, 0x20, 0x3A, 0xAA, 0xAA, 0xE0, 0x00, 0x00,
  0x00, 0x40, 0x75, 0x40, 0x75, 0xE0, 0x00, 0x00,
  0x00, 0x20, 0x6A, 0xC0, 0x7A, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x55, 0xFF, 0xFD, 0x40, 0x00, 0x00,
  0x00, 0x20, 0x2B, 0xFF, 0xFE, 0xA0, 0x00, 0x00,
  0x00, 0x10, 0x57, 0xFF, 0xFF, 0x40, 0x00, 0x00,
  0x00, 0x00, 0xBF, 0xFF, 0xFF, 0xA0, 0x00, 0x00,
  0x00, 0x00, 0x5F, 0xFF, 0xFF, 0xD0, 0x00, 0x00,
  0x02, 0x00, 0xBF, 0xFF, 0xFF, 0xA8, 0x00, 0x00,
  0x01, 0x41, 0x78, 0x7E, 0x0F, 0xD0, 0x00, 0x00,
  0x00, 0xA2, 0xF3, 0x3E, 0xE7, 0xA8, 0x00, 0x00,
  0x00, 0x01, 0x77, 0xBC, 0xE7, 0xD4, 0x00, 0x00,
  0x00, 0x02, 0xF7, 0xAC, 0xE7, 0xAA, 0x00, 0x00,
  0x00, 0x01, 0x73, 0x2E, 0xE7, 0x54, 0x00, 0x00,
  0x00, 0x0A, 0xF8, 0x6E, 0x0D, 0xAA, 0x00, 0x00,
  0x00, 0x51, 0x7F, 0xDF, 0xF9, 0x54, 0x00, 0x00,
  0x00, 0x82, 0xFB, 0xFF, 0xF7, 0xAA, 0x00, 0x00,
  0x01, 0x01, 0x7D, 0x7F, 0xEF, 0x54, 0x00, 0x00,
  0x02, 0x02, 0xBF, 0xFF, 0xFF, 0xAA, 0x00, 0x00,
  0x00, 0x01, 0x7F, 0xFF, 0xFF, 0x54, 0x00, 0x00,
  0x00, 0x00, 0x9F, 0xFF, 0xFE, 0xA8, 0x20, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFE, 0x03, 0xFC, 0x00,
  0x00, 0x00, 0x02, 0x7F, 0xF0, 0x07, 0xFE, 0x00,
  0x00, 0x00, 0x00, 0x7F, 0xC0, 0x1F, 0xCE, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xE0, 0x3E, 0x0E, 0x00,
  0x00, 0x00, 0x00, 0x7F, 0xE0, 0x3C, 0x7C, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xE0, 0x38, 0x3C, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xE0, 0x78, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xE0, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xF0, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xF0, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xF0, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xF0, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xF1, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xF8, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xF8, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xF8, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xF8, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFC, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFC, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFD, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x0F, 0xFF, 0xFF, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x3F, 0xE0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


#define PIN_SCK   D8
#define PIN_MOSI  D10
#define PIN_CS    D3

#define BTN_PREV  D0
#define BTN_PLAY  D1
#define BTN_NEXT  D2

#define BLACK 0
#define WHITE 1

Adafruit_SharpMem display(PIN_SCK, PIN_MOSI, PIN_CS, 144, 168);

// ---------- Apple Media Service (UUIDs stored least significant byte first) ----------

const uint8_t AMS_UUID_SERVICE[] = {
  0xDC, 0xF8, 0x55, 0xAD, 0x02, 0xC5, 0xF4, 0x8E, 0x3A, 0x43, 0x36, 0x0F, 0x2B, 0x50, 0xD3, 0x89
};
const uint8_t AMS_UUID_REMOTE_COMMAND[] = {
  0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8, 0x8A, 0x4A, 0xB1, 0x57, 0xD8, 0x81, 0x3C, 0x9B
};
const uint8_t AMS_UUID_ENTITY_UPDATE[] = {
  0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A, 0x1F, 0x41, 0x8D, 0x80, 0xCE, 0xAB, 0x7C, 0x2F
};

enum { CMD_PLAY = 0, CMD_PAUSE = 1, CMD_TOGGLE = 2, CMD_NEXT = 3, CMD_PREVIOUS = 4 };
enum { ENTITY_PLAYER = 0, ENTITY_QUEUE = 1, ENTITY_TRACK = 2 };
enum { PLAYER_PLAYBACK_INFO = 1 };
enum { TRACK_ARTIST = 0, TRACK_ALBUM = 1, TRACK_TITLE = 2, TRACK_DURATION = 3 };

BLEClientService        amsService(AMS_UUID_SERVICE);
BLEClientCharacteristic amsRemoteCommand(AMS_UUID_REMOTE_COMMAND);
BLEClientCharacteristic amsEntityUpdate(AMS_UUID_ENTITY_UPDATE);

// ---------- State shown on the screen ----------

char title[96]  = "";
char artist[96] = "";
char album[96]  = "";
bool playing    = false;
float durationSec   = 0;        // song length, from the iPhone
float elapsedSec    = 0;        // position when the iPhone last told us
float playbackRate  = 0;        // 1.0 while playing, 0 while paused
uint32_t elapsedAtMs = 0;       // millis() when elapsedSec was received
int batteryPercent = -1;        // -1 until the first reading
bool connected  = false;
bool ready      = false;          // paired and subscribed to AMS
bool paired     = false;          // link is encrypted, but media service not found yet
volatile bool needsRedraw = true;

uint16_t currentConn = BLE_CONN_HANDLE_INVALID;
bool amsFound = false;       // service and both characteristics found
uint8_t amsTries = 0;        // retries done from loop() after pairing
uint32_t lastAmsTry = 0;

// ---------- Text helpers ----------

// The built-in font has no accents, so turn common UTF-8 letters into plain ASCII.
void toAscii(const uint8_t* in, uint16_t len, char* out, size_t outSize) {
  size_t o = 0;
  for (uint16_t i = 0; i < len && o + 1 < outSize; i++) {
    uint8_t c = in[i];
    if (c < 0x80) { out[o++] = c; continue; }
    if (c == 0xC3 && i + 1 < len) {
      uint8_t d = in[++i];
      const char* map = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
      out[o++] = (d >= 0x80 && d <= 0xBF) ? map[d - 0x80] : '?';
      continue;
    }
    // Skip the rest of any other multi-byte character (emoji, other alphabets)
    while (i + 1 < len && (in[i + 1] & 0xC0) == 0x80) i++;
    out[o++] = '?';
  }
  out[o] = '\0';
}

// ---------- Drawing ----------

// The screen is mounted sideways (landscape), so the picture is 168 wide x 144 tall.
// If everything shows upside down, change 1 to 3.
#define DISPLAY_ROTATION 1

// Prints text in up to maxLines lines of maxChars characters, breaking between words.
// bold = draw it twice, 1 pixel apart, so it looks thicker.
void printWrapped(const char* text, int x, int y, int maxChars, int maxLines, bool bold) {
  int len = strlen(text);
  int start = 0;
  for (int line = 0; line < maxLines && start < len; line++) {
    int end = min(start + maxChars, len);
    if (end < len) {
      int space = end;
      while (space > start && text[space] != ' ') space--;
      if (space > start) end = space;
    }
    // Last line and still text left: finish with "..."
    char buf[40];
    int n = min(end - start, (int)sizeof(buf) - 1);
    memcpy(buf, text + start, n);
    buf[n] = '\0';
    if (line == maxLines - 1 && end < len && n >= 3) strcpy(buf + n - 3, "...");
    for (int b = 0; b < (bold ? 2 : 1); b++) {
      display.setCursor(x + b, y + line * 10);
      display.print(buf);
    }
    start = end;
    while (start < len && text[start] == ' ') start++;
  }
}

// "2:05" style time
void formatTime(int seconds, char* out) {
  if (seconds < 0) seconds = 0;
  sprintf(out, "%d:%02d", seconds / 60, seconds % 60);
}

float currentElapsed() {
  float t = elapsedSec + playbackRate * (millis() - elapsedAtMs) / 1000.0;
  if (durationSec > 0 && t > durationSec) t = durationSec;
  return t;
}

// ---------- Battery ----------
// The XIAO measures the battery through two resistors (1M and 510k) on PIN_VBAT.
// VBAT_ENABLE must stay LOW: that switches the measuring on, and Seeed warns that
// leaving it HIGH while charging can damage the pin.

void setupBattery() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  analogReference(AR_INTERNAL_2_4);   // measure 0..2.4 V
  analogReadResolution(12);           // 0..4095
}

float readBatteryVolts() {
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_VBAT);
  float pinVolts = (sum / 8.0) * 2.4 / 4095.0;
  return pinVolts * (1000.0 + 510.0) / 510.0;
}

// A LiPo is about 4.2 V when full and 3.3 V when empty, but not in a straight line
int voltsToPercent(float v) {
  const float volts[]   = { 3.30, 3.60, 3.70, 3.80, 3.90, 4.00, 4.20 };
  const int   percent[] = {    0,   10,   35,   60,   75,   85,  100 };
  if (v <= volts[0]) return 0;
  for (int i = 1; i < 7; i++) {
    if (v <= volts[i]) {
      return percent[i - 1] + (percent[i] - percent[i - 1]) * (v - volts[i - 1]) / (volts[i] - volts[i - 1]);
    }
  }
  return 100;
}

void updateBattery() {
  static uint32_t lastRead = 0;
  if (batteryPercent >= 0 && millis() - lastRead < 30000) return;  // every 30 seconds
  lastRead = millis();
  int p = voltsToPercent(readBatteryVolts());
  if (p != batteryPercent) {
    batteryPercent = p;
    needsRedraw = true;
  }
}

// Small battery icon, 18 x 8 px, filled in proportion to the charge
void drawBatteryIcon(int x, int y, uint16_t color) {
  display.drawRect(x, y, 16, 8, color);
  display.fillRect(x + 16, y + 2, 2, 4, color);
  if (batteryPercent > 0) display.fillRect(x + 2, y + 2, 12 * batteryPercent / 100, 4, color);
}

void drawPlayIcon(int x, int y, uint16_t color) {   // small triangle, 7 px tall
  display.fillTriangle(x, y, x, y + 6, x + 5, y + 3, color);
}

void drawPauseIcon(int x, int y, uint16_t color) {  // two small bars, 7 px tall
  display.fillRect(x, y, 2, 7, color);
  display.fillRect(x + 4, y, 2, 7, color);
}

// Square "album art" frame with the cat picture, since iPhones don't send the cover over Bluetooth
void drawArtBox(int x, int y, int size) {
  display.drawRect(x, y, size, size, BLACK);
  display.drawBitmap(x + (size - CAT_W) / 2, y + (size - CAT_H) / 2, catBitmap, CAT_W, CAT_H, BLACK);
}

// Bottom row with the three button icons, drawn as shapes so each sits exactly
// in the middle of its spot (they match the three physical buttons below the screen)
void drawButtonsBar() {
  int w = display.width();
  int y = display.height() - 27;
  int cy = y + 14;
  display.drawFastHLine(0, y, w, BLACK);

  // Previous |<
  int px = 18;
  display.fillRect(px - 7, cy - 6, 2, 13, BLACK);
  display.fillTriangle(px + 6, cy - 6, px + 6, cy + 6, px - 4, cy, BLACK);

  // Next >|
  int nx = w - 18;
  display.fillRect(nx + 5, cy - 6, 2, 13, BLACK);
  display.fillTriangle(nx - 6, cy - 6, nx - 6, cy + 6, nx + 4, cy, BLACK);

  // Middle: pause bars while playing, play triangle while paused
  int mx = w / 2;
  if (playing) {
    display.fillRect(mx - 6, cy - 7, 4, 15, BLACK);
    display.fillRect(mx + 2, cy - 7, 4, 15, BLACK);
  } else {
    display.fillTriangle(mx - 5, cy - 7, mx - 5, cy + 7, mx + 7, cy, BLACK);
  }
}

// iPod "Now Playing" screen
void drawNowPlaying() {
  int w = display.width();
  char buf[12];

  // Header: black bar with white text, like the iPod title bar
  display.fillRect(0, 0, w, 14, BLACK);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(4, 3);
  display.print("Now Playing");
  if (playing) drawPlayIcon(w - 12, 3, WHITE);
  else drawPauseIcon(w - 13, 3, WHITE);
  drawBatteryIcon(w - 38, 3, WHITE);
  display.setTextColor(BLACK);

  // Cover placeholder on the left, song info on the right
  drawArtBox(4, 22, 64);
  int tx = 74, chars = (w - tx) / 6;   // 15 characters fit next to the cover
  if (title[0]) printWrapped(title, tx, 26, chars, 3, true);
  else printWrapped("Nothing playing", tx, 26, chars, 3, true);
  printWrapped(artist, tx, 60, chars, 1, false);
  printWrapped(album, tx, 72, chars, 1, false);

  // Progress: elapsed time, bar, time left
  int barY = 91;
  float t = currentElapsed();
  formatTime((int)t, buf);
  display.setCursor(4, barY + 11);
  display.print(buf);
  char left[12] = "-";
  formatTime((int)(durationSec - t), left + 1);
  display.setCursor(w - 4 - 6 * strlen(left), barY + 11);
  display.print(left);

  display.drawRect(4, barY, w - 8, 8, BLACK);
  if (durationSec > 0) {
    int fill = (int)((w - 12) * t / durationSec);
    display.fillRect(6, barY + 2, fill, 4, BLACK);
  }

  drawButtonsBar();
}

void draw() {
  display.clearDisplayBuffer();  // clear only in memory, so the screen doesn't flash
  display.setTextColor(BLACK);
  display.setTextWrap(false);

  if (!connected || !ready) {
    display.setTextSize(2);
    display.setCursor(4, 30);
    display.print("Mini Mando");
    display.setTextSize(1);
    display.setCursor(4, 62);
    if (!connected) display.print("Waiting for iPhone");
    else if (!paired) display.print("Pairing...");
    else if (amsTries < 10) { display.print("Finding music... "); display.print(amsTries); }
    else display.print("No music service :(");
    display.setCursor(4, 80);
    display.print("Settings > Bluetooth");
    display.setCursor(4, 92);
    display.print("> " DEVICE_NAME);
    display.refresh();
    return;
  }

  drawNowPlaying();
  display.refresh();
}

// ---------- AMS callbacks ----------

// Called every time the iPhone changes song, artist or play/pause state.
// Data format: [entity id, attribute id, flags, value...]
void amsUpdateCallback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) chr;
  if (len < 3) return;
  uint8_t entity = data[0];
  uint8_t attribute = data[1];
  const uint8_t* value = data + 3;
  uint16_t valueLen = len - 3;

  if (entity == ENTITY_TRACK && attribute == TRACK_TITLE) {
    toAscii(value, valueLen, title, sizeof(title));
  } else if (entity == ENTITY_TRACK && attribute == TRACK_ARTIST) {
    toAscii(value, valueLen, artist, sizeof(artist));
  } else if (entity == ENTITY_TRACK && attribute == TRACK_ALBUM) {
    toAscii(value, valueLen, album, sizeof(album));
  } else if (entity == ENTITY_TRACK && attribute == TRACK_DURATION) {
    char buf[16];
    toAscii(value, valueLen, buf, sizeof(buf));
    durationSec = atof(buf);
  } else if (entity == ENTITY_PLAYER && attribute == PLAYER_PLAYBACK_INFO) {
    // Value looks like "1,1.0,42.5": state (0 = paused, 1 = playing), speed, seconds played
    char buf[32];
    toAscii(value, valueLen, buf, sizeof(buf));
    playing = (buf[0] == '1');
    char* rate = strchr(buf, ',');
    char* elapsed = rate ? strchr(rate + 1, ',') : NULL;
    playbackRate = rate ? atof(rate + 1) : 0;
    elapsedSec = elapsed ? atof(elapsed + 1) : 0;
    elapsedAtMs = millis();
  }
  needsRedraw = true;
}

void subscribeToAms() {
  if (!amsEntityUpdate.enableNotify()) return;

  uint8_t track[] = { ENTITY_TRACK, TRACK_ARTIST, TRACK_ALBUM, TRACK_TITLE, TRACK_DURATION };
  amsEntityUpdate.write_resp(track, sizeof(track));

  uint8_t player[] = { ENTITY_PLAYER, PLAYER_PLAYBACK_INFO };
  amsEntityUpdate.write_resp(player, sizeof(player));

  ready = true;
  needsRedraw = true;
}

// ---------- Bluetooth connection ----------

bool discoverAms(uint16_t connHandle) {
  amsFound = amsService.discover(connHandle) &&
             Bluefruit.Discovery.discoverCharacteristic(connHandle, amsRemoteCommand, amsEntityUpdate) == 2;
  return amsFound;
}

// After pairing the iPhone sometimes needs a moment before it shares its media
// service, so loop() keeps trying for about 20 seconds.
void retryAms() {
  if (!paired || ready || amsTries >= 10 || millis() - lastAmsTry < 2000) return;
  amsTries++;
  lastAmsTry = millis();
  if (amsFound || discoverAms(currentConn)) subscribeToAms();
  needsRedraw = true;
}

void connectCallback(uint16_t connHandle) {
  BLEConnection* conn = Bluefruit.Connection(connHandle);
  connected = true;
  needsRedraw = true;
  currentConn = connHandle;
  amsFound = false;
  amsTries = 0;

  conn->requestMtuExchange(247);  // lets long song titles arrive in one piece

  // Ask for pairing straight away: some iPhones only show the media service
  // after the link is encrypted, so we look for it again once it is secured.
  discoverAms(connHandle);
  conn->requestPairing();  // the iPhone shows a "Pair" popup the first time
}

void securedCallback(uint16_t connHandle) {
  BLEConnection* conn = Bluefruit.Connection(connHandle);
  if (!conn->secured()) {
    conn->requestPairing();
    return;
  }
  paired = true;
  needsRedraw = true;
  lastAmsTry = millis();
  if (amsFound || discoverAms(connHandle)) subscribeToAms();
}

void disconnectCallback(uint16_t connHandle, uint8_t reason) {
  (void) connHandle;
  (void) reason;
  connected = false;
  ready = false;
  album[0] = '\0';
  durationSec = 0;
  elapsedSec = 0;
  playbackRate = 0;
  paired = false;
  amsFound = false;
  currentConn = BLE_CONN_HANDLE_INVALID;
  title[0] = '\0';
  artist[0] = '\0';
  needsRedraw = true;
}

void startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(amsService);  // tells iOS we want its media service
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 1636);  // fast at first, then slow (~1 s) to save battery
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

// ---------- Buttons ----------

const uint8_t buttonPins[3]     = { BTN_PREV, BTN_PLAY, BTN_NEXT };
const uint8_t buttonCommands[3] = { CMD_PREVIOUS, CMD_TOGGLE, CMD_NEXT };
bool buttonWasDown[3] = { false, false, false };

void checkButtons() {
  for (int i = 0; i < 3; i++) {
    bool down = digitalRead(buttonPins[i]) == LOW;
    if (down && !buttonWasDown[i] && ready) {
      amsRemoteCommand.write8_resp(buttonCommands[i]);
    }
    buttonWasDown[i] = down;
  }
}

// ---------- Main ----------

void setup() {
  for (int i = 0; i < 3; i++) pinMode(buttonPins[i], INPUT_PULLUP);
  setupBattery();

  display.begin();
  display.setRotation(DISPLAY_ROTATION);
  display.clearDisplay();

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(0);
  Bluefruit.setName(DEVICE_NAME);
  Bluefruit.Periph.setConnectCallback(connectCallback);
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);
  Bluefruit.Security.setSecuredCallback(securedCallback);

  amsService.begin();
  amsRemoteCommand.begin();
  amsEntityUpdate.setNotifyCallback(amsUpdateCallback);
  amsEntityUpdate.begin();

  startAdvertising();
}

void loop() {
  checkButtons();
  retryAms();
  updateBattery();

  // Redraw on changes, and at least once a second: the Sharp panel needs a regular
  // refresh (VCOM toggle) to stay healthy when the picture doesn't change.
  static uint32_t lastRefresh = 0;
  if (needsRedraw) {
    needsRedraw = false;
    draw();
    lastRefresh = millis();
  } else if (millis() - lastRefresh > 1000) {
    if (ready && playing) draw();   // moves the time and progress bar
    else display.refresh();
    lastRefresh = millis();
  }

  delay(20);  // simple debounce, and lets the chip rest between checks
}
