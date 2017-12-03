# ZLL-gateway

## Brief
Experiments with CC2531 USB and ZNP Framework from TI. The final goal is to be able to install and pilot Zigbee devices like Hue lamps from Phillips

## How to build it
You will need to install libznp. this library is basically the ZNP framework
which has been built as a dynamic library. You can find on Github, following
this link : [ZNP Library](https://github.com/Tropicao/znp-host-framework). Refer to ZNP library README for instructions about how to build it.

This project use [Meson](http://mesonbuild.com/) as build system. You will need the following components :
* Meson
* Python 3
* Ninja

Then you can build and execute the project with the following commands :
* meson builddir
* ninja -C builddir
* ./builddir/zll-gateway

If not installed in standard library path, you will need to indicate ZNP library path, as following :  
``` export LD_LIBRARY_PATH=YOUR_CUSTOM_PATH```

You can also specify the logging level of application with the following :  
``` export LOG_LEVEL=YOUR_LOG_LEVEL ```  
The values of log level can be one between this ones :
* LOG_ERROR
* LOG_WARNING
* LOG_INFO
* LOG_INFO_LOWLEVEL
* LOG_VERBOSE
