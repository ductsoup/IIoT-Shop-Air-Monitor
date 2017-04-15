# IIoT Shop Air Monitor
An inexpensive, IIoT connected alternative to traditional industrial stack lights and pressure switches. 

## Overview
Typically you'd use a simple pressure switch and a stacklight for a visual indication of the shop air pressure status. The cost is over $500 per unit. 

For under $100 the stack light can be replaced by a Neopixel matrix and the pressure switch with a 0-10V pressure transducer.  A WiFi capable Arduino style microcontroller adds the ability to report to an MQTT broker or a MODBUS TCP master.

![alt text](/images/assembled.jpg)

## Bill of Materials
* [Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/product/3010)
* [Adafruit NeoPixel NeoMatrix 8x8 - 64 RGB LED Pixel Matrix](https://www.adafruit.com/product/1487)
* [ProSense pressure transmitter, 0 to 200 psig range, 0-10 VDC analog output, 14 to 36 VDC operating voltage](https://www.automationdirect.com/adc/Shopping/Catalog/Process_Control_-a-_Measurement/Pressure_Sensors/Pressure_Transmitters/Stainless_Steel_Sensing_Element_-_DIN_Connector/SPT25-10-0200D)
* [Pololu 5V, 5A Step-Down Voltage Regulator D24V50F5](https://www.pololu.com/product/2851)
* [74AHCT125 - Quad Level-Shifter (3V to 5V)](https://www.adafruit.com/product/1787)