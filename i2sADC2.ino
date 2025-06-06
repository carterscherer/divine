
#include "AudioTools.h"

AudioInfo info(44100, 2, 32);
I2SStream i2sStream; // Access I2S as stream
CsvOutput<int32_t> csvOutput(Serial);
StreamCopy copier(csvOutput, i2sStream); // copy i2sStream to csvOutput

// Arduino Setup
void setup(void) {
    Serial.begin(115200);
    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);
    
    auto cfg = i2sStream.defaultConfig(RX_MODE);
    cfg.copyFrom(info);
    cfg.i2s_format = I2S_STD_FORMAT; // or try with I2S_LSB_FORMAT
    cfg.is_master = true;
     // this module nees a master clock if the ESP32 is master
    cfg.use_apll = false;  // try with yes
    cfg.pin_mck = 3; 
    i2sStream.begin(cfg);

    // make sure that we have the correct channels set up
    csvOutput.begin(info);

}

// Arduino loop - copy data
void loop() {
    copier.copy();
}
