# Selective Connection
Selective Connection is a framework for abstraction of P2P communications (Wi-fi Direct, Bluetooth, Bluetooth LE, etc.)

# Quick Start
Selective Connection is composed of Linux-side and Android-side programs.

## Linux-side
It is tested on Raspberry Pi 3 running Raspbian Jessie(2016.11 Version).
```
$ git clone https://github.com/SKKU-ESLAB/selective-connection.git
$ cd selective-connection
$ make
```

## Android-side
Import the project from ```android``` directory and build it.

It is tested on Pixel 1 running Android 7.1

## Pre-requisite for Linux-side
Raspbian that will run Linux-side program requires some prerequisite libraries.

### Install Bluetooth Library
```
$ sudo apt-get install libbluetooth-dev
```

Since this project requires legacy IPC to communicate with Bluetooth service, you should execute Bluetooth service with legacy flag. You need to edit your systemd config file.
```
$ sudo vi /etc/systemd/system/dbus-org.bluez.service
```

```
ExecStart=/usr/lib/bluetooth/bluetoothd --compat
```

```
$ sudo systemctl daemon-reload
$ sudo systemctl restart bluetooth
$ sudo chmod 777 /var/run/sdp
```

### Edit wpa_supplicant Daemon's Config
```
$ sudo vi /etc/wpa_supplicant/wpa_supplicant.conf
```

```
update_config=1
driver_param=p2p_device=1
```

### Install udhcpd
```
$ sudo apt-get install udhcpd
$ sudo touch /var/lib/misc/udhcpd.leases
$ sudo wpa_supplicant -iwlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
```

# Notes
## Bluetooth Issues
### Check the bluetooth HCI interface
```
$ hciconfig -a
```

### Change hci0 interface to Scan Mode 
Set page scan and inquiry can (Discoverable)
```
$ sudo hciconfig hci0 up piscan
```

## Wifi-Direct Issues
### Check the wifi module support p2p
```
$ iw list
```
### Remove the P2P group (Wifi-direct) on the wifi interface
remove group ```p2p-wlan0-0``` on the wifi interface ```wlan0```
```
sudo wpa_cli -i wlan0 p2p_group_remove p2p-wlan0-0
```
