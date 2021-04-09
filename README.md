# GPS_Fixer
The GPS_Fixer displays the received Data from up to two GPS Modules. This comes in handy if you want to have a fast GPS worm up before using the module. Connect the module to the Arduino and wait for a GPS fix. So you can put your GPS Moudluse outside without the need to use your Drone or other devices. This means you don't have to worry or supervise your device.

The GPS Data can be displayed in two modes. The first mode "One_line mode" shows both the GPS Module, the number of Satellites it found, and the first digits of the longitude and the latitude. E.g:

![One_Line Mode](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/O.png)


The "Mulit_line Mode" only shows information from one Module. It switches between 4 different information pages. So it can display the information with much higher permission. By using the Potentiometer( connected to A0 ) the user can choose between the 4 different pages or a cycle mode. In the cycle mode, a new age will be displayed every 2 seconds.
First frame for GPS Module 2:

![Multi_line Mode Frame 1 for GPS 2](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/M_2_1.png)

Frame 2,3,4 for GPS Module 1:

![Multi_line Mode Frame 2 for GPS 1](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/M_1_2.png)
![Multi_line Mode Frame 3 for GPS 1](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/M_1_3.png)
![Multi_line Mode Frame 4 for GPS 1](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/M_1_4.png)

To switch between the display mode a button connected to Pin 2 can be used. A second button connected to Pin 3 can be used to change the GPS module displayed or updated. In "One_line mode" the module which is currently read is underlined.

# Customizing
To customice the Programm three constants can be used:
1. **WAITFORSIGNALTIME**: Default: 5000 (5 Seconds) Detect a GPS Module as missing if no Data were received in this time span.

2. **MULTIMODETIME**: Default: 200 (2 Seconds) Describe how longe a page in .Multi_Display Mode will be displayed. Here after two seconds the next page will be displayed.

3. **METRICS**: Describe in which units the value will be displayed (0 = metric, 1 = imperial)

4. **GPSNUMBER**: Describe how many GPS Modules are connected. Default is 2. More is not possible with the [Arduino Uno](https://www.amazon.de/Arduino-Uno-Rev-3-Mikrocontroller-Board/dp/B008GRTSV6/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=1O4VEK2QHUW18&dchild=1&keywords=arduino+uno&qid=1617870212&sprefix=arduino+uno%2Caps%2C201&sr=8-3) because all pins are in use. The only option would be to change the Display to I2C communication. Each GPSModule must be initialized by using the method *void initGPSStruct(SoftwareSerial* soft)*. This method needs an instance of a Softwareserial connection as shown in the following.

```cpp
//rx, tx
SoftwareSerial port3(10, 11);

void setup() {
	...

	//GPS
  	initGPSStruct3(&port);

	...
}
```
5. **TIMEOFFSET**: To customice your timzone the TIMEOFFSET constant can be used. The value describe the hour offset to UTC time. A list of Offsets can be found on [Wikipedia](https://en.wikipedia.org/wiki/List_of_UTC_time_offsets)

6. **SUMMERTIME**: Describe if it is Summer or Winter Time. 0 means Wintertime and nothing is added to the hour. 1 means summer time and therefor one hour will be added.

# Hardware
An [Arduino Uno](https://www.amazon.de/Arduino-Uno-Rev-3-Mikrocontroller-Board/dp/B008GRTSV6/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=1O4VEK2QHUW18&dchild=1&keywords=arduino+uno&qid=1617870212&sprefix=arduino+uno%2Caps%2C201&sr=8-3) is used in combination with a [HD44780 (2 x 16) LCD Display](https://www.amazon.de/AZDelivery-HD44780-Display-Schnittstelle-Hintergrund/dp/B07CQG6CMT/ref=sr_1_1?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=HD44780+Chip&qid=1617870304&sr=8-1). To test a [BN220](https://www.amazon.de/bobotron-3-0V-5-0V-Glonass-Antenne-Eingebautes-Blau/dp/B08P75135L/ref=sr_1_1?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=BN220&qid=1617870342&sr=8-1) was used as a GPS Module. In theory, also other GPS Modules should work like [BN180](https://www.amazon.de/WIshioT-GPS-Modul-UART-TTL-Dual-Glonass-GPS-Modul-Aircraft-Controller/dp/B07FKRTQYS/ref=sr_1_2?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=BN180&qid=1617870383&sr=8-2) or [BN880](https://www.amazon.de/Shumo-BN-880Q-GPS-Modul-QMC5883L-PIXHAWK-Silber-Blau/dp/B07Z5KZW62/ref=sr_1_5?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=gps+bn&qid=1617870392&sr=8-5) (not tested). Also, some capacitors for debouncing and resistors aside some potentiometers are needed.

GPS1 is connected to rx = 10, tx = 11;
GPS2 is connected to rx = 8, tx = 9;

![Circuit image](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/circuit.png)
Made with https://www.circuit-diagram.org