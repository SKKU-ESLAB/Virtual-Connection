#!/bin/bash
echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo "600000" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
echo "600000" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq
