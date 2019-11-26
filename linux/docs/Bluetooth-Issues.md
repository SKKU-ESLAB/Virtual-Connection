# How to Address Bluetooth Related Issues

When you run Linux-side app, some Bluetooth-related issues can happen.

I arranged some issues and their solutions as following.

## Check the bluetooth HCI interface

$ hciconfig -a

## Change hci0 interface to Scan Mode

Set page scan and inquiry can (Discoverable)

$ sudo hciconfig hci0 up piscan
