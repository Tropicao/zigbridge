# Script utils

## Why
This set of scripts is meant to be used with an official Hue bridge, for debug purposes.

## What
This directory contains the following elements :
* bridge.py : main class used as interface with the bridge
* scripts which use bridge.py to execute specific actions
  * get_light.py : get list of installed lights
  * set_light.py: delete light present in index 1 of bridge
  * set_touchlink : initiate touchlink procedure to install lamp  

## How
Simply call one of the script, without argument. If it is the first time that you use the bridge API, bridge.py will first create a user, resulting in a file named username.txt. This file will then be used each time you call the utils script. If you need to reset to factory the bridge, **please remember to delete this file**, in order to make bridge.py generate a new user.
