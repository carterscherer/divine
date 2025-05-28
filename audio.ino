#include <SD.h>
#include <SPI.h>
#include <esp_adc/adc_oneshot.h>

// SD Card pins (ESP32-S3 defaults)
#define SD_MOSI 11  // DI
#define SD_MISO 13  // DO
#define SD_SCK  12  // SCK
#define SD_CS   10  // CS

// Analog mic pin (must be ADC1 channel on ESP32-S3)
#define MIC_PIN ADC_CHANNEL_4  // GPIO4

// Record control pin
#define RECORD_SWITCH_PIN 5

// Audio configuration
const int sampleRate = 16000;  // 16kHz sample rate
const int bufferSize = 1024;   // Buffer size for SD writes
const int recordingDuration = 5; // Record for 5 seconds
uint16_t audioBuffer[bufferSize];
File audioFile;

// ADC handle
adc_oneshot_unit_handle_t adc_handle;
bool isRecording = false;

void setup() {
  Serial.begin(115200);
  while(!Serial); // Wait for serial port
  
  // Initialize SD card with custom pins
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    while(1);
  }
  Serial.println("SD Card initialized.");

  // Configure ADC for microphone
  adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t config = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MIC_PIN, &config));

  // Setup button pin - CHANGE TO INPUT_PULLDOWN IF YOUR BUTTON PULLS HIGH
  pinMode(RECORD_SWITCH_PIN, INPUT);

  Serial.println("System ready. Signal HIGH on pin 5 to record 5 seconds of audio.");
}

void loop() {
  // Immediately start recording when pin goes HIGH
  if (digitalRead(RECORD_SWITCH_PIN) == HIGH && !isRecording) {
    startRecording();
  }
}

void startRecording() {
  isRecording = true;
  Serial.println("Recording trigger detected - starting recording...");
  
  // Create unique filename
  char filename[20];
  int fileNumber = 0;
  do {
    sprintf(filename, "/rec_%d.wav", fileNumber++);
  } while (SD.exists(filename));

  Serial.print("Recording to: ");
  Serial.println(filename);

  // Open WAV file
  audioFile = SD.open(filename, FILE_WRITE);
  if (!audioFile) {
    Serial.println("Failed to create file!");
    isRecording = false;
    return;
  }

  // Write WAV header
  writeWavHeader(audioFile, sampleRate);

  // Record for exactly 5 seconds
  unsigned long startTime = millis();
  unsigned long samplesRecorded = 0;
  
  while ((millis() - startTime) < recordingDuration * 1000) {
    unsigned long sampleTime = micros();
    
    // Record one buffer of samples
    for (int i = 0; i < bufferSize; i++) {
      // Read analog value (0-4095 for 12-bit ADC)
      int raw;
      ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, MIC_PIN, &raw));
      audioBuffer[i] = raw << 4; // Scale to 16-bit
      
      // Wait for next sample time
      while (micros() - sampleTime < (i * (1000000/sampleRate))) {
        yield();
      }
    }

    // Write buffer to SD card
    audioFile.write((uint8_t*)audioBuffer, bufferSize * 2);
  }

  stopRecording();
}

void stopRecording() {
  // Update WAV header with actual size
  updateWavHeader(audioFile, sampleRate * recordingDuration);
  audioFile.close();
  
  isRecording = false;
  Serial.println("5-second recording complete.");
}

void writeWavHeader(File file, int sampleRate) {
  byte header[44];
  
  // RIFF chunk
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  *(uint32_t*)(header + 4) = 36 + (sampleRate * recordingDuration * 2);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  
  // fmt subchunk
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  *(uint32_t*)(header + 16) = 16;
  *(uint16_t*)(header + 20) = 1;
  *(uint16_t*)(header + 22) = 1;
  *(uint32_t*)(header + 24) = sampleRate;
  *(uint32_t*)(header + 28) = sampleRate * 2;
  *(uint16_t*)(header + 32) = 2;
  *(uint16_t*)(header + 34) = 16;
  
  // data subchunk
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  *(uint32_t*)(header + 40) = sampleRate * recordingDuration * 2;
  
  file.write(header, 44);
}

void updateWavHeader(File file, uint32_t sampleCount) {
  uint32_t dataSize = sampleCount * 2;
  uint32_t fileSize = dataSize + 36;
  
  file.seek(4);
  file.write((uint8_t*)&fileSize, 4);
  
  file.seek(40);
  file.write((uint8_t*)&dataSize, 4);
}