adb wait-for-device push libled.so /uni_data/uniapp/lib/
rem adb wait-for-device push test /uni_data
rem adb wait-for-device shell chmod +x /uni_data/test
rem adb wait-for-device shell "cd /uni_data && ./test"
rem pause
adb wait-for-device shell reboot