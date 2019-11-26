# How to Install Prerequisites
Raspbian that will run Linux-side program requires some prerequisite libraries.

## Install Bluetooth Library
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

## Edit wpa_supplicant Daemon's Config
```
$ sudo vi /etc/wpa_supplicant/wpa_supplicant.conf
```

```
update_config=1
driver_param=p2p_device=1
```

## Install udhcpd
```
$ sudo apt-get install udhcpd
$ sudo touch /var/lib/misc/udhcpd.leases
$ sudo wpa_supplicant -iwlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
```
