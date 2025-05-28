import os
from dotenv import load_dotenv
import google.generativeai as genai

# Load environment variables from .env file
load_dotenv()

# Get the Gemini API key from the environment variables
GOOGLE_API_KEY = os.getenv("GOOGLE_API_KEY")

# Check if the API key is loaded
if not GOOGLE_API_KEY:
    print("Error: GOOGLE_API_KEY not found in .env file. Please create a .env file with your API key.")
    exit()

genai.configure(api_key=GOOGLE_API_KEY)

# Specific path to the SD card volume
SD_CARD_PATH = "/Volumes/GENERIC/"

# Output directory for the transcriptions
OUTPUT_DIR = "transcriptions"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def transcribe_audio(audio_file_path):
    """Transcribes an audio file using the Gemini API."""
    try:
        model = genai.GenerativeModel('gemini-pro')  # Or 'gemini-pro-vision' if needed
        with open(audio_file_path, "rb") as audio_file:
            audio_data = audio_file.read()

        response = model.generate_content(
            [{"mime_type": "audio/*", "data": audio_data}],  # Adjust mime_type if known
            stream=False,
        )
        return response.text
    except Exception as e:
        print(f"Error transcribing {audio_file_path}: {e}")
        return None

def process_specific_file(sd_card_path, output_dir, filename):
    """Transcribes a specific audio file from the SD card."""
    audio_file_path = os.path.join(sd_card_path, filename)
    if os.path.exists(audio_file_path):
        print(f"Found audio file: {audio_file_path}")
        transcription = transcribe_audio(audio_file_path)
        if transcription:
            output_filename = f"{os.path.splitext(filename)[0]}.txt"
            output_path = os.path.join(output_dir, output_filename)
            with open(output_path, "w") as outfile:
                outfile.write(transcription)
            print(f"Transcription saved to: {output_path}")
        else:
            print(f"Transcription failed for {audio_file_path}.")
    else:
        print(f"Error: File '{filename}' not found in '{sd_card_path}'.")

if __name__ == "__main__":
    print(f"Looking for audio files in: {SD_CARD_PATH}")
    file_to_transcribe = input("Enter the exact name of the audio file you want to transcribe (e.g., audio.wav): ")

    process_specific_file(SD_CARD_PATH, OUTPUT_DIR, file_to_transcribe)

    print("Transcription process completed (for the specified file).")