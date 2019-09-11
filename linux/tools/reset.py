#!/usr/bin/python
from subprocess import check_output
import subprocess

config = {
    'bt_device_name': 'hci0',
    'wifi_device_name': 'wlan0',
    'realtek_mode': False
}


def main():
    # Reset Bluetooth device
    print("* Reset Bluetooth device")
    print("  - OFF")
    cmd = ['sudo', 'hciconfig', config['bt_device_name'], 'down']
    subprocess.call(cmd)
    # TODO: rfkill block

    print("  - ON + PISCAN")
    # TODO: rfkill unblock
    cmd = ['sudo', 'hciconfig', config['bt_device_name'], 'up', 'piscan']
    subprocess.call(cmd)

    # Reset Wi-fi Direct
    print("* Reset Wi-fi Direct")
    print("  - Try to remove Wi-fi P2P groups")
    removed_wifi_p2p_name = try_to_remove_wifi_p2p()
    if removed_wifi_p2p_name:
        print "    (Removed Wi-fi P2P of " + wfd_interface_name + ")"

    if config['realtek_mode']:
        reset_wpa_supplicant()

    print("  - Kill udhcpd")
    cmd = ['sudo', 'killall', 'udhcpd', '-q']
    subprocess.call(cmd)


def try_to_remove_wifi_p2p():
    cmd = ['sudo', 'wpa_cli', 'status']
    out = check_output(cmd)

    tokens = out.split("\n")
    firstline = tokens[0]
    tokens = firstline.split("'")
    for token in tokens:
        if "p2p-wlan" in token:
            wfd_interface_name = token
            cmd = ['sudo', 'wpa_cli', '-i', config['wifi_device_name'],
                   'p2p_group_remove', wfd_interface_name]
            subprocess.call(cmd)
            return wfd_interface_name


def reset_wpa_supplicant():
    print("  - Kill wpa_supplicant")
    cmd = ['sudo', 'killall', 'wpa_supplicant', '-q']
    subprocess.call(cmd)

    print("  - Relaunch wpa_supplicant")
    conf_file = open("p2p.conf", "w")
    conf_file.write("ctrl_interface=/var/run/wpa_supplicant \
            \nap_scan=1 \
            \ndevice_name=SelCon \
            \ndevice_type=1-0050F204-1 \
            \ndriver_param=p2p_device=1 \
            \n\nnetwork={ \
            \n\tmode=3 \
            \n\tdisabled=2 \
            \n\tssid=\"DIRECT-SelCon\" \
            \n\tkey_mgmt=WPA-PSK \
            \n\tproto=RSN \
            \n\tpairwise=CCMP \
            \n\tpsk=\"12345670\" \
            \n}")
    conf_file.close()

    cmd = ['sudo', 'wpa_cli', '-i', '-Dnl80211', '-i',
           config['wifi_device_name'], '-cp2p.conf', '-Bd']
    subprocess.call(cmd)


if __name__ == "__main__":
    main()
