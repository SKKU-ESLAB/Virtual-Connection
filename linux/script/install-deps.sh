#!/bin/bash
sudo apt-get update
sudo apt-get install libssl-dev libnl-3-dev libnl-genl-3-dev -y
sudo apt-get install libbluetooth-dev -y

sudo apt-get install udhcpd -y
sudo touch /var/lib/misc/udhcpd.leases
sudo wpa_supplicant -iwlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
