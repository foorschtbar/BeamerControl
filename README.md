# BeamerControl

A Arduino/ESP8266/NodeMCU based RS232-MQTT-Bridge to control a beamer via the serviceport

![](.github/beamercontrol.jpg)
![](.github/settings.png)

## Supported Hardware

- Canon LV-Series
- BenQ LX770/LH770

## Case

You can find the case on [Printables](https://www.printables.com/model/465783-beamercontrol-rs232-mqtt-bridge-to-control-a-beame)

![Case](.github/rendering1.png)
![Case](.github/rendering2.png)

## Wiring

![Case](.github/wiring.png)

## MQTT Topics

### Status updates

The device send JSON-formatted status message periodically (can be changed), by request (see command section) or the power state is changes with the button or on webinterface on the following topics:

- `<prefix>/<hostname>/status`

### Commands

The device support a set of commands published on

- `<prefix>/<hostname>/cmd`
- `<prefix>/cmd`

| Command             | Description                          |
| ------------------- | ------------------------------------ |
| `{"poweron":true}`  | Power on beamer                      |
| `{"poweron":flase}` | Shutdown beamer                      |
| `{"pwrstate":"on"}` | Power on beamer                      |
| `{"poweron":"off"}` | Shutdown beamer                      |
| `{"status":"get"}`  | Triggers status push on status topic |
