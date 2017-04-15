# IIoT Shop Air Monitor
An inexpensive, IIoT connected alternative to traditional industrial stack lights and pressure switches. 

## Overview
Typically you'd use a simple pressure switch and a stacklight for a visual indication of the shop air pressure status. The cost is over $500 per unit. That offended me so I looked into more capable alternatives.

For under $100 the stack light can be replaced by a Neopixel matrix and the pressure switch with a 0-10V pressure transducer.  A WiFi capable Arduino style microcontroller adds the ability to report to a MODBUS TCP master or an MQTT broker (exercise for the reader).

![alt text](/images/assembled.jpg)

## Bill of Materials
* [Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/product/3010)
* [Adafruit NeoPixel NeoMatrix 8x8 - 64 RGB LED Pixel Matrix](https://www.adafruit.com/product/1487)
* [ProSense pressure transmitter, 0 to 200 psig range, 0-10 VDC analog output, 14 to 36 VDC operating voltage](https://www.automationdirect.com/adc/Shopping/Catalog/Process_Control_-a-_Measurement/Pressure_Sensors/Pressure_Transmitters/Stainless_Steel_Sensing_Element_-_DIN_Connector/SPT25-10-0200D)
* [Pololu 5V, 5A Step-Down Voltage Regulator D24V50F5](https://www.pololu.com/product/2851)
* [74AHCT125 - Quad Level-Shifter (3V to 5V)](https://www.adafruit.com/product/1787)
* [Integra P6063C Enclosure](https://www.automationdirect.com/adc/Shopping/Catalog/Enclosures_-z-_Subpanels_-z-_Thermal_Management_-z-_Lighting/Enclosures/Padlocking_Enclosures/P6063C)
* [Standoffs](https://www.adafruit.com/product/3299)
* 3D printed sub-panel and mount

## MODBUS Address Map
```
S0_CurrentMillis   40001 // modpoll -m tcp -t 4:float -r 40001 [ip_addr]
S0_RSSI            40003
S0_MODE            40005
S0_THRESH_IDLE     40007 // retained
S0_THRESH_LOW      40009 // retained
S0_THRESH_HIGH     40011 // retained
S1_UPDATE_INTERVAL 40013 // retained
S1_FILTER_WEIGHT   40015 // retained
S1_PSIG_RAW        40017
S1_PSIG_OFFSET     40019
S1_PSIG            40021 // retained
S2_R               40023
S2_G               40025
S2_B               40027
S0_STORE_CONFIG    40029
```
Write the desired values to the retained register then write 255 to S0_STORE_CONFIG to save those settings across power cycles.

## Diffusers
The Neopixel matrix is very bright but also directional. To disperse the light add an optional diffuser. There are two styles; one that fits inside over the LED matrix the enclosure and another mounted outside.

![alt text](/images/diffuser1.jpg)
![alt text](/images/diffuser4.jpg)

## Design Notes
### Power
Industrial sensors typically require 24VDC, microcontrollers 5 or 3.3VDC. The Neopixel matrix could demand up to 4A at 5VDC so we have to start with a 24VDC then use the Pololu regulator to step down to 5V.

### Level Shifting
While powered from 5V the Feather's GPIO operates at 3.3V. In order for it to reliably communicate with the 5V Neopixel matrix we have to add the 74AHCT125 to shift the logic levels.
