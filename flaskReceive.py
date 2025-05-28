from flask import Flask, request
from datetime import datetime
import os

app = Flask(__name__)
UPLOAD_FOLDER = 'recordings'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload_audio():
    if request.data:
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f"{UPLOAD_FOLDER}/recording_{timestamp}.pcm"
        with open(filename, 'wb') as f:
            f.write(request.data)
        return f"Audio received and saved as {filename}", 200
    return "No data received", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
