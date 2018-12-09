# Zigbridge interfaces

The zigbridge gateway offer various interfaces to communicate with Zigbee devices. There are currently 2 different interfaces :
* an event-based Unix socket interface
* an event-based TCP socket interfaces

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
`./builddir/zigbridge -c YOUR_CONFIG_FILE -r`
* Start proper software tool to use the selected interface (see below to select the proper tool)


## TCP and Unix interfaces
Those two interfaces are basically sockets, which are used by Zigbridge to send JSON events, and to received JSON commands.
To quickly prototype with those two interfaces, you can use the following tools :
* Unix socket interface : you need a tool to link your stdin to the socket input, and to display received data to your stdout. [Socat](https://linux.die.net/man/1/socat) allow you to do so, by linking sockets : Zigbridge is presenting the socket interface throught /tmp/zg_sock, so you can use the following command to start sending and receiving with Unix interface : `socat - /tmp/zg_sock`
* TCP interface : any TCP-capable program can talk to the TCP socket exposed by Zigbridge. For example, you can use [netcat](https://linux.die.net/man/1/nc), and defining a port for Zigbridge in its configuration, you can start using the TCP interface with the following command : `nc 127.0.0.1 $PORT`  
You can then directly type json formatted command to those interfaces to interact with Zigbridge, and read directly in stdout any incoming event.

#### Commands
* **Version** : used to query the gateway software version  
*Example* :
  * Input : `{"command":"version"}`
  * Output : `{"version":"0.4.0"}`
* **Open Network** : used to allow new devices to join for half a minute  
  *Example* :
    * Input : `{"command":"open_network"}`
    * Output : `{"open_network":"ok"}`
* **get_device_list** : used to query the list of installed devices and the corresponding properties  
  *Example* :
    * Input : `{"command":"get_device_list"}`
    * Output : `{"devices":[{"id": 0,"short_addr": 52041,"ext_addr": 6066005677890593, "endpoints": []}]}`
* **Touchlink** : used to initiate a new touchlink procedure. The procedure will return OK if started, or an error if it cannot start or if another touchlink is in progress  
  *Example* :
    * Input : `{"command":"touchlink"}`
    * Output : `{"touchlink":"ok"}`
* **On/Off** : used to toggle on/off a device. The procedure will return 0 if command has been sent, or 1 if device is.
  The targeted device is identified by the "id" field
  unknown
  *Example* :
    * Input : `{"command":"on_off", "data":{"id", 0, "state":"1"}}`
    * Output : `{"on_off":"0"}`

#### Events
* **Button event** : event received when a button is installed and that button is toggled  
  *Example* : `{"event":"button_state","data":{"id":255,"state":0}}`  
* **Temperature event** : event received when a sensor reports a temperature value
  *Example* : `{"event":"temperature","data":{"temperature":2076}}`  
* **Touchlink event** : event received when a touchlink has a new state to notify  
  *Example* : `{"event":"event_touchlink","data":{"status":"finished"}}`

## "Those interfaces are too difficult to use !""
Even if HTTP interface has been the object of a quick implementation in Zigbridge, it has been removed to focus on basic interfaces (TCP, Unix). However, HTTP interface has not been completely left aside but relocated in another project : [zigbridge-api](https://github.com/HornWilly/zigbridge-api). The project is still under development but aims to provide a proper REST interface to Zigbridge.
