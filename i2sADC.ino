#include <AudioLogger.h>
#include <AudioTools.h>
#include <AudioToolsConfig.h>

// Make sure that USB CDC on Boot is enabled
// I2S ADC 24BIT 192K-96K-48K / 16 BIT 48K

const int I2S_WS = 40;  // LRCK LEFT RIGHT CLOCK 
const int I2S_SCK = 42; // BLCK
const int I2S_SD = 15; //  DATA

//AudioEncodedServer server(new WAVEncoder(),"ssid","password");  
AudioWAVServer server("applebees","bonelesswings"); // the same a above

I2SStream i2sStream;    // Access I2S as stream
ConverterFillLeftAndRight<int16_t> filler(LeftIsEmpty); // fill both channels - or change to RightIsEmpty

void setup(){
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

  // start i2s input with default configuration
  Serial.println("starting I2S...");

  auto config = i2sStream.defaultConfig(RX_MODE);
  config.sample_rate = 48000;                // 48kHz, supported in both 24-bit and 16-bit modes
  config.bits_per_sample = 32;               // Use 32 because 24-bit data is usually left-justified in 32-bit word
  config.i2s_format = I2S_STD_FORMAT;        // Standard I2S format (most ADCs use this)
  config.channels = 2;                       // Stereo (typical for ADCs)
  config.pin_bck = I2S_SCK;                  // Bit clock (BCLK)
  config.pin_ws = I2S_WS;                    // Word select (LRCK)
  config.pin_data = I2S_SD;   
  config.is_master = true;               // Serial data output from ADC
  config.use_apll = false;
  config.pin_mck = 44;
  i2sStream.begin(config);

  Serial.println("I2S started");

  // start data sink
  server.begin(i2sStream, config, &filler);
}

// Arduino loop  
void loop() {
  // Handle new connections
  server.copy();  
}
