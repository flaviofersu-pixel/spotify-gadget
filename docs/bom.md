# Bill of materials

Prices are what I paid or saw in the Netherlands (Oct 2026), including VAT unless noted. Check stock and part numbers before ordering.

## Prototype (desk version, case v7)

| Qty | Part | Notes | Approx. price |
|---|---|---|---|
| 1 | Seeed Studio XIAO nRF52840 (102010448), **pre-soldered headers** | For the breadboard | ~€12 |
| 1 | Seeed Studio XIAO nRF52840, **no headers** | Goes inside the case | ~€10 |
| 1 | Adafruit Sharp Memory Display breakout 1.3" 144x168 (ADA-3502) | DigiKey was out of stock, Kiwi Electronics had it | ~€28 |
| 1 | LiPo 3.7 V 150 mAh with protection circuit (PKCELL, 28 x 20.5 x 4.2 mm) | Cut the JST plug and solder to BAT+/BAT- under the XIAO, red to + | ~€5 |
| 2 | Tactile switch 6 x 6 mm | Previous and next | pack of 20 ~€2 |
| 1 | Tactile switch 12 x 12 mm | Play/pause (case v4 and later) | ~€0.50 |
| - | Thin silicone wire, 28-30 AWG | Much easier than breadboard jumpers inside the case | ~€5 |
| - | PLA filament | Case and button caps | - |

**Tools:** soldering iron (350-370 °C for lead-free solder), 0.6-0.8 mm flux-core solder, flux pen, breadboard and jumper wires, tweezers.

## Slim version (next to the camera, planned)

Reuses the XIAO without headers, the battery and the switches, plus:

| Qty | Part | Notes | Approx. price |
|---|---|---|---|
| 1 | Bare Sharp LS013B7DH05 panel | Taken out of the Adafruit breakout (24.7 x 30 x 0.75 mm), or a spare | - |
| 1 | FPC breakout, 10 pin, **0.5 mm pitch**, connector pre-soldered, no header | Recommended connector in the Sharp datasheet: Panasonic AYF531035 (Y5B series) | ~€1.20 |
| 1 | Strong double-sided tape (3M VHB type) | Mounting on the phone case. No magnets near the camera. | ~€3 |

## PCB v0.1

| Qty | Part | Notes | Approx. price |
|---|---|---|---|
| 5 | 2-layer PCB, 43 x 53 mm | Upload `hardware/pcb/v0.1/mini-remote-v0.1-gerbers.zip` to JLCPCB or AISLER | ~€10-20 incl. shipping |
| 3 | M2.5 screws | Hold the display board | - |

## Cheaper display variants

| Firmware | Display | Notes |
|---|---|---|
| `firmware/mini_mando_oled` | 0.96" SSD1306 OLED, 128x64, I2C | ~€5, uses more power, screen turns off after a timeout |
| `firmware/mini_mando_eink` | 1.54" e-paper, 200x200, SSD1681 (WeAct / Waveshare V2) | Zero power while showing an image, refreshes only on song change |
