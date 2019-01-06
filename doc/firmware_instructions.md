# CC2531 firmware instructions

## Brief
In order to be able to communicate with devices on Zigbee network, the Zigbee Host gateway software must communicate with a USB radio module. This module, which core component is a CC2531 from Texas Instruments, must be flashed with a specific firmware in order to send and receive proper status/commands to/from gateway software

## Hardware
The hardware used in this project is a CC2531 USB stick. CC2531 is a 802.15.4 radio chip from Texas Instruments. It has the same features as CC2530 except it has hardware USB interface. This module can be easily found on official Texas Instruments shop website, or on any electronic online provider.

## Software
In order to be able to communicate with Zigbee devices, CC2531 must be flashed with Texas Instruments' [Z-Stack 3.0](http://www.ti.com/tool/Z-STACK) firmware. Z-Stack is Texas Instruments' Zigbee modular stack implementation. Defining a certain number of build flag allow the user to build the firmware adapted to its needs.

## Tools
In order to build and flash the firmware onto the USB radio head, the following hardware/software tools are needed :

* a CC2531 USB dongle
* a [CC Debugger](http://www.ti.com/tool/CC-DEBUGGER) from Texas Instruments, to reflash the USB dongle with freshly built firmware
* The [IAR Embedded Toolchain for 8051](https://www.iar.com/iar-embedded-workbench/#!?architecture=8051) : it is needed to build the Z-Stack based firmware
* The [SmartFR Flash Programmer](http://www.ti.com/tool/FLASH-PROGRAMMER) from Texas Instruments to flash firmware onto USB dongle. If you are using a Linux development host, you can fall back on [cc-tool project](https://github.com/AlexMekkering/cc-tool) to reflash the dongle.

## instructions
* Download and install all software tools listed above
* Open IAR. In IAR, open GenericApp example in Home Automation project in Z-Stack directory. Example :  
`C:\\Texas Instruments\Z-Stack 3.0.1\Projects\zstack\HomeAutomation\GenericApp\CC2531\GenericApp.eww`
* Next, we will set project to generate proper .hex file.
  * Right-click on project name in left column, select "Options...".
  * In the newly opened window, select "Linker" in the left column
  * In the new display, click on tab "Output"
  * In "Output file" part, make sure that "Override default" check box is checked. In the text option right behind the check box, make sure that the provided filename extension is .hex (example : `GenericApp.hex`)
  * In the "Format" part, make sure that the "Other" radio button is checked.
    * Set the "Output Format" list option to "intel-extended".
    * Set the "Format Variant" list option to "None"
    * Set the "Module-local symbols" list option to "Include all"
* Finally, we will set the build flag to enable all needed features on our radio head
  * If you have closed "Options..." window, open it again.
  * In the left column, select "C/C++ Compiler"
  * In the new display, select the "Preprocessor" tab
  * In the "Defined symbols" list, we will add all our build flags.

The list of needed build flags are the following :

* INT_HEAP_LEN=2170 (should be defined by default)
* NWK_MAX_DEVICE_LIST=0 : there is no need to allocate ressources to an intern device list, since the list will be stored on host size by gateway application
* SECURE=1 : it is needed to allow encrypted communication with device implementing security
* NV_INIT : enable basic persistent data in USB dongle flash
* NV_RESTORE : allow basic network data persistence, needed to rejoin network
  without issue when ZNP is restarted
* ZTOOL_P1 : enable ZTOOL (Texas Instruments tool) messages over CC2531 serial port 1
* MT_TASK : enable Monitor and Test commands, which is the core of communication between host and USB radio. A certain amount of MT sub flags need to be defined too
  * MT_AF_FUNC : enable AF commands
  * MT_AF_CB_FUNC : enable AF callbacks, to receive asynchronous ZDO events or data
  * MT_SYS_FUNC : enable SYS commands
  * MT_UTIL_FUNC : enable UTIL commands
  * MT_ZDO_FUNC : enable ZDO commands
  * MT_ZDO_MGMT : enable ZDO remote device management commands
  * MT_ZDO_EXTENSIONS : enable extended ZDO features like network security
  * ZDO_API_ADVANCED : enable advanced ZDO commands
  * MT_ZDO_CB_FUNC : enable ZDO callbacks, to receive asynchronous ZDO events or data
* INTER_PAN : enable sending of inter-pan broadcast message (needed for touchlink)
* ZCL_READ : fundamental Zigbee cluster, firmware will not build without it
* ZCL_WRITE : fundamental Zigbee cluster, firmware will not build without it
* ZCL_IDENTIFY : fundamental Zigbee cluster, firmware will not build without it
