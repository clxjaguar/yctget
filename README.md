# yctget
Program to fetch datas from YC-7x7 dataloggers from YCT 

We needed a datalogger, we found one. But the supplied VB6 program was very crappy, _really_ unusable, even on old XP machines. So I did mine, able to run under windows and linux. All the fun was to reverse the (binary) RS232 protocol. Our is a YC-727D with 2 thermocouple inputs, but the protocol is the same for 1, 2, 3 or 4 inputs models (YC-717, YC-727, YC-737, YC-747). Bytes orders needs to be checked for 3th and 4nd channels tough.
