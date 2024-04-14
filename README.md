# esp-idf-can2socket
CANbus to BSD-Socket bridge using esp32.   
It's purpose is to be a bridge between a CAN-Bus and a BSD-Socket.    
With a rich library of python, you can visualize and perform statistical processing on CAN data.   
This project does not include any visualization or statistical processing.   
Many python visualization and statistical processing samples are available on the Internet.   

# Software requirement
ESP-IDF V4.4/V5.x.   
ESP-IDF V5.1 is required when using ESP32C6.   

# Hardware requirements
- SN65HVD23x CAN-BUS Transceiver   
SN65HVD23x series has 230/231/232.   
They differ in standby/sleep mode functionality.   
Other features are the same.   

- Termination resistance   
I used 150 ohms.   

# Wireing   
|SN65HVD23x||ESP32|ESP32-S2/S3|ESP32-C3/C6||
|:-:|:-:|:-:|:-:|:-:|:-:|
|D(CTX)|--|GPIO21|GPIO17|GPIO0|(*1)|
|GND|--|GND|GND|GND||
|Vcc|--|3.3V|3.3V|3.3V||
|R(CRX)|--|GPIO22|GPIO18|GPIO1|(*1)|
|Vref|--|N/C|N/C|N/C||
|CANL|--||||To CAN Bus|
|CANH|--||||To CAN Bus|
|RS|--|GND|GND|GND|(*2)|

(*1) You can change using menuconfig. But it may not work with other GPIOs.  

(*2) N/C for SN65HVD232



# Test Circuit   
```
   +-----------+   +-----------+   +-----------+ 
   | Atmega328 |   | Atmega328 |   |   ESP32   | 
   |           |   |           |   |           | 
   | Transmit  |   | Receive   |   | 21    22  | 
   +-----------+   +-----------+   +-----------+ 
     |       |      |        |       |       |   
   +-----------+   +-----------+     |       |   
   |           |   |           |     |       |   
   |  MCP2515  |   |  MCP2515  |     |       |   
   |           |   |           |     |       |   
   +-----------+   +-----------+     |       |   
     |      |        |      |        |       |   
   +-----------+   +-----------+   +-----------+ 
   |           |   |           |   | D       R | 
   |  MCP2551  |   |  MCP2551  |   |   VP230   | 
   | H      L  |   | H      L  |   | H       L | 
   +-----------+   +-----------+   +-----------+ 
     |       |       |       |       |       |   
     +--^^^--+       |       |       +--^^^--+
     |   R1  |       |       |       |   R2  |   
 |---+-------|-------+-------|-------+-------|---| BackBorn H
             |               |               |
             |               |               |
             |               |               |
 |-----------+---------------+---------------+---| BackBorn L

      +--^^^--+:Terminaror register
      R1:120 ohms
      R2:150 ohms(Not working at 120 ohms)
```

__NOTE__   
3V CAN Trasnceviers like VP230 are fully interoperable with 5V CAN trasnceviers like MCP2551.   
Check [here](http://www.ti.com/lit/an/slla337/slla337.pdf).


# Installation
```
git clone https://github.com/nopnop2002/esp-idf-can2socket
cd esp-idf-can2socket
idf.py set-target {esp32/esp32s2/esp32s3/esp32c3/esp32c6}
idf.py menuconfig
idf.py flash
```

# Configuration

![config-top](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/13737965-d52a-474c-a327-a35ccce04dc9)
![config-app](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/a238e29e-85eb-42cb-bc54-9d64bc01a25a)

## CAN Setting

![config-can](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/93c94856-e930-47b3-ae66-936d05312baf)


## WiFi Setting

![config-wifi](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/5ff129d4-cd8e-4273-a9cf-7134c39fb84d)


## Socket Setting   
You can select the display format.   

![config-format-text](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/953042da-0de8-42f3-89e9-1a08ca112817)
![format-text](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/835de76f-562c-4b85-a23a-5748f9cb8a80)

![config-format-json](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/070bb0eb-c124-4e4f-87c6-b78f2c36a8c8)
![format-json](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/092f3f7b-44d4-474d-ab96-ab2ce6239954)

![config-format-xml](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/b77cb217-16ae-48aa-b0d0-e3ac9e002d90)
![format-xml](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/36c95acd-bb1a-4168-b218-c12d0d100a80)

![config-format-csv](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/69d6134e-0ead-404a-8594-f19710dc8886)
![format-csv](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/d2005409-edc3-4ce0-91f2-fde87e633791)

You can choose between TCP and UDP protocols.   
![config-protocol-tcp](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/40266a72-f1a8-45ea-a2d1-9ef945136260)
![config-protocol-udp-1](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/554b42ad-7230-4dae-b323-4444d82e92d7)

There are the following four methods for specifying the UDP Address.   

- Limited broadcast address   
 The address represented by 255.255.255.255, or \<broadcast\>, cannot cross the router.   
 Both the sender and receiver must specify a Limited broadcast address.   

- Directed broadcast address   
 It is possible to cross the router with an address that represents only the last octet as 255, such as 192.168.10.255.   
 Both the sender and receiver must specify the Directed broadcast address.   
 __Note that it is possible to pass through the router.__   

- Multicast address   
 Data is sent to all PCs belonging to a specific group using a special address (224.0.0.0 to 239.255.255.255) called a multicast address.   
 I've never used it, so I don't know anything more.

- Unicast address   
 It is possible to cross the router with an address that specifies all octets, such as 192.168.10.41.   
 Both the sender and receiver must specify the Unicast address.

![config-protocol-udp-2](https://github.com/nopnop2002/esp-idf-can2socket/assets/6020549/2c44f1d0-e8bc-48f8-89de-fc9354f5e2ef)


# Python code
If you use the TCP protocol, you can use ```tcp-server.py``` to display the can data.   
If you use the UDP protocol, you can use ```udp-server.py``` to display the can data.   

# Troubleshooting   
There is a module of SN65HVD230 like this.   
![SN65HVD230-1](https://user-images.githubusercontent.com/6020549/80897499-4d204e00-8d34-11ea-80c9-3dc41b1addab.JPG)

There is a __120 ohms__ terminating resistor on the left side.   
![SN65HVD230-22](https://user-images.githubusercontent.com/6020549/89281044-74185400-d684-11ea-9f55-830e0e9e6424.JPG)

I have removed the terminating resistor.   
And I used a external resistance of __150 ohms__.   
A transmission fail is fixed.   
![SN65HVD230-33](https://user-images.githubusercontent.com/6020549/89280710-f7857580-d683-11ea-9b36-12e36910e7d9.JPG)

If the transmission fails, these are the possible causes.   
- There is no receiving app on CanBus.
- The speed does not match the receiver.
- There is no terminating resistor on the CanBus.
- There are three terminating resistors on the CanBus.
- The resistance value of the terminating resistor is incorrect.
- Stub length in CAN bus is too long. See [here](https://e2e.ti.com/support/interface-group/interface/f/interface-forum/378932/iso1050-can-bus-stub-length).

# Reference
https://github.com/nopnop2002/esp-idf-candump

https://github.com/nopnop2002/esp-idf-can2http

https://github.com/nopnop2002/esp-idf-can2mqtt

https://github.com/nopnop2002/esp-idf-can2usb

https://github.com/nopnop2002/esp-idf-can2websocket

https://github.com/nopnop2002/esp-idf-CANBus-Monitor


