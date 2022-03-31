# MQTT protocol

This file describes the MQTT protocol needed to communicate with the Wi-Fi Alarm siren.

## Alarm control
  * `wifi_alarm/alarm/command` receives alarm arm/disarm commands
    * `arm_away`: alarm is armed away
    * `arm_home`: alarm is armed home
    * `disarm`: alarm is disarmed (if alarm is triggered, this will stop it)
    * `trigger`: this will trigger the alarm from an external source (e.g. home assistant)
  * `wifi_alarm/alarm/state` publishes the alarm state changes
    * `armed_away`, `armed_home`, `disarmed`, `triggered`

## Alarm settings
All alarm settings can be changed through MQTT
  * `alarm_siren_sound`: when `triggered`, does the alarm sound?
    * set: send `on`/`off` to `wifi_alarm/setting/alarm_siren_sound/set`
    * get: `wifi_alarm/setting/alarm_siren_sound/state`
  * `panel_beep_sound`: when the alarm state changes, does it emit a beep sound?
    * set: send `on`/`off` to `wifi_alarm/setting/panel_beep_sound/set`
    * get: `wifi_alarm/setting/panel_beep_sound/state`
  * `exit_delay_seconds`: Seconds before the alarm is actually armed on `arm_away`.
    * set: send seconds to `wifi_alarm/setting/exit_delay_seconds/set`
    * get: `wifi_alarm/setting/exit_delay_seconds/state`
  * `alarm_trigger_sound_minutes`: when triggered, how long does the siren sound (in minutes) range [1, 59]
    * set: send minutes to `wifi_alarm/setting/alarm_trigger_sound_minutes/set`
    * get: `wifi_alarm/setting/alarm_trigger_sound_minutes/state`
  * `entry_delay_seconds`: when alarm is `armed_away`, how long to wait (in seconds) before the alarm is `triggered`. Does only affect to devices attached to the alarm.
    * set: send minutes to `wifi_alarm/setting/entry_delay_seconds/set`
    * get: `wifi_alarm/setting/entry_delay_seconds/state`
  * `entry_exit_delay_beep_sound`: when `on`, the alarm will beep on "grace" time.
    * set: send `on`/`off` to `wifi_alarm/setting/entry_exit_delay_beep_sound/set`
    * get: `wifi_alarm/setting/entry_exit_delay_beep_sound/state`

There are other settings that can be changed (`low_battery_notification`, `mobile_notifications`) but are useless.

## Accessories (optional)

> :warning: **This may not work properly on latest hardware**: Use it with caution.

This siren has the ability to register some 433Mhz devices as door/window sensors, PIR sensors, smoke sensors, etc, and remotes like keypads.
  * `wifi_alarm/accessories/command`
    * `add`: the siren will be set on pairing mode. Make your device to emit some code and will be added to the siren. The device added will be published to `wifi_alarm/accessories/remote/added`
    * `list`: a json list describing all paired devices will be published on `wifi_alarm/accessories/list`
    * `remotes`: a json list describing all paired remotes will be published on `wifi_alarm/accessories/remote/list`
Each device has 3 attributes:
  * `index`: the device index
  * `type`: device type (`0` is door/window sensor, `1` is PIR, `10` is vibration sensor, etc...)
  * `zone`: zone type (this defines the behavior of the siren if this device is triggered)

### Zones:
  * `0`: normal
  * `1`: 24 hours. the alarm will always trigger, even on `disarmed` state. 
  * `2`: delay. the alarm will trigger after some time (`entry_delay_seconds`)
  * `3`: home. the alarm will immediately trigger only when `armed_away`.
  * `4`: 24 hours silent (useless because notifications are ignored)
  * `5`: home with delay. the alarm will trigger after some time (`entry_delay_seconds`) only when `armed_away`.
  * `6`: off

### Change device zone
Send a json string to `wifi_alarm/accessories/set_json` with this format:
```javascript
{
  "index": 0, // device index
  "type": 2, // device type (useless)
  "zone": 3, // zone = home
}
```

## Factory reset
This is the only way to remove any accessory (device or remote).

Just send `factory_reset` to `wifi_alarm/alarm/factory_reset`.