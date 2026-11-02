/* 
WHAT TO CHANGE AFTER GETTING COMPONENTS:
NOISE_FLOOR (currently 40) - set to whatever number you get in serial monitor ( raw: value) in a quiet room
LOUD_MAX (currently 1500) -  set to whatever number you get in serial monitor ( raw: value) when playing loudest music you will play (8 leds lighting up)
RELEASE - How fast the LEDs fall, lower number = slower, higher number = faster
PEAK_HOLD_MS and PEAK_FALL_MS -  adjust by eye for how long the blue dot lingers and how fast it drops
SAMPLE_WINDOW (currently 30 ms) - change it to change refresh rate of LEDs
BRIGHTNESS - LED brightness

CHANGE NOW OR LATER:
uint32_t colorForLed(int i) - colors of LEDs
*/


#include <Adafruit_NeoPixel.h>

// ---------- Pins and sizes ----------
const int MIC_PIN = A0;
const int LED_PIN = D4;
const int NUM_LEDS = 8;

// ---------- Tuning values (adjust these once hardware arrives) ----------
const int SAMPLE_WINDOW = 30;     // ms spent measuring each reading
const int NOISE_FLOOR = 40;       // peak-to-peak value in a quiet room
const int LOUD_MAX = 1500;        // peak-to-peak value at "full scale"
const int BRIGHTNESS = 40;        // 0-255, keep low to protect your eyes and USB power
const float RELEASE = 0.15;       // how fast the bar falls (smaller = slower)
const unsigned long PEAK_HOLD_MS = 400;   // how long the peak dot stays
const unsigned long PEAK_FALL_MS = 100;   // how often the peak dot drops after that

const bool DEBUG = true;          // print values to Serial Monitor for tuning

// ---------- Globals ----------
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

float smoothed = 0;               // smoothed loudness
int peakLevel = 0;                // level of the peak-hold dot
unsigned long peakTime = 0;       // when the peak was last set
unsigned long lastPeakDrop = 0;   // when the dot last fell

// ---------- Helpers ----------

// Measure loudness: highest minus lowest sample over a short window
int readLoudness() {
  unsigned long startTime = millis();
  int signalMax = 0;
  int signalMin = 4095;

  while (millis() - startTime < SAMPLE_WINDOW) {
    int sample = analogRead(MIC_PIN);
    signalMax = max(signalMax, sample);
    signalMin = min(signalMin, sample);
  }
  return signalMax - signalMin;
}

// Color for each LED position: green -> yellow -> red
uint32_t colorForLed(int i) {
  if (i < 5) return strip.Color(0, 255, 0);       // LEDs 0-4 green
  if (i < 7) return strip.Color(255, 160, 0);     // LEDs 5-6 yellow
  return strip.Color(255, 0, 0);                  // LED 7 red
}

// ---------- Arduino functions ----------
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);       // readings 0-4095

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  // 1. Measure loudness
  int raw = readLoudness();

  // 2. Smoothing: rise instantly, fall gradually
  if (raw > smoothed) {
    smoothed = raw;
  } else {
    smoothed = smoothed * (1.0 - RELEASE) + raw * RELEASE;
  }

  // 3. Convert loudness to number of lit LEDs (0-8)
  int level = map((int)smoothed, NOISE_FLOOR, LOUD_MAX, 0, NUM_LEDS);
  level = constrain(level, 0, NUM_LEDS);

  // 4. Peak hold: dot jumps up with the music, then falls slowly
  unsigned long now = millis();
  if (level >= peakLevel) {
    peakLevel = level;
    peakTime = now;
  } else if (now - peakTime > PEAK_HOLD_MS && now - lastPeakDrop > PEAK_FALL_MS) {
    peakLevel--;
    lastPeakDrop = now;
  }

  // 5. Draw
  strip.clear();
  for (int i = 0; i < level; i++) {
    strip.setPixelColor(i, colorForLed(i));
  }
  if (peakLevel > 0) {
    strip.setPixelColor(peakLevel - 1, strip.Color(0, 0, 255));   // blue peak dot
  }
  strip.show();

  // 6. Debug output for tuning
  if (DEBUG) {
    Serial.print("raw:");
    Serial.print(raw);
    Serial.print(" level:");
    Serial.println(level);
  }
}
