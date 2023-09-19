# nrf52-sensors
NRF52840DK Integration with the Irrometer 200SS Watermark Sensor

Introduction:
This project focuses on integrating the nrf52840dk board with the Irrometer 200SS Watermark Sensor using nrfconnect. While the current setup allows for some ADC readings, there seems to be an inconsistency in the values obtained. The readings from the ADC are fluctuating and do not stabilize. The provided code in the build directory serves as a foundation, but it requires further refinement, especially concerning the ADC, to achieve accurate results.

Configuration:
To define the GPIO pins for the nrf52840dk board, we utilize the eac_testpin.yaml file, which in turn modifies the nrf52840_dk_nrf52840.dts file.

Circuit Design:
For a clear understanding of the circuit being used, refer to the images provided below:

![test](https://github.com/SimarGhumman/nrf52-sensors/assets/71862263/028942db-8247-4d12-be5d-0cbd703310b6)

Another image for reference is below as well

![IMG_5893](https://github.com/SimarGhumman/nrf52-sensors/assets/71862263/0280508e-7362-460f-97fd-51591422038f)

Additional Information:

We do have working integration with the irrometer 200SS sensor using the nrf52832dk board along with segger studio. I've provided the code in the WM_nrf82532dk.c file along with it's sdk_config.h.

We also have a file in the repo called WM_Read_Uno_New.ino which contains a working integration of the irrometer 200SS sensor using an Arduino Uno R3 along with the Arduino IDE.

Reference and Calibration:
For calibration details, technical specifications, and other relevant information about the Irrometer 200SS Watermark Sensor, please refer to the link below.

https://www.irrometer.com/200ss.html
