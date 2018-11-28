# IPC demo

This program aims to provide a basis to integrate the Zigbridge gateway through its IPC interface. It is not a full program, it is only a showcase to be able to test a first integration.

## Devices
In order to test integration, the following devices are needed :
* a CC2531 with proper ZNP firmware (see firmware_instructions.md in doc directory)
* a Xiaomi smart switch

## How To
* Build the gateway program (see general README.md in the project repository) :
  * meson builddir
  * ninja -C builddir
* Edit a configuration file using the example provided in the configuration directory
* Start it with the following arguments :
`LD_LIBRARY_PATH=YOUR_ZNP_LIB_DIR ./builddir/zigbridge -c YOUR_CONFIG_FILE -r`
* The gateway will create a Unix socket in the `/tmp` directory named `/tmp/zg_sock`. You can write to/read from this socket with the following command :
`socat - /tmp/zg_sock`
* The socket is listening for commands in JSON payloads, and send events in same kind of payloads. You can type the proper json string directly in socat to send it.
* Open the network to new devices sending the "Open Network" command
* Install the Xiaomi button (long reset on back pin)
* Toggle the button, you should received button event on socat output

## Commands
* Version : used to query the gateway software version
*Example* :
  * Input : `{"command":"version"}`
  * Output : `{"version":"0.4.0"}`
* Open Network : used to allow new devices to join for half a minute
  *Example* :
    * Input : `{"command":"open_network"}`
    * Output : `{"open_network":"ok"}`
* get_device_list : used to query the list of installed devices and the corresponding properties
  *Example* :
    * Input : `{"command":"get_device_list"}`
    * Output : `{"devices":[{"id": 0,"short_addr": 52041,"ext_addr": 6066005677890593, "endpoints": []}]}`
* Touchlink : used to initiate a new touchlink procedure. The procedure will return OK if started, or an error if it cannot start or if another touchlink is in progress
  *Example* :
    * Input : `{"command":"touchlink"}`
    * Output : `{"touchlink":"ok"}`

## Events
* Button event : event received when a button is installed and that button is toggled
  *Example* : `{"event":"button_state","data":{"id":255,"state":0}}`
Temperature event : event received when a sensor reports a temperature value
  *Example* : `{"event":"temperature","data":{"temperature":2076}}`
Touchlink event : event received when a touchlink has a new state to notify
  *Example* : `{"event":"event_touchlink","data":{"status":"finished"}}`
