# Smart Lane Monitor

A 3-lane traffic status system. Ultrasonic sensors on an Arduino Uno R4 WiFi
detect how clear each lane is, drive physical red/yellow/green LEDs per lane,
and serve the same status over WiFi as JSON so a browser dashboard can show
it live.

## Hardware

- Arduino Uno R4 WiFi
- 3x ultrasonic distance sensors (HC-SR04 or similar), one per lane
- 3x sets of red/yellow/green LEDs, one set per lane

See the `#define` pin numbers at the top of `lane_monitor.ino` for exact wiring.

## Files

- `lane_monitor.ino` — runs on the Arduino. Reads all 3 sensors, debounces
  the readings, drives the LEDs, and serves `GET /status` as JSON:
  `{"lane1":0,"lane2":1,"lane3":2}` where `0` = blocked (red), `1` = slow
  (yellow), `2` = clear (green).
- `traffic_dashboard_preview.html` — the browser dashboard. Can run in
  **simulate mode** (dropdowns, no hardware needed) or **live mode**
  (polls a real Arduino's `/status` endpoint every 1.5s).
- `wifi_diagnostic.ino` — a minimal sketch for debugging WiFi connection
  issues in isolation, useful if the main sketch won't connect.

## Running it

1. Open `lane_monitor.ino` in the Arduino IDE. Fill in your `ssid` and
   `password` near the top, then upload it to the board.
2. Open the Serial Monitor (9600 baud) and note the IP address it prints
   once connected.
3. Open `traffic_dashboard_preview.html` in a browser, **on a device
   connected to the same WiFi network as the Arduino**.
4. Type the Arduino's IP into the "Connect to Arduino" box and click
   Connect. The dashboard will start polling live sensor data.

**Note:** the dashboard must be run locally (or on the same local network
as the Arduino) — it can't be hosted on a public site like GitHub Pages
and still connect live, since the Arduino only serves plain HTTP on a
private local IP, which public HTTPS-hosted pages can't reach.

## Tuning

- `GREEN_LIMIT` / `YELLOW_LIMIT` in `lane_monitor.ino` control the distance
  thresholds (in cm) for each state — adjust based on your sensor's
  mounting height/distance.
- `DEBOUNCE_COUNT` controls how many consecutive matching readings are
  needed before a state change is accepted, to filter out sensor noise.
