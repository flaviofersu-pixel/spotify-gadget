# Wokwi simulation

Wokwi has neither the XIAO nRF52840 nor the Sharp Memory LCD, so the simulation uses:

- **XIAO ESP32-C3**, which has the same footprint and pin names (D0-D10, 3V3, GND, 5V).
- A **custom chip** (`sharp-lcd.chip.c`) that imitates the Sharp display: same 9 pins (VIN, 3V3, GND, CLK, DI, CS, EMD, DISP, EIN), it decodes the Adafruit_SharpMem SPI stream and draws it.

The sketch uses fake songs (no Bluetooth) to test the UI and the three buttons. Wiring matches [docs/wiring.md](../docs/wiring.md).

## Browser (`wokwi-web/`)

1. Sign in at https://wokwi.com (the custom chip needs an account) and open https://wokwi.com/projects/new/esp32.
2. Click **+** > **Custom Chip**, name it `sharp-lcd`, language **C**. Paste `sharp-lcd.chip.json` and `sharp-lcd.chip.c` into the two new tabs.
3. Paste `sketch.ino` into the sketch tab.
4. Create a file `libraries.txt` and paste its content.
5. Paste `diagram.json` last (it replaces the whole diagram) and press **Play**.

Click the buttons, or use left arrow / space / right arrow. The Serial Monitor logs every press.

## VS Code + PlatformIO (`wokwi-vscode/`)

1. Install the **PlatformIO IDE** and **Wokwi Simulator** extensions, then run `Wokwi: Request a New License` (free).
2. Open the `wokwi-vscode` folder.
3. Build with PlatformIO (the check mark in the status bar).
4. Open `diagram.json` and press **Play**.

The custom chip is already compiled (`chips/sharp-lcd.chip.wasm`). Rebuild after every change to `src/main.cpp`.
