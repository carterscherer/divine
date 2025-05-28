import machine
import time
import uos
from machine import Pin, ADC, SDCard, SPI
import math

# Pin Configuration
MIC_PIN = 9
BUTTON_PIN = 5
SD_MOSI = 35
SD_MISO = 37
SD_SCK = 36
SD_CS = 38

# Audio Configuration
SAMPLE_RATE = 8000  # 8kHz sample rate
RECORD_SECONDS = 5
NUM_CHANNELS = 1
BITS_PER_SAMPLE = 16
BYTES_PER_SAMPLE = BITS_PER_SAMPLE // 8

# Calculate buffer size
BUFFER_SIZE = SAMPLE_RATE * RECORD_SECONDS

# Initialize hardware
mic = ADC(Pin(MIC_PIN))
mic.atten(ADC.ATTN_11DB)  # Full range 0-3.3V
button = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_DOWN)

# Initialize SD Card - PROPER SPI INITIALIZATION
try:
    # First initialize SPI
    spi = SPI(1, baudrate=20000000,  # Try lower baudrate (1MHz) if this fails
              sck=Pin(SD_SCK),
              mosi=Pin(SD_MOSI),
              miso=Pin(SD_MISO))
    
    # Then initialize SD card with SPI
    sd = SDCard(spi, Pin(SD_CS))
    
    # Mount filesystem
    vfs = uos.VfsFat(sd)
    uos.mount(vfs, "/sd")
    print("SD card mounted successfully")
except Exception as e:
    print("Failed to initialize SD card:", e)
    # If mounting fails, you might want to reboot or handle it differently
    machine.reset()


def create_wav_header(data_size):
    """Generate a WAV header for the given data size"""
    header = bytearray(44)
    
    # RIFF header
    header[0:4] = b'RIFF'
    header[4:8] = (data_size + 36).to_bytes(4, 'little')  # File size - 8
    header[8:12] = b'WAVE'
    
    # fmt chunk
    header[12:16] = b'fmt '
    header[16:20] = (16).to_bytes(4, 'little')  # fmt chunk size
    header[20:22] = (1).to_bytes(2, 'little')  # PCM format
    header[22:24] = NUM_CHANNELS.to_bytes(2, 'little')
    header[24:28] = SAMPLE_RATE.to_bytes(4, 'little')
    header[28:32] = (SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE).to_bytes(4, 'little')  # Byte rate
    header[32:34] = (NUM_CHANNELS * BYTES_PER_SAMPLE).to_bytes(2, 'little')  # Block align
    header[34:36] = BITS_PER_SAMPLE.to_bytes(2, 'little')
    
    # data chunk
    header[36:40] = b'data'
    header[40:44] = data_size.to_bytes(4, 'little')
    
    return header

def record_audio():
    print("Recording started...")
    audio_data = bytearray(BUFFER_SIZE * BYTES_PER_SAMPLE)
    
    start_time = time.ticks_ms()
    for i in range(BUFFER_SIZE):
        # Read analog value (0-4095 for 12-bit ADC)
        sample = mic.read()
        
        # Convert to 16-bit signed (-32768 to 32767)
        sample = (sample - 2048) * 16  # Scale to 16-bit range
        sample = max(min(sample, 32767), -32768)
        
        # Store as little-endian bytes
        audio_data[i*2:i*2+2] = sample.to_bytes(2, 'little', signed=True)
        
        # Sleep to maintain sample rate
        while time.ticks_diff(time.ticks_ms(), start_time) < i * 1000 / SAMPLE_RATE:
            pass
    
    print("Recording complete")
    return audio_data

def save_to_sd(audio_data):
    # Generate filename with timestamp
    timestamp = time.localtime()
    filename = "/sd/recording_{:04d}{:02d}{:02d}_{:02d}{:02d}{:02d}.wav".format(
        timestamp[0], timestamp[1], timestamp[2],
        timestamp[3], timestamp[4], timestamp[5]
    )
    
    # Create WAV header
    header = create_wav_header(len(audio_data))
    
    # Write to file
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(audio_data)
    
    print(f"Saved to {filename}")

def main():
    print("Waiting for button press...")
    
    while True:
        if button.value():  # Button pressed
            # Record audio
            audio_data = record_audio()
            
            # Save to SD card
            save_to_sd(audio_data)
            
            # Debounce delay
            time.sleep(1)
        
        time.sleep(0.1)

if __name__ == "__main__":
    main()
