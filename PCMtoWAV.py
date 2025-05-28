import os
import glob
import wave

def pcm_to_wav(pcm_file, wav_file, channels=1, sample_rate=16000):
    with open(pcm_file, 'rb') as pcm:
        pcm_data = pcm.read()
    with wave.open(wav_file, 'wb') as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)  # 16 bits = 2 bytes
        wav.setframerate(sample_rate)
        wav.writeframes(pcm_data)

# Directory containing your .pcm files
recordings_folder = "recordings"

# Convert all .pcm files in that folder
for pcm_path in glob.glob(os.path.join(recordings_folder, "*.pcm")):
    base = os.path.splitext(pcm_path)[0]
    wav_path = base + ".wav"
    pcm_to_wav(pcm_path, wav_path)
    print(f"Converted: {pcm_path} -> {wav_path}")
