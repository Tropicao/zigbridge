# Hue Bulb Demo

This specific version of zll-gateway intends to show capabilities. It uses :
* TI CC2530 with ZNP firmware
* Linux host for software
* Hue Go Lamp

To execute the demo :
* Set the lamp on
* Start app
* You can type enter key to trigger a light switch request. It should be refused
  by gateway since bulb is not installed.
* Type "touchlink" in console : the lamp should blink. It means that it has been
  seen and installed by the gateway
* Press Enter key again : this time the key press should toggle lamp state
* Press "d" and Enter : device will be asked for its endpoints.
* Stop gateway and display "devices.json" : you should observe data about
  installed device
* Restart gateway; wait for complete start, and press Enter multiple times :
  bulb should switch on and off, without requiring a new installation
