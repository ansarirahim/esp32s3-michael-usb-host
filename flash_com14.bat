@echo off
echo ========================================
echo Flashing to COM14 (Espressif board)
echo Timer Service Stack Fix Applied
echo ========================================
call C:\Users\Abdul\esp-idf-v5.5.1\export.bat
cd C:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host\build
python -m esptool --chip esp32s3 -p COM14 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 bootloader/bootloader.bin 0x10000 esp32s3_michael_usb_host.bin 0x8000 partition_table/partition-table.bin
echo.
echo ========================================
echo Flash complete!
echo ========================================
pause

