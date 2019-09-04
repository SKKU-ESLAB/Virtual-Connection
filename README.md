# Selective Connection
Selective Connection is a framework for abstraction of P2P communications (Wi-fi Direct, Bluetooth, Bluetooth LE, etc.)

# Quick Start
Selective Connection is composed of Linux-side and Android-side programs.

## Linux-side
It is tested on Raspberry Pi 3 running Raspbian Jessie(2016.11 Version).
```
$ git clone https://github.com/SKKU-ESLAB/selective-connection.git
$ cd selective-connection/linux
$ make
```

## Android-side
Import the project from ```android``` directory and build it.

It is tested on Pixel 1 running Android 7.1.

# Prerequisites
On linux-side, it requires ```bluez``` library(```libbluetooth-dev```), ```wpa_supplicant```, ```wpa_cli``` and ```udhcpd```.
For more details, refer to [this page](https://github.com/SKKU-ESLAB/selective-connection/issues/5)

# Detailed Manuals
* [How to install Prerequisites](https://github.com/SKKU-ESLAB/selective-connection/issues/5)
* [How to address Bluetooth-related issues](https://github.com/SKKU-ESLAB/selective-connection/issues/2)
* [How to address Wi-fi Direct-related issues](https://github.com/SKKU-ESLAB/selective-connection/issues/3)
