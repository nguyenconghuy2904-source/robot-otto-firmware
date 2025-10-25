#!/usr/bin/env python3
"""
WebSocket Server cho ESP32-S3 Touch LCD 1.46
Nhận và phát audio/video giữa PC và ESP32
"""

import asyncio
import websockets
import pyaudio
import numpy as np
from PIL import Image
import io
import threading
import queue

# Cấu hình Audio
CHUNK = 320  # 20ms @ 16kHz
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000

class ESP32StreamServer:
    def __init__(self, host="0.0.0.0", port=8765):
        self.host = host
        self.port = port
        self.clients = set()
        
        # Audio setup
        self.audio = pyaudio.PyAudio()
        self.audio_queue = queue.Queue()
        
        # Input stream (microphone)
        self.input_stream = self.audio.open(
            format=FORMAT,
            channels=CHANNELS,
            rate=RATE,
            input=True,
            frames_per_buffer=CHUNK
        )
        
        # Output stream (speaker)
        self.output_stream = self.audio.open(
            format=FORMAT,
            channels=CHANNELS,
            rate=RATE,
            output=True,
            frames_per_buffer=CHUNK
        )
        
        print(f"🎤 Audio initialized: {RATE}Hz, {CHANNELS} channel(s)")
    
    async def handle_client(self, websocket, path):
        """Xử lý kết nối từ ESP32"""
        client_addr = websocket.remote_address
        print(f"✅ ESP32 connected from {client_addr}")
        self.clients.add(websocket)
        
        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    await self.handle_binary_message(message, websocket)
                else:
                    print(f"📝 Text message: {message}")
                    
        except websockets.exceptions.ConnectionClosed:
            print(f"❌ ESP32 disconnected: {client_addr}")
        finally:
            self.clients.discard(websocket)
    
    async def handle_binary_message(self, message, websocket):
        """Xử lý gói tin binary từ ESP32"""
        if len(message) < 6:
            return
            
        header = message[:6]
        payload = message[6:]
        
        if header == b'AUDIO:':
            # Nhận audio từ ESP32 (Opus encoded)
            print(f"🔊 Received audio: {len(payload)} bytes")
            # TODO: Decode Opus và phát ra loa
            # Hiện tại chỉ log, cần thêm Opus decoder
            
            # Phát audio đơn giản (nếu là PCM raw)
            # self.output_stream.write(payload)
            
        elif header == b'VIDEO:':
            # Nhận video frame từ ESP32 (JPEG)
            print(f"📹 Received video frame: {len(payload)} bytes")
            try:
                # Decode và hiển thị JPEG
                img = Image.open(io.BytesIO(payload))
                img.show()  # Mở viewer (hoặc save file)
                # img.save(f"frame_{int(asyncio.get_event_loop().time())}.jpg")
            except Exception as e:
                print(f"❌ Failed to decode JPEG: {e}")
    
    async def send_audio_to_esp32(self):
        """Gửi audio từ PC mic tới ESP32"""
        while True:
            try:
                # Đọc audio từ microphone
                audio_data = self.input_stream.read(CHUNK, exception_on_overflow=False)
                
                # Gửi tới tất cả ESP32 clients
                if self.clients:
                    message = b'AUDIO:' + audio_data
                    await asyncio.gather(
                        *[client.send(message) for client in self.clients],
                        return_exceptions=True
                    )
                    
                await asyncio.sleep(0.02)  # 20ms
            except Exception as e:
                print(f"❌ Audio send error: {e}")
                await asyncio.sleep(0.1)
    
    async def send_video_to_esp32(self):
        """Gửi video frame từ PC camera tới ESP32 (optional)"""
        # TODO: Implement webcam capture và gửi JPEG
        while True:
            await asyncio.sleep(0.1)  # 100ms
    
    async def start_server(self):
        """Khởi động WebSocket server"""
        async with websockets.serve(self.handle_client, self.host, self.port):
            print(f"🚀 WebSocket server running on ws://{self.host}:{self.port}")
            print("📡 Waiting for ESP32 connection...")
            
            # Chạy song song: server + audio sender
            await asyncio.gather(
                asyncio.Future(),  # Keep server running
                # self.send_audio_to_esp32(),  # Uncomment để gửi audio PC → ESP32
            )
    
    def cleanup(self):
        """Dọn dẹp resources"""
        self.input_stream.stop_stream()
        self.input_stream.close()
        self.output_stream.stop_stream()
        self.output_stream.close()
        self.audio.terminate()
        print("🧹 Cleaned up resources")


def main():
    server = ESP32StreamServer(host="0.0.0.0", port=8765)
    try:
        asyncio.run(server.start_server())
    except KeyboardInterrupt:
        print("\n⏹️  Stopping server...")
    finally:
        server.cleanup()


if __name__ == "__main__":
    print("=" * 60)
    print("ESP32-S3 Touch LCD 1.46 - PC Streaming Server")
    print("=" * 60)
    main()
