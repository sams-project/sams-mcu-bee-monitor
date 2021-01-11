# SAMS Project
SAMS is a project funded by the European Union within the H2020-ICT-39-2016-2017 call. SAMS enhances international cooperation of ICT (Information and Communication Technologies) and sustainable agriculture between EU and developing countries in pursuit of the EU commitment to the UN Sustainable Development Goal "End hunger, achieve food security and improved nutrition and promote sustainable agriculture". To get more information about the SAMS project please visit: https://sams-project.eu/

# SAMS bee colony monitoring system based on ESP8266
Bee colony monitoring system developed within SAMS project. The monitoring system's core element is the ESP8266 microchip (https://www.espressif.com/en/products/socs/esp8266). The system measures temperature, humidity and weight. Collected data is sent to the remote SAMS data warehouse (DW) (https://sams.science.itf.llu.lv/), afterwards the ESP8266 enters deep sleep, which also determines the measurement interval. This interval can be easily changed in the `config.h` by changing the value of `SLEEPTIME`. For more information about the `config.h`, refer to section [Configuration file](#configuration-file).

# Tested ESP8266 platforms
In general the code should work on any platform that has the ESP8266 microchip with the necessary break-out pins. Originally, the monitoring system was based on the **ESP8266 ESP-12E** variant (e.g., https://components101.com/wireless/esp12e-pinout-datasheet) with the adapter/ break-out board.

Besides the ESP8266 ESP-12E with adapter board, the code has been successfully tested on the following boards:
* Lolin NodeMCU v3;
* WEMOS D1 mini;
* Adafruit HUZZAH ESP8266 Breakout.

# Contents of the repository
The repository consists of two projects:
* *SAMS_mcu_bee_monitor* - runs the monitoring system, collects sensor data and sends to remote SAMS DW;
* *LoadCellCalibration* - for load cell calibration.

# Sensors and modules used
List of sensors and modules that the monitoring system uses:
* DHT22 temperature and humidity sensor;
* HX711 load cell amplifier. There are different modules that incorporates this 24-bit analog-to-digital (ADC) converter. The monitoring system was tested with these modules: Okystar (https://www.tme.eu/en/details/oky3478-1/other-sensors/okystar/) and SparkFun (https://www.sparkfun.com/products/13879);
* H30A single point load cell. This load cell is the one that was initially used, but any other single point load cell model can be used;
* DS18B20 (or DS18S20) digital thermometer.

Refer to [datasheets](#references) below.

# Load cell calibration
To calibrate the load cell, first upload the *LoadCellCalibration* code to the ESP8266 in order to obtain the following values: *Offset* and *ScaleFactor*. These values then need to be included in the `config.h` file under *SAMS_mcu_bee_monitor*. The according variables are called: `scaleOffset` and `scaleFactor`. The calibration process is menu-guided through a **Serial** port and describes the necessary steps that should be taken.

# Libraries
* RTCVars: https://github.com/highno/rtcvars. Necessary for storing data in the real time clock (RTC) memory between sleep cycles. It is needed to change the `RTC_MAX_VARIABLES` in `RTCVars.h` (under the Arduino libraries folder) to `470`;
* ArduinoJson: https://github.com/bblanchon/ArduinoJson. Necessary to ease the data sending and receiving in a JSON format in an efficient way;
* DHT sensor library: https://github.com/adafruit/DHT-sensor-library. Needed for the DHT22 sensor to obtain temperature and humidity data;
* HX711: https://github.com/bogde/HX711. Helps to obtain weight data from a load cell via HX711 24-bit ADC.

# Configuration file
The `config.h` file contains configuration for the monitoring system. 

### Deep sleep and measurement interval
The variable `SLEEPTIME` sets the time the ESP8266 is in deep sleep mode. This also determines the measurement interval.

### WiFi credentials
Since ESP8266 transfers data wirelessly through a WiFi connection, according credentials need to be inserted. For the WiFi part, fill in the empty "" with the router's SSID (`static const char ssid[] PROGMEM = "";`) and password (`static const char pwd[] PROGMEM = "";`);

### Device credentials
Credentials needed for device authentication and authorization. By using the `client_id` and `secret` the device obtains an access token that is used for data sending (the access token expiration time by default is set to 24h). Fill in the empty "" with the according client ID (`static const char client_id[] PROGMEM = "";`) and secret (`static const char secret[] PROGMEM = "";`).

In order to obtain credentials for your device, please contact SAMS team via [LinkedIn](https://www.linkedin.com/groups/8960651/).

### Enabled sensors
This section sets the sensors that are enabled (1) or disabled (not connected) (0).
* `DHT_ENABLED` refers to DHT22 sensors;
* `SCALE_ENABLED` refers to HX711 ADC;
* `DS18_ENABLED` refers to DS18B20 or DS18S20;
* `BAT_ENABLED` should be set to 1, if battery voltage is being measured via the ESP8266 analog (marked `ADC` or `A0`) pin.

### Sensor related configs
#### DHT22
The `DHTPIN` determins the data pin the sensor is connected to (in this case GPIO12); the `DHTTYPE` determines the sensor type, needed for the `DHT.h` library.
#### HX711
The `DOUT` and `SCK` determines the GPIO # for the pins on the respective HX711 module. The values for `scaleFactor` and `scaleOffset` should be obtained by using the *LoadCellCalibration* program.
#### DS18x20
The `DSPIN` determines the according data pin for DS18B20 or DS18S20 type sensors. The `DS_SENSOR_COUNT` is just some hardcoded maximum sensor count, since the DS18B20 (and DS18S20) uses the 1-Wire protocol, hence multiple such sensors can be connected to one data line.
#### Voltage measurement
The `voltageFactor` is used for converting ADC value to volts. Needs to be changed accordingly, taking into account the resistor values used for the voltage divider. For `R1 = 430k; R2 = 82k; Vin_max = 6.24V;` this value is set as `5.4f`.

# References
Useful information and materials:
* [Suggestions to reduce ESP8266 power consumption](https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-1/) by OppoverBakke (Erik H. Bakke);
* [JSON library for embedded systems](https://arduinojson.org/);
* [DHT22 sensor](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf);
* [HX711 load cell amplifier](https://cdn.sparkfun.com/assets/b/f/5/a/e/hx711F_EN.pdf);
* [H30A single point load cell](https://www.bosche.eu/en/scale-components/load-cells/single-point-load-cell/single-point-load-cell-h30a);
* [DS18B20](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) and [DS18S20](https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf) digital thermometer.

# Other open-source software and hardware solutions within SAMS
Check out also the open-source SAMS beehive monitoring system based on Raspberry Pi Zero W: https://github.com/sams-project/sams-app. The SAMS DW (a universal system, which is able to operate with different data inputs and have flexible data processing algorithms) source code can be found under: https://github.com/sams-project.
