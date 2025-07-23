# heart-rate-monitor

## System Hardware Architecture:

### Major Component BOM
1. Sensor: MAX30102 PPG Sensor https://www.analog.com/en/products/max30102.html
2. MCU: ESP32-S3-WROOM-1 https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide.html
3. Computer (Server): Raspberry Pi Zero WH
4. Computer (Client): Anything with a web browser

### Layout
General:
|Sensor + MCU| <---> |Local Server| <---> |Client Computer|


Specific:
|MAX30102 + ESP32 MCU| <---> |Raspberry Pi| <---> |MacOS w/ Web Browser|

### Communication:

MAX30102 to ESP32 MCU: i2c


|MAX30102 + ESP32 MCU| to |Raspberry Pi|: BLE


|Raspberry Pi| to |MacOS w/ Web Browser|: web server over Wi-Fi


## System Software Architecture:
MCU:
1. freeRTOS

Raspberry Pi with 64-bit Raspbian Lite OS:
1. Serve web page with Apache
2. Front end: HTML and JavaScript with Chart.js
3. Backend: Python with bleak and Flask
4. Database: mariaDB (potentially)

