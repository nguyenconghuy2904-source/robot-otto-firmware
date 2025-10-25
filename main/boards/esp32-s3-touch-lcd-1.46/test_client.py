#!/usr/bin/env python3
"""
Test client để kiểm tra WebSocket server
Giả lập ESP32 gửi audio/video
"""

import asyncio
import websockets
import struct
import math

async def test_connection():
    uri = "ws://localhost:8765"
    
    try:
        async with websockets.connect(uri) as websocket:
            print("✅ Connected to server!")
            
            # Test 1: Send audio
            print("\n📤 Test 1: Sending audio packet...")
            # Tạo sine wave 1kHz, 320 samples @ 16kHz (20ms)
            audio_samples = []
            for i in range(320):
                sample = int(32767 * math.sin(2 * math.pi * 1000 * i / 16000))
                audio_samples.append(sample)
            
            # Pack to bytes (int16)
            audio_data = struct.pack(f'{len(audio_samples)}h', *audio_samples)
            message = b'AUDIO:' + audio_data
            await websocket.send(message)
            print(f"   Sent {len(message)} bytes audio")
            
            await asyncio.sleep(1)
            
            # Test 2: Send video
            print("\n📤 Test 2: Sending video frame...")
            # Fake JPEG header
            jpeg_data = bytes([
                0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46,
                0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
                0x00, 0x01, 0x00, 0x00, 0xFF, 0xD9
            ])
            message = b'VIDEO:' + jpeg_data
            await websocket.send(message)
            print(f"   Sent {len(message)} bytes video")
            
            await asyncio.sleep(1)
            
            # Test 3: Continuous streaming (5 seconds)
            print("\n📤 Test 3: Continuous streaming (5 seconds)...")
            for i in range(25):  # 5s @ 200ms interval
                # Send audio
                audio_data = struct.pack(f'{len(audio_samples)}h', *audio_samples)
                await websocket.send(b'AUDIO:' + audio_data)
                
                # Send video every 5 packets (1 second)
                if i % 5 == 0:
                    await websocket.send(b'VIDEO:' + jpeg_data)
                    print(f"   [{i//5}/5] Sent audio + video")
                
                await asyncio.sleep(0.2)
            
            print("\n✅ All tests completed!")
            
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    print("=" * 60)
    print("ESP32 WebSocket Test Client")
    print("=" * 60)
    asyncio.run(test_connection())
