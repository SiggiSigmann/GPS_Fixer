# GPS_Fixer
Can be used to find a gps fix for GPS moduled (tested with BN220)
There are two display modes. One displays both Modules but only Longtitute and langtitute. In the second mode more infromation willbe displayed but only for one GPS Module at the time.

# Hardware
The LCD is connected to Pins rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
GPS1 is connected to rx = 10, tx = 11;
GPS2 is connected to rx = 8, tx = 9;

![Circuit image](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/circuit.png)
Made with https://www.circuit-diagram.org

# Bug
After choosing another Module the display prints Missing even it is not true. After the next refresh cycle this it will display the values correctly.