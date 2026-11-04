// Fake Sharp Memory LCD 144x168 custom chip for Wokwi.
// Reads the bits that the Adafruit_SharpMem library sends on CLK/DI/CS
// and draws them on the simulator framebuffer.

#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>

#define WIDTH 144
#define HEIGHT 168
#define BYTES_PER_LINE (WIDTH / 8)

typedef enum { ST_CMD, ST_ADDR, ST_DATA, ST_TRAILER, ST_IGNORE } state_t;

typedef struct {
  pin_t cs, clk, di;
  buffer_t fb;
  uint32_t width, height;
  bool active;
  bool reversed;
  uint8_t bits, nbits;
  state_t state;
  int line, index;
  uint8_t data[BYTES_PER_LINE];
  uint8_t row[WIDTH * 4];
} chip_state_t;

static chip_state_t state;  // there is only one display

// Colors close to the real panel: silver grey and near black
static const uint8_t ON_RGBA[4] = { 0xC9, 0xCD, 0xC3, 0xFF };
static const uint8_t OFF_RGBA[4] = { 0x1E, 0x22, 0x20, 0xFF };

static uint8_t reverse_bits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

static void clear_screen(chip_state_t *chip) {
  for (int x = 0; x < WIDTH; x++) {
    for (int c = 0; c < 4; c++) chip->row[x * 4 + c] = ON_RGBA[c];
  }
  for (int y = 0; y < HEIGHT; y++) {
    buffer_write(chip->fb, y * WIDTH * 4, chip->row, sizeof(chip->row));
  }
}

static void draw_line(chip_state_t *chip) {
  for (int x = 0; x < WIDTH; x++) {
    bool on = (chip->data[x / 8] >> (x % 8)) & 1;
    const uint8_t *color = on ? ON_RGBA : OFF_RGBA;
    for (int c = 0; c < 4; c++) chip->row[x * 4 + c] = color[c];
  }
  buffer_write(chip->fb, chip->line * WIDTH * 4, chip->row, sizeof(chip->row));
}

static void process_byte(chip_state_t *chip, uint8_t b) {
  if (chip->state == ST_CMD && (b & 0x05) == 0 && (reverse_bits(b) & 0x05) != 0) {
    chip->reversed = true;
  }
  if (chip->reversed) b = reverse_bits(b);

  switch (chip->state) {
    case ST_CMD:
      if (b & 0x04) {
        clear_screen(chip);
        chip->state = ST_IGNORE;
      } else if (b & 0x01) {
        chip->state = ST_ADDR;
      } else {
        chip->state = ST_IGNORE;
      }
      break;
    case ST_ADDR:
      if (b >= 1 && b <= HEIGHT) {
        chip->line = b - 1;
        chip->index = 0;
        chip->state = ST_DATA;
      } else {
        chip->state = ST_IGNORE;
      }
      break;
    case ST_DATA:
      chip->data[chip->index++] = b;
      if (chip->index == BYTES_PER_LINE) {
        draw_line(chip);
        chip->state = ST_TRAILER;
      }
      break;
    case ST_TRAILER:
      chip->state = ST_ADDR;
      break;
    case ST_IGNORE:
      break;
  }
}

// On this display CS is active HIGH, unlike most SPI devices
static void on_cs_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->active = (value == HIGH);
  chip->state = ST_CMD;
  chip->reversed = false;
  chip->bits = 0;
  chip->nbits = 0;
}

// Each rising CLK edge brings one bit on DI, least significant bit first
static void on_clk_rise(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (!chip->active) return;
  if (pin_read(chip->di)) chip->bits |= (1 << chip->nbits);
  chip->nbits++;
  if (chip->nbits == 8) {
    process_byte(chip, chip->bits);
    chip->bits = 0;
    chip->nbits = 0;
  }
}

void chip_init(void) {
  chip_state_t *chip = &state;
  chip->cs = pin_init("CS", INPUT);
  chip->clk = pin_init("CLK", INPUT);
  chip->di = pin_init("DI", INPUT);
  chip->fb = framebuffer_init(&chip->width, &chip->height);
  clear_screen(chip);

  const pin_watch_config_t cs_watch = {
    .edge = BOTH,
    .pin_change = on_cs_change,
    .user_data = chip,
  };
  pin_watch(chip->cs, &cs_watch);

  const pin_watch_config_t clk_watch = {
    .edge = RISING,
    .pin_change = on_clk_rise,
    .user_data = chip,
  };
  pin_watch(chip->clk, &clk_watch);
}
