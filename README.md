<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Firmware for [HARDWARIO STEM](https://stem.hardwario.cloud) Power Controller Kit

[![Travis](https://travis-ci.org/hardwario/twr-radio-stem-power-controller.svg?branch=master)](https://travis-ci.org/hardwario/twr-radio-stem-power-controller)
[![Release](https://img.shields.io/github/release/hardwario/twr-radio-stem-power-controller.svg)](https://github.com/hardwario/twr-radio-stem-power-controller/releases)
[![License](https://img.shields.io/github/license/hardwario/twr-radio-stem-power-controller.svg)](https://github.com/hardwario/twr-radio-stem-power-controller/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/hardwario_en.svg?style=social&label=Follow)](https://twitter.com/hardwario_en)

The strip is divided into 11 fields 0 to 10 (the first 4 LEDs are intended for the lecturer, then there are 2 LEDs as a gap, then continue segments for students 10 x 12 LEDs with 2 LEDs as gaps)

## MQTT

#### Led Strip on Power module

  * Color, standart format "#RRGGBB" or "RRGGBB" and non standart format for white component "#RRGGBB(WW)" or "RRGGBB(WW)"

    Segmet number from 0 to 10
    ```
    mosquitto_pub -t 'node/{id}/led-strip/{segment}/color/set' -m '{color}'
    ```
    example:
    ```
    mosquitto_pub -t 'node/{id}/led-strip/0/color/set' -m '"#250000"'
    mosquitto_pub -t 'node/{id}/led-strip/10/color/set' -m '"#250000(80)"'
    ```

#### LED

  * On
    ```
    mosquitto_pub -t "node/{id}/led/-/state/set" -m true
    ```
  * Off
    ```
    mosquitto_pub -t "node/{id}/led/-/state/set" -m false
    ```
  * Get state
    ```
    mosquitto_pub -t "node/{id}/led/-/state/get" -n
    ```

#### Relay on Power module
  * On
    ```
    mosquitto_pub -t 'node/{id}/relay/-/state/set' -m true
    ```
    > **Hint** First aid:
    If the relay not clicked, so make sure you join 5V DC adapter to Power Module

  * Off
    ```
    mosquitto_pub -t 'node/{id}/relay/-/state/set' -m false
    ```
  * Get state
    ```
    mosquitto_pub -t 'node/{id}/relay/-/state/get' -n
    ```


## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
