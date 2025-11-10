# 🔧 ESP32 LPC21ISP Programmer

## 📋 PURPOSE
This ESP32 device is a dedicated programmer for flashing firmware to **SwissMicros Voyager Series calculators**, which feature an **LPC1115** CPU. It acts as a USB-to-serial bridge that automatically programs LPC devices using the lpc21isp protocol.

## ⚙️ OPERATION MODES
The device operates in two modes:

### 1. 🔌 CDC HOST MODE (Default)
   - Acts as a USB host controller
   - Connects to USB-to-UART converters (CP210x, FTDI FT23x, CH34x)
   - Automatically detects and programs connected LPC1115 microcontrollers
   - Handles bootloader synchronization automatically
   - Programs firmware from hex files stored in internal storage

### 2. 💾 MSC DEVICE MODE
   - Acts as a USB Mass Storage Device
   - Appears as a removable drive on your computer
   - Allows you to copy hex firmware files to the device
   - Accessible when connected to a computer via USB

## 🔄 SWITCHING BETWEEN MODES
To switch between CDC HOST mode and MSC DEVICE mode:

1. While the device is running, press and **HOLD** the BOOT0 button (GPIO0)
2. Keep the button pressed for **2 seconds**
3. The device will automatically:
   - 💾 Save the new mode to non-volatile memory
   - 🔄 Restart to apply the new mode
   - ✅ Continue operating in the selected mode

The selected mode is saved permanently and will persist across power cycles.
You can switch modes at any time using the BOOT0 button.

> **Note:** The BOOT0 button is typically located on the ESP32 development board.
> On ESP32-S3 DevKitC-1, it's the button labeled "BOOT" or "IO0".

## 💡 LED STATUS INDICATORS
The onboard RGB LED provides visual feedback:

- 🟣 **MAGENTA (Solid):**     Waiting for USB device connection
- 🟡 **YELLOW (Blinking):**   Synchronizing with LPC bootloader
- 🔵 **BLUE (Rapid Blink):**  Programming/flashing firmware in progress
- 🟢 **GREEN (Solid):**       Programming completed successfully
- 🔴 **RED (Solid):**         Error occurred

## 📖 USAGE
1. 📁 Copy your firmware.hex file to the storage partition (via MSC DEVICE mode)
2. 🔌 Connect a USB-to-UART converter (CP210x, FTDI, or CH34x) to the ESP32
3. 🔗 Connect the UART converter to your SwissMicros Voyager calculator (LPC1115)
4. The device will automatically:
   - 🔍 Detect the USB-to-UART converter
   - 🔄 Synchronize with the LPC bootloader
   - ⬇️ Program the firmware.hex file to the calculator
   - 💡 Provide status feedback via the LED

## 📦 FIRMWARE FILES
Place your firmware.hex file in the storage partition. The device will look for
"firmware.hex" by default. Copy your hex file to the partition using MSC DEVICE mode
before programming.

## 🔧 TECHNICAL DETAILS
- 📡 **Communication:** 115200 baud, 8 data bits, 1 stop bit, no parity
- 📋 **Protocol:** lpc21isp
- 💾 **Storage:** FAT filesystem on internal flash
- 🎯 **Target Device:** SwissMicros Voyager Series calculators
- 🔩 **CPU:** LPC1115 microcontroller
- ⏱️ **Sync Timeout:** Up to 3 minutes for bootloader synchronization

For more information, refer to the project documentation.

