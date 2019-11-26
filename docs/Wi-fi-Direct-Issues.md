# How to Address Wi-fi Direct Related Issues
When you run Linux-side app, some Wi-fi Direct-related issues can happen.

I arranged some issues and their solutions as following.

## Check the wifi module support p2p
```
$ iw list
```
## Remove the P2P group (Wifi-direct) on the wifi interface

remove group ```p2p-wlan0-0``` on the wifi interface ```wlan0```
```
sudo wpa_cli -i wlan0 p2p_group_remove p2p-wlan0-0
```
