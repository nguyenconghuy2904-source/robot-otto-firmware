@echo off
echo Creating merged binary file...

rem Check if required files exist
if not exist "build\bootloader\bootloader.bin" (
    echo Error: bootloader.bin not found!
    exit /b 1
)

if not exist "build\partition_table\partition-table.bin" (
    echo Error: partition-table.bin not found!
    exit /b 1
)

if not exist "build\xiaozhi.bin" (
    echo Error: xiaozhi.bin not found!
    exit /b 1
)

if not exist "build\ota_data_initial.bin" (
    echo Error: ota_data_initial.bin not found!
    exit /b 1
)

if not exist "build\generated_assets.bin" (
    echo Error: generated_assets.bin not found!
    exit /b 1
)

rem Create a temporary Python script to merge binaries
echo import struct > merge_bins.py
echo.>> merge_bins.py
echo def create_merged_bin(): >> merge_bins.py
echo     # Define flash layout >> merge_bins.py
echo     files = [ >> merge_bins.py
echo         (0x0, 'build/bootloader/bootloader.bin'), >> merge_bins.py
echo         (0x8000, 'build/partition_table/partition-table.bin'), >> merge_bins.py
echo         (0xd000, 'build/ota_data_initial.bin'), >> merge_bins.py
echo         (0x20000, 'build/xiaozhi.bin'), >> merge_bins.py
echo         (0x800000, 'build/generated_assets.bin'), >> merge_bins.py
echo     ] >> merge_bins.py
echo.>> merge_bins.py
echo     # Find the maximum address to determine output size >> merge_bins.py
echo     max_addr = 0 >> merge_bins.py
echo     for addr, filename in files: >> merge_bins.py
echo         with open(filename, 'rb') as f: >> merge_bins.py
echo             data = f.read() >> merge_bins.py
echo             max_addr = max(max_addr, addr + len(data)) >> merge_bins.py
echo.>> merge_bins.py
echo     # Create output buffer >> merge_bins.py
echo     output = bytearray(max_addr) >> merge_bins.py
echo.>> merge_bins.py
echo     # Copy each file to the correct offset >> merge_bins.py
echo     for addr, filename in files: >> merge_bins.py
echo         print(f'Adding {filename} at 0x{addr:x}') >> merge_bins.py
echo         with open(filename, 'rb') as f: >> merge_bins.py
echo             data = f.read() >> merge_bins.py
echo             output[addr:addr+len(data)] = data >> merge_bins.py
echo.>> merge_bins.py
echo     # Write merged binary >> merge_bins.py
echo     with open('merged-firmware.bin', 'wb') as f: >> merge_bins.py
echo         f.write(output) >> merge_bins.py
echo.>> merge_bins.py
echo     print(f'Merged binary created: merged-firmware.bin ({len(output)} bytes)') >> merge_bins.py
echo.>> merge_bins.py
echo if __name__ == '__main__': >> merge_bins.py
echo     create_merged_bin() >> merge_bins.py

rem Run the Python script
python merge_bins.py

rem Clean up
del merge_bins.py

echo.
echo Merged binary created successfully!
echo You can flash it using: esptool.py --chip esp32s3 write_flash 0x0 merged-firmware.bin