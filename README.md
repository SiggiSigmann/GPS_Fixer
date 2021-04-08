# GPS_Fixer
The GPS_Fixer displays the receafed Data from up to two GPS Modules.
This comes in handy if you want to have a fast gps fix befor using the module. Connect the module to the Arduino
and wait for an fix. This means no extra hardware like a Drone or a Computer is needed to valdate the fix.

The GPS Data can be displayed in two modies. Te first mode "One_line mode" shows both GPS Module, the number of Sattelites 
it found and the first digits of the Longtitute and the Langtitute.
The "Mulit_line Mode" only shows information from one Module. It swithes between 4 different infromation pages. So it can
display the infromation with a much higher persission. By using the Potentionmeter( connected to A0 ) the user can choos beween the 4 different pages or a sycle mode. In the scycel mode, a new age will be displyed every 2 seconds.

To switch between the display mde a button connected to Pin 2 can be used. A second button connectet to Pin 3 can be use to change the gps module displyed or updated. In "One_line mode" the module which is currently read is underlined.

# Hardware
An [Arduino Uno](https://www.amazon.de/Arduino-Uno-Rev-3-Mikrocontroller-Board/dp/B008GRTSV6/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=1O4VEK2QHUW18&dchild=1&keywords=arduino+uno&qid=1617870212&sprefix=arduino+uno%2Caps%2C201&sr=8-3) is used in combinateion with a [HD44780 (2 x 16) LCD Display](https://www.amazon.de/AZDelivery-HD44780-Display-Schnittstelle-Hintergrund/dp/B07CQG6CMT/ref=sr_1_1?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=HD44780+Chip&qid=1617870304&sr=8-1). To test a [BN220](https://www.amazon.de/bobotron-3-0V-5-0V-Glonass-Antenne-Eingebautes-Blau/dp/B08P75135L/ref=sr_1_1?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=BN220&qid=1617870342&sr=8-1) was used as an GPS Module. In theory 
also other gps Moudle should work like [BN180](https://www.amazon.de/WIshioT-GPS-Modul-UART-TTL-Dual-Glonass-GPS-Modul-Aircraft-Controller/dp/B07FKRTQYS/ref=sr_1_2?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=BN180&qid=1617870383&sr=8-2) or [BN880](https://www.amazon.de/Shumo-BN-880Q-GPS-Modul-QMC5883L-PIXHAWK-Silber-Blau/dp/B07Z5KZW62/ref=sr_1_5?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=gps+bn&qid=1617870392&sr=8-5) (not tested).

The LCD is connected to Pins rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
GPS1 is connected to rx = 10, tx = 11;
GPS2 is connected to rx = 8, tx = 9;
Potentionmeter for mode selection is connected to A0

![Circuit image](https://raw.githubusercontent.com/SiggiSigmann/GPS_Fixer/main/img/circuit.png)
Made with https://www.circuit-diagram.org
