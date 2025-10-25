#!/usr/bin/env python3
"""
Quick test script - chạy cả server và client
"""

import asyncio
import websockets
import struct
import math
from threading import Thread
import time

# Server part
async def handle_client(websocket):
    print("✅ ESP32 connected!")
    try:
        async for message in websocket:
            if isinstance(message, bytes):
                if message[:6] == b'AUDIO:':
                    print(f"🔊 Received audio: {len(message)-6} bytes")
                elif message[:6] == b'VIDEO:':
                    print(f"📹 Received video: {len(message)-6} bytes")
    except websockets.exceptions.ConnectionClosed:
        print("❌ ESP32 disconnected")

async def run_server():
    async with websockets.serve(handle_client, "localhost", 8765):
        print("🚀 Server running on ws://localhost:8765")
        await asyncio.Future()

# Client part
async def run_client():
    await asyncio.sleep(2)  # Wait for server
    
    uri = "ws://localhost:8765"
    async with websockets.connect(uri) as websocket:
        print("\n📱 Client connected!")
        
        # Send test audio
        audio_samples = []
        for i in range(320):
            sample = int(32767 * math.sin(2 * math.pi * 1000 * i / 16000))
            audio_samples.append(sample)
        
        audio_data = struct.pack(f'{len(audio_samples)}h', *audio_samples)
        
        # Send test video
        jpeg_data = bytes([0xFF, 0xD8, 0xFF, 0xD9])  # Minimal JPEG
        
        # Stream for 5 seconds
        print("📤 Starting streaming test...")
        for i in range(10):
            await websocket.send(b'AUDIO:' + audio_data)
            if i % 3 == 0:
                await websocket.send(b'VIDEO:' + jpeg_data)
                print(f"   Sent packet {i+1}/10")
            await asyncio.sleep(0.5)
        
        print("✅ Test completed!")

async def main():
    # Run server and client concurrently
    await asyncio.gather(
        run_server(),
        run_client()
    )

if __name__ == "__main__":
    print("=" * 60)
    print("Quick WebSocket Streaming Test")
    print("=" * 60)
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n⏹️  Stopped")
