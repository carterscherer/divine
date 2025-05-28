#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "applebees";
const char* password = "bonelesswings";
const char* serverUrl = "http://192.169.68.54:8080/upload";  // <-- Set your Flask server IP and port

// I2S mic pins
#define I2S_WS 25
#define I2S_SD 33
#define I2S_SCK 32
#define I2S_PORT I2S_NUM_0

// Buffer settings
#define bufferLen 1024
int16_t sBuffer[bufferLen];

// Button pin
#define BUTTON_PIN 2

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  connectToWiFi();
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed! Recording...");

    std::vector<uint8_t> audioData;

    // Record for ~2 seconds
    unsigned long start = millis();
    while (millis() - start < 2000) {
      size_t bytesRead;
      i2s_read(I2S_PORT, &sBuffer, sizeof(sBuffer), &bytesRead, portMAX_DELAY);
      audioData.insert(audioData.end(), (uint8_t*)sBuffer, (uint8_t*)sBuffer + bytesRead);
    }

    Serial.println("Recording done. Sending...");

    // Send to Flask server
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");
    int httpResponseCode = http.POST(audioData.data(), audioData.size());

    if (httpResponseCode > 0) {
      Serial.printf("Server responded: %d\n", httpResponseCode);
    } else {
      Serial.printf("Failed to send: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    delay(1000);
  }
}
