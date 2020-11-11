# Virtual Connection
Virtual Connection is a framework for abstraction of P2P communications (Wi-fi Direct, Bluetooth, Bluetooth LE, etc.)

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
For more details, refer to [this page](https://github.com/SKKU-ESLAB/selective-connection/blob/master/docs/How-to-Install-Prerequisites.md)

# Documentations
[Selective Connection Documentations](https://github.com/SKKU-ESLAB/selective-connection/tree/master/docs)

# Publication
* Gyeonghwan Hong, Dongkun Shin, "Virtual Connection: Selective Connection System for Energy-Efﬁcient Wearable Consumer Electronics", Nov. 2020, Early Access, IEEE Transactions on Consumer Electronics: [Link](https://ieeexplore.ieee.org/document/9247261)
