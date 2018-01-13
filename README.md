# ZLL-gateway

## Brief
This software aims to provide a complete gateway solution to pilot devices on a
Zigbee network. The gateway is designed to run on a Linux host equipped with a
CC2531 USB dongle.

## How to build it

This project use [Meson](http://mesonbuild.com/) as build system. You will need the following components :
* Meson
* Python 3
* Ninja

Then you can build and execute the project with the following commands :
* meson builddir
* ninja -C builddir
* ./builddir/zll-gateway

The project currently have the following dependencies :
* libznp
* iniparser
* jansson
* eina

ZNP library is basically the ZNP framework which has been built as a dynamic library. You can find on Github, following this link : [ZNP Library](https://github.com/Tropicao/znp-host-framework). Refer to ZNP library README for instructions about how to build it.
If not installed in standard library path, you will need to indicate ZNP library path, as following :  
``` export LD_LIBRARY_PATH=YOUR_CUSTOM_PATH```  
(On most common hosts, if you have installed ZNP library on default path, it will be located in /usr/local/lib)

## How to run it
The gateway software can be run with the following arguments :
* -c <config_path> : specify a custom configuration file. Without this argument, the gateway expects to find a configuration file under /etc/<app_name>/config.ini.
* -r : during startup, completely reset the gateway state. It means that all devices and network configuration will be cleaned

You can also specify the logging level of application with the following :  
``` export LOG_LEVEL=YOUR_LOG_LEVEL ```  
The values of log level can be one between this ones :
* LOG_CRI
* LOG_ERR
* LOG_WARN
* LOG_INF
* LOG_DBG
