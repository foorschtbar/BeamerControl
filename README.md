# BeamerControl
A Arduino based MQTT-Bridge to control a beamer via the serviceport

## Usage

## Status updates

The device send JSON-formatted status message periodically (can be changed), by request (see command section) or the power state is changes with the button or on webinterface on the following topics:

- <prefix>/<hostname>/status


### LWT
- TBD

## Commands

The device support a set of commands published on 

- <prefix>/<hostname>/cmd
- <prefix>/cmd

The commands are

- TBD...