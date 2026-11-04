// Mini Mando (cheap OLED version) - Spotify remote for iPhone
// Board: Seeed XIAO nRF52840 ("Seeed nRF52 Boards" core, not the mbed one)
// Display: 0.96" SSD1306 OLED, 128x64, I2C (the common 4-pin module)
//
// Same Apple Media Service logic as the Sharp version. Only the screen changed.
// An OLED uses much more power than the Sharp memory LCD, so the screen turns
// itself off after SCREEN_TIMEOUT_MS and wakes on any button press or song change.
//
// Wiring: OLED SDA->D4, SCL->D5, VCC->3V3, GND->GND (check VCC/GND order on your board!)
//         buttons: D0 (previous), D1 (play/pause), D2 (next), other leg to GND
//
// Libraries: Adafruit GFX, Adafruit SSD1306
// First pairing: iPhone Settings > Bluetooth > "Mini Mando" > Pair.

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DEVICE_NAME "Mini Mando"

#define OLED_ADDRESS 0x3C   // most modules; some use 0x3D
#define SCREEN_TIMEOUT_MS 15000

#define BTN_PREV  D0
#define BTN_PLAY  D1
#define BTN_NEXT  D2

Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool screenOn = true;
uint32_t lastActivity = 0;

void wakeScreen() {
  lastActivity = millis();
  if (!screenOn) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    screenOn = true;
  }
}

void sleepScreen() {
  if (screenOn) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);  // panel draws only a few uA when off
    screenOn = false;
  }
}

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
enum { TRACK_ARTIST = 0, TRACK_TITLE = 2 };

BLEClientService        amsService(AMS_UUID_SERVICE);
BLEClientCharacteristic amsRemoteCommand(AMS_UUID_REMOTE_COMMAND);
BLEClientCharacteristic amsEntityUpdate(AMS_UUID_ENTITY_UPDATE);

// ---------- State shown on the screen ----------

char title[96]  = "";
char artist[96] = "";
bool playing    = false;
bool connected  = false;
bool ready      = false;          // paired and subscribed to AMS
bool paired     = false;          // link is encrypted, but media service not found yet
volatile bool needsRedraw = true;

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

void drawButtonsBar() {
  display.drawFastHLine(0, 53, 128, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(4, 56);
  display.print("|<<");
  display.setCursor(58, 56);
  display.print(playing ? "||" : ">");
  display.setCursor(106, 56);
  display.print(">>|");
}

void draw() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);
  display.setTextSize(1);

  if (!connected || !ready) {
    display.setTextSize(2);
    display.setCursor(4, 0);
    display.print("Mini Mando");
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print(!connected ? "Waiting for iPhone" : paired ? "Paired, no media svc" : "Pairing...");
    display.setCursor(0, 40);
    display.print("Bluetooth > " DEVICE_NAME);
    display.display();
    return;
  }

  // Status line
  display.setCursor(0, 0);
  display.print(playing ? "> Playing" : "|| Paused");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // Title: up to 3 lines of 21 characters
  display.setCursor(0, 12);
  if (title[0]) {
    char shortTitle[64];
    strncpy(shortTitle, title, 63);
    shortTitle[63] = '\0';
    display.print(shortTitle);
  } else {
    display.print("Nothing playing");
  }

  // Artist: one line, no wrap
  display.setTextWrap(false);
  display.setCursor(0, 42);
  char shortArtist[22];
  strncpy(shortArtist, artist, 21);
  shortArtist[21] = '\0';
  display.print(shortArtist);

  drawButtonsBar();
  display.display();
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
  } else if (entity == ENTITY_PLAYER && attribute == PLAYER_PLAYBACK_INFO) {
    // Value looks like "1,1.0,42.5": the first number is 0 = paused, 1 = playing
    playing = (valueLen > 0 && value[0] == '1');
  }
  needsRedraw = true;
  wakeScreen();  // show the new song
}

void subscribeToAms() {
  if (!amsEntityUpdate.enableNotify()) return;

  uint8_t track[] = { ENTITY_TRACK, TRACK_ARTIST, TRACK_TITLE };
  amsEntityUpdate.write_resp(track, sizeof(track));

  uint8_t player[] = { ENTITY_PLAYER, PLAYER_PLAYBACK_INFO };
  amsEntityUpdate.write_resp(player, sizeof(player));

  ready = true;
  needsRedraw = true;
}

// ---------- Bluetooth connection ----------

bool discoverAms(uint16_t connHandle) {
  if (!amsService.discover(connHandle)) return false;
  return Bluefruit.Discovery.discoverCharacteristic(connHandle, amsRemoteCommand, amsEntityUpdate) == 2;
}

void connectCallback(uint16_t connHandle) {
  BLEConnection* conn = Bluefruit.Connection(connHandle);
  connected = true;
  needsRedraw = true;

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
  if (amsService.discovered() || discoverAms(connHandle)) {
    subscribeToAms();
  }
}

void disconnectCallback(uint16_t connHandle, uint8_t reason) {
  (void) connHandle;
  (void) reason;
  connected = false;
  ready = false;
  paired = false;
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
    if (down && !buttonWasDown[i]) {
      wakeScreen();
      if (ready) amsRemoteCommand.write8_resp(buttonCommands[i]);
    }
    buttonWasDown[i] = down;
  }
}

// ---------- Main ----------

void setup() {
  for (int i = 0; i < 3; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.setContrast(0x40);  // dimmer = less power, still readable indoors
  display.clearDisplay();
  display.display();
  lastActivity = millis();

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

  // The OLED keeps its picture by itself, so only redraw when something changed.
  if (needsRedraw) {
    needsRedraw = false;
    draw();
  }

  if (screenOn && millis() - lastActivity > SCREEN_TIMEOUT_MS) {
    sleepScreen();
  }

  delay(20);  // simple debounce, and lets the chip rest between checks
}
