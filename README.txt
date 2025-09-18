ESP32 Keg Level Monitor (Local Web UI)

A tiny ESP32 + HX711 sketch that reads a load-cell under your keg, smooths the signal, converts to pounds, estimates “% full,” and serves a refresh-every-5-seconds status page on your local network.

This is a simple project: I've seen snippets of this on other websites---for example here: https://www.hackster.io/davidtilley/iot-home-beer-keg-scale-b603db

My two cents are that there's a range of projects with different bells and whistles and various code that you can find online. I could not find any kind of design that worked fully off-the-shelf. Here's a build that worked completely. 

---

## What it does

- Reads HX711 load-cell amplifier (pins `DOUT=4`, `SCK=5` by default).
- Maintains a rolling buffer of 8 readings and displays the **averaged** weight.
- Estimates **percent full** assuming **8.0 lb** keg+tubing hardware and **41.7 lb** of liquid for a full 5-gal keg.
- Serves a minimal HTML page at `http://<esp32-ip>/` with weight, % full, and a corny status message.
- Prints raw and averaged values to Serial (115200).

---

## Hardware

- **ESP32** dev board (my favorite is ESP-WROOM-32 ESP32 ESP-32S Development Board---simple USB-C plugin)
- **HX711** amplifier module (
- **Load cell** (e.g., 50–200 kg single-point or four-cell platform)---you can either gut your own industrial scale, or get load cells and print a platform for it.
- Wires, stable power (use the ESP32’s 3.3V for HX711 VCC, USB-C via ESP-WROOM) 

### HX711 ↔ ESP32 wiring (default in code)

| HX711 | ESP32 | Notes |
|---|---|---|
| VCC | 3.3V | Do **not** use 5V on most ESP32 boards |
| GND | GND | Common ground required |
| DT (DOUT) | GPIO **4** | `const int DOUT = 4;` |
| SCK | GPIO **5** | `const int HX_SCK = 5;` |
| E+ / E- / A+ / A- | Load cell | Follow your load cell’s color map |

> If you change pins, update `DOUT` and `HX_SCK` in the sketch.

---

## Firmware prerequisites

- **Arduino IDE** (with the **ESP32 board package**)
- Libraries:
  - `WiFi` (bundled with ESP32 core)
  - `HX711` by Bogdan Necula (a.k.a. `bogde/HX711`)

### Install (Arduino IDE)

1. **Boards**: Install *ESP32 by Espressif Systems* via Boards Manager.
2. **Library**: Install *HX711* (by Bogdan Necula) via Library Manager.
3. Open the sketch, set your Wi-Fi SSID/password at the top.
4. Select your ESP32 board + port, and **Upload**.
5. Open **Serial Monitor** at **115200** baud to see the IP.


---

## Configuration knobs (in the sketch)

- **Wi-Fi**
  ```cpp
  const char* ssid = "yourWiFiName";
  const char* password = "yourWiFiPassword";
  ```

- **HX711 pins**
  ```cpp
  const int DOUT = 4;
  const int HX_SCK = 5;
  ```

- **Smoothing window & sample cadence**
  ```cpp
  const int NUM_READINGS = 8;           // rolling average size
  const unsigned long readingInterval = 1000; // ms
  ```

- **Calibration to pounds**
  ```cpp
  float rawToPounds(long reading) {
    return (14100.0 - reading) / 5850.0;
  }
  ```
  Replace these constants with your own (see **Calibration**).

- **Percent-full calculation**
  ```cpp
  latestPercent = 100.0 * ((latestAvgPounds - 8.0) / 41.7);
  latestPercent = constrain(latestPercent, 0.0, 100.0);
  ```
  - `8.0` = assumed keg+tubing hardware weight (empty)
  - `41.7` = assumed liquid weight when full (5 gal ≈ 41.7 lb)

  Adjust for your keg size, fittings, and beverage density.

---

## How to run

1. Power the ESP32 (and load cell/HX711).
2. Check **Serial Monitor** for:
   - “Connected to Wi-Fi”
   - The assigned **IP address** (e.g., `192.168.1.47`)
3. Visit `http://<that IP>/` in a browser on the same network.  
   The page auto-refreshes every 5 seconds.
Note: this IP comes fast---if you don't have serial monitor open, you may miss it!

---

---

## Assumptions baked into %-full

- **Empty hardware weight**: `8.0 lb` (keg shell + tubing + hardware) — change this to your actual empty weight.
- **Full liquid weight**: `41.7 lb` for a **5-gal** keg of beer (≈8.34 lb/gal × 5).  
  If you use sixtels/half-barrels or other beverages (coffee, kombucha), adjust accordingly.

Examples:
- **Sixtel (5.16 gal)** ≈ 43.0 lb liquid
- **Half barrel (15.5 gal)** ≈ 129.3 lb liquid

---

## Web endpoint & example

- **GET /** → HTML page with:
  - Weight (lbs, averaged)
  - Percent full (0–100%)
  - Status message (based on % thresholds)

Quick test from a terminal:
```bash
curl -s http://<esp32-ip>/ | head
```

---

## Troubleshooting

- **All readings ~0 or nonsense**
  - Check HX711 wiring, especially **DT/SCK pins** and **GND** commonality.
  - Ensure HX711 is powered via **3.3V**.
  - Verify the load cell is mounted securely and preloaded (no rocking).

- **Numbers jump around**
  - Increase `NUM_READINGS` (e.g., 16) or reading interval.
  - Isolate from vibration, add a foam pad, stabilize the platform.
  - Keep wires short, avoid power noise (use a stable 5V → 3.3V regulator feeding the ESP32).

- **Can’t find the web page**
  - Confirm Serial shows a valid **IP address**.
  - Make sure your computer/phone is on the same Wi-Fi network.
  - Some routers block client-to-client—try another network or enable client isolation off.

- **Bluetooth + Wi-Fi issues**
  - Many inexpensive ESP32 boards struggle with concurrent BT + Wi-Fi. Prefer Wi-Fi only for this sketch.

---

## Customization ideas

- **Change thresholds/messages** in `getBeerStatus()`
- **Different keg sizes**: replace `8.0` and `41.7` with your actual empty/base and full liquid weights.
- **Faster/slower UI refresh**: update the meta refresh in the HTML and `readingInterval`.
- **Security**: LAN-only, no TLS/auth. Don’t expose directly to the internet.
- **Next steps**: MQTT publishing, OLED/LED display, backend logging.

---

## Quick-start checklist

- [ ] Wire HX711 ↔ ESP32 (`DT→GPIO4`, `SCK→GPIO5`, `VCC→3.3V`, `GND→GND`)
- [ ] Install `HX711` library
- [ ] Put Wi-Fi SSID/password in the sketch
- [ ] Upload, open Serial @ 115200, note IP
- [ ] Calibrate `rawToPounds()` with at least one known weight
- [ ] Adjust `8.0` (empty hardware) and `41.7` (full liquid) for your setup
- [ ] Visit `http://<esp32-ip>/` and enjoy the stats 🍺
