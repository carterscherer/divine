#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Flask server details
const char* serverUrl = "http://192.168.68.60:5000/upload";

// I2S configuration
#define I2S_WS 37
#define I2S_SD 38  // Make sure this is connected to your ESP32's I2S data pin
#define I2S_SCK 36
#define I2S_PORT I2S_NUM_0

// Button configuration
#define BUTTON_PIN 5

// Audio configuration
#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16
#define RECORD_TIME 5 // seconds
#define BUFFER_SIZE (SAMPLE_RATE * SAMPLE_BITS / 8 * RECORD_TIME)

uint8_t* audioBuffer = NULL;
size_t bytesRead = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT);
  
  // Initialize I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_in_num = I2S_SD,
    .data_out_num = I2S_PIN_NO_CHANGE
  };
  
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    Serial.println("Button pressed - starting recording");
    recordAudio();
    sendToServer();
    delay(1000); // debounce
  }
}

void recordAudio() {
  // Allocate buffer for recording
  audioBuffer = (uint8_t*)malloc(BUFFER_SIZE);
  if (audioBuffer == NULL) {
    Serial.println("Failed to allocate audio buffer");
    return;
  }
  
  Serial.println("Recording...");
  bytesRead = 0;
  size_t totalBytes = SAMPLE_RATE * sizeof(int16_t) * RECORD_TIME;
  
  while (bytesRead < totalBytes) {
    size_t bytesToRead = min(totalBytes - bytesRead, (size_t)1024);
    size_t bytesReadNow = 0;
    i2s_read(I2S_PORT, audioBuffer + bytesRead, bytesToRead, &bytesReadNow, portMAX_DELAY);
    bytesRead += bytesReadNow;
  }
  
  Serial.println("Recording finished");
}

void sendToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }
  
  // Create WAV header
  uint32_t fileSize = bytesRead + 44 - 8;
  uint32_t dataSize = bytesRead;
  
  uint8_t wavHeader[44] = {
    // RIFF header
    'R', 'I', 'F', 'F',
    fileSize & 0xff, (fileSize >> 8) & 0xff, (fileSize >> 16) & 0xff, (fileSize >> 24) & 0xff,
    'W', 'A', 'V', 'E',
    // fmt chunk
    'f', 'm', 't', ' ',
    16, 0, 0, 0, // fmt chunk size (16 for PCM)
    1, 0, // Audio format (1 = PCM)
    1, 0, // Number of channels (1)
    SAMPLE_RATE & 0xff, (SAMPLE_RATE >> 8) & 0xff, (SAMPLE_RATE >> 16) & 0xff, (SAMPLE_RATE >> 24) & 0xff,
    (SAMPLE_RATE * sizeof(int16_t)) & 0xff, ((SAMPLE_RATE * sizeof(int16_t)) >> 8) & 0xff, 
    ((SAMPLE_RATE * sizeof(int16_t)) >> 16) & 0xff, ((SAMPLE_RATE * sizeof(int16_t)) >> 24) & 0xff,
    sizeof(int16_t), 0, // Block align
    SAMPLE_BITS, 0, // Bits per sample
    // data chunk
    'd', 'a', 't', 'a',
    dataSize & 0xff, (dataSize >> 8) & 0xff, (dataSize >> 16) & 0xff, (dataSize >> 24) & 0xff
  };
  
  // Combine header and audio data
  uint8_t* wavFile = (uint8_t*)malloc(bytesRead + 44);
  memcpy(wavFile, wavHeader, 44);
  memcpy(wavFile + 44, audioBuffer, bytesRead);
  
  // Send to server
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "audio/wav");
  
  int httpResponseCode = http.POST(wavFile, bytesRead + 44);
  
  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.printf("Error code: %d\n", httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  
  // Free memory
  free(wavFile);
  free(audioBuffer);
  audioBuffer = NULL;
}