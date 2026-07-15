#include <WiFiS3.h>

// =============== WIFI ===============
// Edit these to match your network.
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

// =============== PINS ===============

// Lane 1
#define TRIG1 2
#define ECHO1 3
#define RED1 4
#define YELLOW1 5
#define GREEN1 6

// Lane 2
#define TRIG2 11
#define ECHO2 12
#define RED2 8
#define YELLOW2 9
#define GREEN2 10

// Lane 3
#define TRIG3 13
#define ECHO3 A0
#define RED3 A1
#define YELLOW3 A2
#define GREEN3 A3

// =============== SETTINGS ===============

const float MAX_DISTANCE = 150.0;
// pulseIn timeout derived from MAX_DISTANCE (~29us per cm round trip)
const unsigned long ECHO_TIMEOUT_US = (unsigned long)(MAX_DISTANCE * 2.0 * 29.0);

const float GREEN_LIMIT  = 90.0;
const float YELLOW_LIMIT = 40.0;

// Gap between reading each lane's sensor so the previous echo dies
// down before the next TRIG fires (prevents cross-talk between the
// 3 ultrasonic sensors, which otherwise causes lights to flip on their own).
const unsigned long LANE_SETTLE_MS = 60;

// A state must be seen this many times in a row before we act on it.
// This kills single-reading flicker/noise (lights changing by themselves).
const int DEBOUNCE_COUNT = 4;

// =============== STATE ===============

int stableState[3]    = {2, 2, 2};  // last confirmed state per lane
int candidateState[3] = {2, 2, 2};  // state we're currently seeing
int candidateCount[3] = {0, 0, 0};  // how many times in a row we've seen it

// =============== SENSOR ===============

float pingOnce(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return MAX_DISTANCE;

  float distance = duration * 0.0343 / 2.0;
  return constrain(distance, 0, MAX_DISTANCE);
}

// Takes 3 quick readings and returns the median, so a single noisy/
// reflected echo can't cause a light to jump by itself.
float getDistance(int trig, int echo) {
  float a = pingOnce(trig, echo);
  delayMicroseconds(600); // brief gap so this sensor's own echo clears
  float b = pingOnce(trig, echo);
  delayMicroseconds(600);
  float c = pingOnce(trig, echo);

  // Median of 3
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

// =============== LIGHTS ===============

void lane1Light(int state) {
  digitalWrite(RED1, state == 0);
  digitalWrite(YELLOW1, state == 1);
  digitalWrite(GREEN1, state == 2);
}
void lane2Light(int state) {
  digitalWrite(RED2, state == 0);
  digitalWrite(YELLOW2, state == 1);
  digitalWrite(GREEN2, state == 2);
}
void lane3Light(int state) {
  digitalWrite(RED3, state == 0);
  digitalWrite(YELLOW3, state == 1);
  digitalWrite(GREEN3, state == 2);
}

void setLaneLight(int lane, int state) {
  if (lane == 0) lane1Light(state);
  if (lane == 1) lane2Light(state);
  if (lane == 2) lane3Light(state);
}

// =============== CHECK LANE ===============

void checkLane(int lane, float distance) {
  int rawState;
  if (distance > GREEN_LIMIT) rawState = 2;
  else if (distance >= YELLOW_LIMIT) rawState = 1;
  else rawState = 0;

  // Debounce: only accept a new state once we've seen it consistently.
  if (rawState == candidateState[lane]) {
    if (candidateCount[lane] < DEBOUNCE_COUNT) candidateCount[lane]++;
  } else {
    candidateState[lane] = rawState;
    candidateCount[lane] = 1;
  }

  if (candidateCount[lane] < DEBOUNCE_COUNT) {
    // Not confirmed yet, keep showing the last stable light.
    setLaneLight(lane, stableState[lane]);
    return;
  }

  stableState[lane] = rawState;
  setLaneLight(lane, rawState); // lights update immediately, every loop
}

// =============== WEB SERVER ===============
// Serves the same {"lane1":N,"lane2":N,"lane3":N} shape the dashboard
// expects, built from the debounced stableState[] the lights already use
// (not a raw sensor reading), so the webpage always agrees with the LEDs.

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  // Brief wait for the request bytes to arrive, then drain them.
  // We only serve one thing, so we don't need to parse the path.
  unsigned long start = millis();
  while (client.connected() && !client.available() && millis() - start < 200) {
    // waiting
  }
  while (client.available()) {
    client.read();
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *"); // lets the dashboard fetch() from any host/file
  client.println("Connection: close");
  client.println();
  client.print("{\"lane1\":");
  client.print(stableState[0]);
  client.print(",\"lane2\":");
  client.print(stableState[1]);
  client.print(",\"lane3\":");
  client.print(stableState[2]);
  client.println("}");

  delay(1);
  client.stop();
}

// =============== SETUP ===============

void setup() {
  Serial.begin(9600);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

  pinMode(RED1, OUTPUT); pinMode(YELLOW1, OUTPUT); pinMode(GREEN1, OUTPUT);
  pinMode(RED2, OUTPUT); pinMode(YELLOW2, OUTPUT); pinMode(GREEN2, OUTPUT);
  pinMode(RED3, OUTPUT); pinMode(YELLOW3, OUTPUT); pinMode(GREEN3, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected!");
  Serial.print("Dashboard should point at: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

// =============== LOOP ===============

void loop() {
  float lane1 = getDistance(TRIG1, ECHO1);
  checkLane(0, lane1);
  delay(LANE_SETTLE_MS);

  float lane2 = getDistance(TRIG2, ECHO2);
  checkLane(1, lane2);
  delay(LANE_SETTLE_MS);

  float lane3 = getDistance(TRIG3, ECHO3);
  checkLane(2, lane3);

  handleClient();
}
