#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ------------------- OLED Setup -------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_CS 5
#define OLED_DC 16
#define OLED_RESET 17

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ------------------- DHT Setup -------------------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ------------------- Calibration Settings -------------------
float tempCalibration = 0.0;  // adjust if needed (Â°C)
float humCalibration  = 0.0;  // adjust if needed (%)

// ------------------- Moving Average -------------------
#define WINDOW_SIZE 10
float tempWindow[WINDOW_SIZE] = {0};
float humWindow[WINDOW_SIZE] = {0};
int indexWin = 0;

// ------------------- Timing -------------------
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 2000; // every 2 sec
int slideIndex = 0; // cycles 0Ã¢â‚¬â€œ8 (0Ã¢â‚¬â€œ7 climates, 8 best match)

// ------------------- Climate Types -------------------
const String climateTypes[8] = {
  "Tropical","Subtropical","Temperate","Mediterranean",
  "Desert/Arid","Mountain/Alpine","Polar/Tundra","Savanna/Grassland"
};

// ------------------- Ideal Ranges -------------------
struct ClimateRange {
  float tempMin, tempMax;
  float humMin, humMax;
};

ClimateRange climateRanges[8] = {
  {20,30,70,90},    // Tropical
  {10,30,60,80},    // Subtropical
  {0,25,50,70},     // Temperate
  {10,30,40,60},    // Mediterranean
  {20,45,10,30},    // Desert/Arid
  {-5,15,40,70},    // Mountain/Alpine
  {-20,10,50,80},   // Polar/Tundra
  {20,30,30,60}     // Savanna/Grassland
};

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  Serial.println(" Smart Plant Climate Monitor Started...");
}

// ------------------- Main Loop -------------------
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    // Read sensor (with calibration)
    float temp = dht.readTemperature() + tempCalibration;
    float hum  = dht.readHumidity() + humCalibration;

    if (isnan(temp) || isnan(hum)) {
      displayError(" DHT11 Error!");
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Apply moving average smoothing
    tempWindow[indexWin] = temp;
    humWindow[indexWin] = hum;
    indexWin = (indexWin + 1) % WINDOW_SIZE;

    float avgTemp = 0, avgHum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
      avgTemp += tempWindow[i];
      avgHum += humWindow[i];
    }
    avgTemp /= WINDOW_SIZE;
    avgHum /= WINDOW_SIZE;

    // Display either individual climate slide or best match
    if (slideIndex < 8) {
      displayClimateSlide(avgTemp, avgHum, slideIndex);
    } else {
      displayBestMatch(avgTemp, avgHum);
    }

    slideIndex = (slideIndex + 1) % 9; // rotate 0Ã¢â‚¬â€œ8
  }
}

// ------------------- Display Error -------------------
void displayError(String msg) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(msg);
  display.display();
}

// ------------------- Plant Chance Calculation -------------------
int calculatePlantChance(int index, float temp, float hum) {
  ClimateRange range = climateRanges[index];

  float tempScore = 1.0 - abs((temp - (range.tempMin + range.tempMax)/2) / ((range.tempMax - range.tempMin)/2));
  float humScore  = 1.0 - abs((hum  - (range.humMin + range.humMax)/2) / ((range.humMax - range.humMin)/2));

  tempScore = constrain(tempScore, 0, 1);
  humScore  = constrain(humScore, 0, 1);

  return int((tempScore*50 + humScore*50)); // balanced weighting
}

// ------------------- Display Climate Slide -------------------
void displayClimateSlide(float temp, float hum, int index) {
  String climate = climateTypes[index];
  int chance = calculatePlantChance(index, temp, hum);

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Plant Climate Monitor");
  display.println("-------------------");
  display.println("Climate: " + climate);
  display.println("Temp: " + String(temp,1) + " C");
  display.println("Humidity: " + String(hum,1) + " %");
  display.println("Chance: " + String(chance) + " %");
  display.display();

  Serial.print("Climate: "); Serial.print(climate);
  Serial.print(" | Temp: "); Serial.print(temp);
  Serial.print("Â°C | Hum: "); Serial.print(hum);
  Serial.print("% | Chance: "); Serial.println(chance);
}

// ------------------- Display Best Match -------------------
void displayBestMatch(float temp, float hum) {
  int bestIndex = 0;
  int bestChance = 0;

  for (int i = 0; i < 8; i++) {
    int chance = calculatePlantChance(i, temp, hum);
    if (chance > bestChance) {
      bestChance = chance;
      bestIndex = i;
    }
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Best Match Climate");
  display.println("-------------------");
  display.println("Climate: " + climateTypes[bestIndex]);
  display.println("Temp: " + String(temp,1) + " C");
  display.println("Humidity: " + String(hum,1) + " %");
  display.println("Chance: " + String(bestChance) + " %");
  display.display();

  Serial.print("Best Match Climate: ");
  Serial.print(climateTypes[bestIndex]);
  Serial.print(" | Chance: ");
  Serial.println(bestChance);
}