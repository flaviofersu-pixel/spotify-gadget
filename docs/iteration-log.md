# Iteration log

Everything I tried, in order, with what went wrong and what changed.

## 0. Idea and part selection (Oct 28)

**Goal:** a small, light screen next to the camera of my iPhone 16 Pro (the free space there is about 26 x 38 mm) that shows the current Spotify song and has play/pause and skip buttons. It should last a long time on one charge.

**Options considered:**

| Option | Verdict |
|---|---|
| ESP32 + Wi-Fi + Spotify Web API | Rejected: battery would last hours, needs Premium and a hotspot |
| nRF52840 + BLE + Apple Media Service | **Chosen**: built into iOS, no app, weeks of battery |
| OLED screen | Rejected for the main build: higher power, board too wide |
| Sharp Memory LCD | **Chosen**: reflective like an e-reader, near-zero power |

**Decisions:**
- Mount with VHB tape, **not magnets** (Apple warns magnets can affect the camera stabilizer).
- Buy two XIAO boards: one with pre-soldered headers for the breadboard, one without for inside the case.
- Start with the Adafruit Sharp breakout (40 x 39 mm, easy to wire) and move to the bare panel for the slim version later.

**Output:** [`hardware/case/v0-size-test`](../hardware/case/v0-size-test), a solid 26 x 38 x 9 mm block to check the size on the phone. Plus 3D renders of the idea in [`media/renders`](../media/renders).

## 1. Simulation (Oct 29)

Wokwi has no XIAO nRF52840 and no Sharp display, so:
- Used the XIAO ESP32-C3 (same footprint and pin names).
- Wrote a **Wokwi custom chip** in C that reads the Sharp SPI protocol and draws the framebuffer.

The simulated firmware uses fake songs to test the UI and the buttons. See [`simulation/`](../simulation).

## 2. Case v1 (Oct 30)

First hollow case for the Adafruit breakout: base + snap-on lid, 47 x 57 x 12.5 mm. Display on 4 posts, window of 27 x 25 mm, battery and XIAO underneath, USB-C opening on the side.

**Problems:** the recesses around the button holes were on the face that touches the print bed, so they printed in mid-air and came out stringy.

I also measured the print from a photo, using my Xteink X3 e-reader (63.7 x 97.6 mm) as the scale: the print was within 1 mm of the design. See [`media/photos/case-v1-dimensions.png`](../media/photos/case-v1-dimensions.png).

**Realisation:** this case cannot fit next to the camera. The display board alone (40 mm) is wider than the gap (26 mm). v1 to v7 are the **desk prototype**; the slim version needs the bare panel.

## 3. Case v2 (Oct 30)

- Straight 4.2 mm button holes, no recess.
- Lid lip from 1.5 mm to 3 mm tall with tighter tolerance, because the lid was too loose.

## 4. First working hardware (Oct 31)

- `firmware/hardware_test` on the breadboard: display worked first time.
- **Buttons did not work**: the breadboard's GND rail was not connected to the XIAO's GND. One jumper fixed it.
- **Previous button still dead**: the tactile switch was rotated 90 degrees. Re-seating it fixed it.
- **Compile error "Python was not found"**: the Seeed nRF52 core needs Python on Windows. Installed Python with "Add to PATH" and disabled the Microsoft Store alias.
- **iPhone did not show the device** in Settings > Bluetooth. Connecting once with nRF Connect triggered the pairing prompt. The firmware was changed to always request pairing.
- **Result: the screen shows the song and the buttons control Spotify.** 🎉

## 5. Case v3 and v4 (Oct 31)

- **v3:** the lid was thicker than the button height, so the buttons were hidden. Added 6.4 x 6.4 mm square pockets under the lid with a 0.6 mm skin on top.
- **v4:** buttons now stick out through square openings like a real remote. Bigger **12 x 12 mm button for play/pause**, 6 x 6 mm for previous/next. Small ledges hold each switch. An alternative lid without ledges (`v4b`) is included.

## 6. Case v5 and v6 + firmware polish (Nov 1)

- **v5:** display mounted landscape, with L-shaped corner holders. The exact board dimensions (40 x 39 mm, holes 3 mm from each edge) came from Adafruit's published design files.
- **v6:** I pointed out that square pillars made no sense for round holes. The posts became **round with 2.6 mm pins** that go into the board's 3 mm holes, so the board centers itself.
- **Firmware:**
  - "Paired, no media svc" bug: the iPhone sometimes offers AMS a few seconds after pairing. The firmware now retries every 2 s for about 20 s.
  - Play/pause icons are drawn as shapes instead of characters, so they are centered.
  - **iPod-style UI**: Now Playing bar, bold title, artist, album, progress bar with elapsed/remaining time, button row at the bottom.
  - AMS only sends text, so there is no album art. Instead a **60 x 60 pixel-art cat with headphones** sits where the cover would be.
  - **Battery icon** using the XIAO's built-in voltage divider on `PIN_VBAT`.

## 7. Case v7 (Nov 2)

Everything snaps in, no glue or tape:
- **XIAO cradle**: rails, side locators and a flexing clip with a 0.6 mm lip. Insert USB-C first, push down until it clicks.
- **Battery pocket** with fences and small bumps, plus a gap for the wires.
- **USB-C opening** sized to the plug (9.6 x 3.9 mm stadium) instead of a big rectangle.
- **Corner radius** from 5 to 8 mm.
- **Printable button caps** with recessed icons, printed face down, press-fit on the switch plungers.
- Generated with a Python script ([`prototype_case_v7.py`](../hardware/case/v7/prototype_case_v7.py)) using `manifold3d`, plus documentation images ([top view](case-v7-top-view.png), [wiring](case-v7-wiring.png)).

## 8. Soldering problems and PCB v0.1 (Nov 3-4)

Soldering the buttons to the XIAO **inside the case** was almost impossible: breadboard jumper wires are thick and stiff, and I had already glued the XIAO to the base.

What I learned about how small devices are really built:
- Thin, soft silicone wire (28-30 AWG), flux and a fine tip.
- Solder everything on the table, test, then fold it into the case.
- Commercial devices use a **PCB instead of wires**.

So I designed [**PCB v0.1**](../hardware/pcb/v0.1): 43 x 53 mm, 2 layers, the XIAO soldered on top with USB-C on the edge, a window underneath for the battery pads, the 3 switches in through-holes and a 9-pin header for the display, held with M2.5 screws. The only wires left are the battery's.

- Generated with a Python script that writes Gerber + Excellon files and runs a clearance check.
- Estimated cost: about $2 for 5 boards at JLCPCB plus shipping, or AISLER (Germany) for a faster EU option.
- **Before ordering:** print the 1:1 template at 100 %, check the 50 mm scale line and place the real parts on it.
- On this PCB previous moves to **D2** and next to **D0**, so the firmware pins must be swapped.

## Side tracks

- **Market research:** looked at existing Spotify remotes and car/desk "now playing" screens to see what already exists.
- **Cheaper version:** firmware variants for a 0.96" SSD1306 OLED (with screen timeout to save power) and a 1.54" e-paper display ([`firmware/`](../firmware)).
- **Keychains:** two decorative keychain designs inspired by the remote ([`keychain/`](../keychain)).
