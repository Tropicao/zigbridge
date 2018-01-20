# Hue Bulb Demo

This specific version of zll-gateway intends to show capabilities. It uses :
* TI CC2530 with ZNP firmware
* Linux host for software
* [Hue Go Lamp](https://www2.meethue.com/fr-fr/p/hue-white-and-color-ambiance-hue-go-lampe-a-poser/7146060PH)
* [Xiaomi smart switch](https://xiaomi-mi.com/sockets-and-sensors/xiaomi-mi-wireless-switch/)

To execute the demo :
* Plug the lamp
* Start gateway app doing a reset (-r)
* You can type enter key to trigger a light switch request. It should be refused
  by gateway since bulb is not installed.
* Type "touchlink" in console : the lamp should blink. It means that it has been
  seen and reset by gateway. It will automatically join our local network.
* Press Enter key again : this time the key press should toggle lamp state
* Do a long press on Xiami switch reset pin (blue led will blink, release when it stops blinking)
* Push Xiaomi button multiple times : it will toggle the Philips bulb
