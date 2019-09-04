#!/usr/bin/python
from subprocess import check_output
import subprocess

cmd = ['sudo', 'wpa_cli', 'status']
out = check_output(cmd)

tokens = out.split("\n")
firstline = tokens[0]
tokens = firstline.split("'")

found = False

for token in tokens:
    if "p2p-wlan" in token:
        wfd_interface_name = token
        cmd = ['sudo', 'wpa_cli', '-i', 'wlan0', 'p2p_group_remove', wfd_interface_name]
        subprocess.call(cmd)
        print "Removed Wi-fi P2P of " + wfd_interface_name
        found = True

if not found:
    print "No Wi-fi P2P"
