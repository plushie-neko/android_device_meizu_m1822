#!/vendor/bin/sh

# The kernel creates both /dev/goodix_fp and /dev/sunwave_fp unconditionally.
# To detect the actual hardware, we read the sunwave chip_info sysfs node.
# If the sunwave chip is not connected, the id reads as 0x0.
sunwave_id=$(cat /sys/devices/virtual/misc/sunwave_fp/chip_info | grep -o 'id[[:space:]]*:[[:space:]]*0x[0-9A-Fa-f]*' | awk '{print $3}')

if [ "$sunwave_id" != "0x0" ] && [ -n "$sunwave_id" ]; then
    setprop ro.hardware.fingerprint sunwave
else
    setprop ro.hardware.fingerprint goodix
fi
