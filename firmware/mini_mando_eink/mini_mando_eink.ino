// Mini Mando (e-ink version) - Spotify remote for iPhone
// Board: Seeed XIAO nRF52840 ("Seeed nRF52 Boards" core, not the mbed one)
// Display: 1.54" black/white e-paper, 200x200, SSD1681 controller
//          (WeAct Studio module, or any GDEY0154D67 / Waveshare 1.54" V2 module)
//
// Same Apple Media Service logic as the Sharp version. Only the screen changed.
// E-paper keeps its picture with zero power, so the screen is only refreshed
// when the song, artist or play/pause state changes, and sleeps in between.
//
// Wiring: VCC->3V3, GND->GND, SCL/CLK->D8, SDA/DIN->D10,
//         CS->D3, DC->D4, RES/RST->D5, BUSY->D6
//         buttons: D0 (previous), D1 (play/pause), D2 (next), other leg to GND
//
// Library: GxEPD2 (by Jean-Marc Zingg), it installs Adafruit GFX too
// First pairing: iPhone Settings > Bluetooth > "Mini Mando" > Pair.

#include <bluefruit.h>
#include <GxEPD2_BW.h>

#define DEVICE_NAME "Mini Mando"

#define PIN_CS    D3
#define PIN_DC    D4
#define PIN_RST   D5
#define PIN_BUSY  D6

#define BTN_PREV  D0
#define BTN_PLAY  D1
#define BTN_NEXT  D2

// Wait this long after the last change before refreshing, so that title, artist
// and play state (which arrive as separate messages) end up in a single refresh.
#define SETTLE_MS 300

// Every this many fast (partial) refreshes, do one full refresh with the black/white
// flash. This clears the faint "ghost" of older pictures.
#define PARTIALS_BEFORE_FULL 20

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY));

uint32_t lastChange = 0;

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

void drawScreen() {
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(true);

  if (!connected || !ready) {
    display.setTextSize(3);
    display.setCursor(10, 50);
    display.print("Mini");
    display.setCursor(10, 78);
    display.print("Mando");
    display.setTextSize(1);
    display.setCursor(10, 120);
    display.print(!connected ? "Waiting for iPhone" : paired ? "Paired, no media svc" : "Pairing...");
    display.setCursor(10, 140);
    display.print("Settings > Bluetooth");
    display.setCursor(10, 152);
    display.print("> " DEVICE_NAME);
    return;
  }

  // Status line
  display.setTextSize(1);
  display.setCursor(6, 6);
  display.print(playing ? "> Playing" : "|| Paused");
  display.drawFastHLine(0, 18, 200, GxEPD_BLACK);

  // Title: big letters, up to 5 lines of 16 characters
  display.setTextSize(2);
  display.setCursor(0, 28);
  if (title[0]) {
    char shortTitle[81];
    strncpy(shortTitle, title, 80);
    shortTitle[80] = '\0';
    display.print(shortTitle);
  } else {
    display.print("Nothing playing");
  }

  // Artist: small letters, up to 2 lines of 33 characters
  display.setTextSize(1);
  display.setCursor(0, 150);
  char shortArtist[67];
  strncpy(shortArtist, artist, 66);
  shortArtist[66] = '\0';
  display.print(shortArtist);

  // Buttons bar
  display.drawFastHLine(0, 176, 200, GxEPD_BLACK);
  display.setTextSize(2);
  display.setCursor(8, 182);
  display.print("|<<");
  display.setCursor(playing ? 88 : 94, 182);
  display.print(playing ? "||" : ">");
  display.setCursor(156, 182);
  display.print(">>|");
}

void refreshScreen() {
  static uint8_t partialCount = PARTIALS_BEFORE_FULL;  // first refresh is a full one

  if (partialCount >= PARTIALS_BEFORE_FULL) {
    display.setFullWindow();
    partialCount = 0;
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
    partialCount++;
  }

  display.firstPage();
  do {
    drawScreen();
  } while (display.nextPage());

  display.hibernate();  // panel uses ~0 power and keeps the picture until the next refresh
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

  char text[96];
  if (entity == ENTITY_TRACK && attribute == TRACK_TITLE) {
    toAscii(value, valueLen, text, sizeof(text));
    if (strcmp(text, title) == 0) return;
    strcpy(title, text);
  } else if (entity == ENTITY_TRACK && attribute == TRACK_ARTIST) {
    toAscii(value, valueLen, text, sizeof(text));
    if (strcmp(text, artist) == 0) return;
    strcpy(artist, text);
  } else if (entity == ENTITY_PLAYER && attribute == PLAYER_PLAYBACK_INFO) {
    // Value looks like "1,1.0,42.5": the first number is 0 = paused, 1 = playing.
    // The iPhone also sends this when only the position changes, so ignore repeats.
    bool nowPlaying = (valueLen > 0 && value[0] == '1');
    if (nowPlaying == playing) return;
    playing = nowPlaying;
  } else {
    return;
  }
  needsRedraw = true;
  lastChange = millis();
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
    if (down && !buttonWasDown[i] && ready) {
      amsRemoteCommand.write8_resp(buttonCommands[i]);
    }
    buttonWasDown[i] = down;
  }
}

// ---------- Main ----------

void setup() {
  for (int i = 0; i < 3; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  display.init(0);  // 0 = no debug output on Serial
  display.setRotation(0);  // change to 1, 2 or 3 if the picture is turned in the case

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

  // E-paper keeps its picture without power, so only refresh on real changes,
  // once the burst of updates from the iPhone has settled.
  if (needsRedraw && millis() - lastChange > SETTLE_MS) {
    needsRedraw = false;
    refreshScreen();  // takes ~0.3 s (partial) or ~2 s (full)
  }

  delay(20);  // simple debounce, and lets the chip rest between checks
}
