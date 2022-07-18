#!bin/bash

load () {
    echo "  [log] Load module"
    sudo dmesg --clear
    sudo insmod rtcN.ko
    sudo dmesg
}

unload () {
    echo "  [log] Unload module"
    sudo dmesg --clear
    sudo rmmod rtcN.ko
    sudo dmesg
}

check_times () {
    echo ""
    echo "-----RTC_Time----"
    sudo cat /proc/rtcn0
    echo "-----Systime-----"
    sudo date -u '+%H:%M:%S %d-%m-%Y'
    echo "-----------------"
    echo ""
}

dir = $(pwd)

echo "-------------------"
echo "TEST RTCn MODULE v2"
echo "-------------------"
echo "0. Make it"
sudo make
echo
echo "1. Load & Read module:"
load
echo "  [log] check lsmod:"
lsmod | grep rtcN
echo "  [log] Check create /proc and /dev files"
ls /proc | grep rtcn
ls /proc/modules rtcN
ls -la /dev/ | grep rtcn
echo "  [log] Read time in /proc and compare with systime"
check_times
echo "  [do] sleep 5 sec"
sleep 5
echo "  [tm] check again"
check_times
unload
echo ""

echo "2. LOAD WITH PARAMS (x2 speed)"
sudo dmesg --clear
sudo insmod rtcN.ko rtc_mode=2 factor_time=2
sudo dmesg
check_times
echo "  [do] sleep 5 sec..."
sleep 5
echo "  [tm] check again"
check_times
unload
echo ""

echo "3. TIME TEST"
load
check_times
echo "  [do] sleep 2 sec"
sleep 2
echo "  [set] stop rtcn (factor_time = 0)"
sudo echo f 0 > /proc/rtcn0
check_times
echo "  [do] sleep 5 sec"
sleep 5
check_times
echo "  [set] set slow mode and factor = 2"
sudo echo m 1 > /proc/rtcn0
sudo echo f 2 > /proc/rtcn0
check_times
echo "  [do] sleep 6 sec"
sleep 6
check_times
echo "  [set] set normal mode"
sudo echo m 0 > /proc/rtcn0
check_times
echo "  [do] sleep 6 sec"
sleep 6
check_times
echo "  [set] Set new time"
sudo echo t 21:45:00 17.07.2022 > /proc/rtcn0
check_times
echo "  [do] sleep 2 sec"
sleep 2
check_times

unload
echo ""




echo "X. Clean it"
sudo make clean

echo "-------------------"
echo "      FINISH       "
echo "-------------------"
