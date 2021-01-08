#include "esphome.h"

struct TuyaCmd
{
  uint8_t cmd;
  uint8_t *data;
  uint8_t data_length;
};

class TuyaMCUComponent : public Component, public UARTDevice
{
public:
  TuyaMCUComponent(UARTComponent *parent) : UARTDevice(parent) {}

  // state
  uint8_t cmd_st = 0;
  long restart_at = 0;
  long start_time = 0;
  bool init_called = false;

  void setup() override
  {
    // init is delayed
    start_time = millis();
  }
  void loop() override
  {
    // start if needed
    if (!init_called && millis() > start_time + 2000) {
      init_called = true;
      init();
    }
    // restart after factory reset
    if (restart_at > 0 && millis() > restart_at)
    {
      restart_at = 0;
      id(restart_sw).turn_on();
      return;
    }

    read_mcu_command();
  }

  void read_mcu_command()
  {
    if (available())
    {
      uint8_t next = read();
      if (cmd_st == 0 && next == 0x55)
      {
        cmd_st = 1;
      }
      else if (cmd_st == 1 && next == 0xaa)
      {
        // read full cmd
        uint8_t version = read();
        uint8_t cmd = read();
        uint8_t data_len_1 = read();
        uint8_t data_len_2 = read();
        uint16_t data_len = (data_len_1 << 8) | data_len_2;
        uint8_t data[data_len];
        for (int i = 0; i < data_len; i++)
        {
          data[i] = read();
        }
        uint8_t checksum = read();
        cmd_st = 0;
        onReceiveTuyaCmd(cmd, data, data_len);
      }
      else if (cmd_st == 1)
      {
        cmd_st = 0;
      }
    }
  }

  void onReceiveTuyaCmd(uint8_t cmd, uint8_t *data, uint16_t data_len)
  {
    // debug print
    // Serial.print("RECV: ");
    // writeHex(cmd);
    // uint8_t dataSizeA = data_len >> 8;
    // uint8_t dataSizeB = data_len & 0x00FF;
    // writeHex(dataSizeA);
    // writeHex(dataSizeB);
    // for (int i = 0; i < data_len; i++)
    // {
    //   writeHex(data[i]);
    // }
    // Serial.println("");

    // battery level
    if (cmd == 0x07 && data_len == 8 && data[0] == 0x10 && data[1] == 0x02 && data[3] == 0x04 && data[7] != 0x00)
    {
      uint8_t level = data[7];
      if (level > 0 && level <= 100)
      {
        id(battery).publish_state(data[7]);
      }
    }
    // ac power failure
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x0F && data[1] == 0x01 && data[3] == 0x01)
    {
      if (data[4] == 0x00)
        id(ac_failure).publish_state(true);
      else if (data[4] == 0x01)
        id(ac_failure).publish_state(false);
    }
    // alarm state
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x01 && data[1] == 0x04 && data[3] == 0x01)
    {
      if (data[4] == 0x00)
        id(alarm_state).publish_state("disarmed");
      else if (data[4] == 0x01)
        id(alarm_state).publish_state("armed_away");
      else if (data[4] == 0x02)
        id(alarm_state).publish_state("armed_home");
    }
    // alarm siren
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x20 && data[1] == 0x04 && data[3] == 0x01)
    {
      if (data[4] == 0x00)
        id(alarm_state).publish_state("disarmed");
      else if (data[4] == 0x01)
        id(alarm_state).publish_state("triggered");
    }
    // device added
    else if (cmd == 0x07 && data_len == 13 && data[0] == 0x26 && data[4] == 0x06)
    {
      uint8_t index = data[6];
      uint8_t type = data[7];
      uint8_t zone = data[8];

      // Serial.print("Device added: ");
      // writeHex(index); // idx
      // writeHex(type);  // type
      // writeHex(zone);  // zone
      // Serial.println("");

      StaticJsonBuffer<2000> jsonBuffer;
      JsonObject &root = jsonBuffer.createObject();
      root["index"] = index;
      root["type"] = type;
      root["zone"] = zone;

      int len1 = root.measureLength();
      char output[len1 + 1];
      root.printTo(output, sizeof(output));
      id(mqttc).publish(App.get_name() + "/accessories/remote/added", output);
    }
    // remote added
    else if (cmd == 0x07 && data_len == 14 && data[0] == 0x26 && data[4] == 0x06)
    {
      uint8_t index = data[7];
      uint8_t type = data[8];
      uint8_t zone = data[9];

      // Serial.print("Remote added: ");
      // writeHex(index); // idx
      // writeHex(type);  // type
      // writeHex(zone);  // zone
      // Serial.println("");

      StaticJsonBuffer<2000> jsonBuffer;
      JsonObject &root = jsonBuffer.createObject();
      root["index"] = index;
      root["type"] = type;
      root["zone"] = zone;
      root["isRemote"] = true;

      int len1 = root.measureLength();
      char output[len1 + 1];
      root.printTo(output, sizeof(output));
      id(mqttc).publish(App.get_name() + "/accessories/added", output);
    }
    // device list
    else if (cmd == 0x07 && data_len >= 7 && data[0] == 0x26 && data[4] == 0x02 && data[5] == 0x00)
    {
      int ndev = data[6];
      StaticJsonBuffer<2000> jsonBuffer;
      JsonArray &arr = jsonBuffer.createArray();
      for (int i = 7; i < 7 + ndev * 7; i += 7)
      {
        uint8_t index = data[i];
        uint8_t type = data[i + 1];
        uint8_t zone = data[i + 2];
        // Serial.print("Device from list: ");
        // writeHex(index); // idx
        // writeHex(type);  // type
        // writeHex(zone);  // zone
        // Serial.println("");

        JsonObject &root = jsonBuffer.createObject();
        root["index"] = index;
        root["type"] = type;
        root["zone"] = zone;
        arr.add(root);
      }
      int len1 = arr.measureLength();
      char output[len1 + 1];
      arr.printTo(output, sizeof(output));
      id(mqttc).publish(App.get_name() + "/accessories/list", output);
    }
    // remote list
    else if (cmd == 0x07 && data_len >= 7 && data[0] == 0x26 && data[4] == 0x02 && data[5] == 0x01)
    {
      int ndev = data[6];
      StaticJsonBuffer<2000> jsonBuffer;
      JsonArray &arr = jsonBuffer.createArray();
      for (int i = 7; i < 7 + ndev * 7; i += 7)
      {
        uint8_t index = data[i];
        uint8_t type = data[i + 1];
        uint8_t zone = data[i + 2];
        // Serial.print("Remote from list: ");
        // writeHex(index); // idx
        // writeHex(type);  // type
        // writeHex(zone);  // zone
        // Serial.println("");

        JsonObject &root = jsonBuffer.createObject();
        root["index"] = index;
        root["type"] = type;
        root["zone"] = zone;
        root["isRemote"] = true;
        arr.add(root);
      }
      int len1 = arr.measureLength();
      char output[len1 + 1];
      arr.printTo(output, sizeof(output));
      id(mqttc).publish(App.get_name() + "/accessories/remote/list", output);
    }
    // factory reset response
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x22 && data[1] == 0x01 && data[3] == 0x01 && data[4] == 0x01)
    {
      restart_at = millis() + 4000;
      id(mqttc).publish(App.get_name() + "/accessories/remote/list", "[]");
      id(mqttc).publish(App.get_name() + "/accessories/list", "[]");
    }
    // settings: exit delay seconds
    else if (cmd == 0x07 && data_len == 8 && data[0] == 0x02 && data[1] == 0x02 && data[3] == 0x04)
    {
      id(exit_delay_seconds).publish_state(data[7]);
    }
    // settings: alarm beep sound
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x0a && data[1] == 0x01 && data[3] == 0x01)
    {
      id(panel_beep_sound).publish_state(data[4]);
    }
    // settings: alarm trigger sound minutes
    else if (cmd == 0x07 && data_len == 8 && data[0] == 0x03 && data[1] == 0x02 && data[3] == 0x04)
    {
      id(alarm_trigger_sound_minutes).publish_state(data[7]);
    }
    // settings: alarm siren sound
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x04 && data[1] == 0x01 && data[2] == 0x00 && data[3] == 0x01)
    {
      id(alarm_siren_sound).publish_state(data[4]);
    }
    // settings: low battery notification
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x11 && data[1] == 0x01 && data[3] == 0x01)
    {
      id(low_battery_notification).publish_state(data[4]);
    }
    // settings: mobile notifications
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x1b && data[1] == 0x01 && data[3] == 0x01)
    {
      id(mobile_notifications).publish_state(data[4]);
    }
    // settings: entry delay seconds
    else if (cmd == 0x07 && data_len == 8 && data[0] == 0x1c && data[1] == 0x02 && data[3] == 0x04)
    {
      id(entry_delay_seconds).publish_state(data[7]);
    }
    // settings: entry/exit delay beep sound
    else if (cmd == 0x07 && data_len == 5 && data[0] == 0x1d && data[1] == 0x01 && data[3] == 0x01)
    {
      id(entry_exit_delay_beep_sound).publish_state(data[4]);
    }
  }

  void init()
  {
    writeHearbeat();
    writeInitSeq();
    writeNetworkOk();
  }

  void writeHearbeat()
  {
    send_cmd(0x00);
  }

  void writeInitSeq()
  {
    send_cmd(0x01); // Querying product information
    send_cmd(0x02); // Querying working mode
    send_cmd(0x08); // Querying Status
  }

  void writeNetworkOk()
  {
    send_1byte_cmd(0x03, 0x02);
    send_1byte_cmd(0x03, 0x02);
    send_1byte_cmd(0x03, 0x03);
    send_1byte_cmd(0x03, 0x04);
  }

  void alarm_armAway()
  {
    uint8_t data[] = {0x01, 0x04, 0x00, 0x01, 0x01};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void alarm_armHome()
  {
    uint8_t data[] = {0x01, 0x04, 0x00, 0x01, 0x02};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void alarm_disarm()
  {
    uint8_t data[] = {0x01, 0x04, 0x00, 0x01, 0x00};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void alarm_trigger()
  {
    uint8_t data[] = {0x01, 0x04, 0x00, 0x01, 0x03};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void alarm_trigger_disarm()
  {
    uint8_t data[] = {0x20, 0x04, 0x00, 0x01, 0x00};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void list_accessories()
  {
    uint8_t data[] = {0x26, 0x00, 0x00, 0x02, 0x02, 0x00};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void list_remotes()
  {
    uint8_t data[] = {0x26, 0x00, 0x00, 0x02, 0x02, 0x01};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void add_accessory()
  {
    uint8_t data[] = {0x26, 0x00, 0x00, 0x02, 0x06, 0xff};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void factory_reset()
  {
    uint8_t data[] = {0x22, 0x01, 0x00, 0x01, 0x01};
    send_data_cmd(0x06, data, sizeof(data));
  }

  void mute_alarm()
  {
    uint8_t data[] = {0x19, 0x01, 0x00, 0x01, 0x01};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. entry delay in seconds (max 255 seconds. alarm does not support more than 255)
  void set_exit_delay_seconds(uint8_t seconds)
  {
    uint8_t data[] = {0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, seconds};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. siren sound enabled
  void set_alarm_siren_sound(bool enabled)
  {
    uint8_t data[] = {0x04, 0x01, 0x00, 0x01, (enabled ? (uint8_t)0x01 : (uint8_t)0x00)};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. alarm trigger siren sound minutes (max 59)
  void set_alarm_trigger_sound_minutes(uint8_t minutes)
  {
    if (minutes > 59)
      minutes = 59;
    else if (minutes < 1)
      minutes = 1;
    uint8_t data[] = {0x03, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, minutes};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. panel beep on arm/disarm
  void set_panel_beep_sound(bool enabled)
  {
    uint8_t data[] = {0x0a, 0x01, 0x00, 0x01, (enabled ? (uint8_t)0x01 : (uint8_t)0x00)};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. low battery notification enabled (useless?)
  void set_low_battery_notification(bool enabled)
  {
    uint8_t data[] = {0x11, 0x01, 0x00, 0x01, (enabled ? (uint8_t)0x01 : (uint8_t)0x00)};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. mobile notifications enabled (useless?)
  void set_mobile_notifications(bool enabled)
  {
    uint8_t data[] = {0x1b, 0x01, 0x00, 0x01, (enabled ? (uint8_t)0x01 : (uint8_t)0x00)};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. entry delay seconds (max 255 seconds. alarm does not support more than 255)
  void set_entry_delay_seconds(uint8_t seconds)
  {
    uint8_t data[] = {0x1c, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, seconds};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // settings. entry/exit delay beep sound enabled
  void set_entry_exit_delay_beep_sound(bool enabled)
  {
    uint8_t data[] = {0x1d, 0x01, 0x00, 0x01, (enabled ? (uint8_t)0x01 : (uint8_t)0x00)};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // set device config
  void set_device_config(uint8_t index, uint8_t type, uint8_t zone)
  {
    uint8_t data[] = {0x26, 0x00, 0x00, 0x09, 0x04, 0x00, index, type, zone, 0xff, 0xff, 0xff, 0xff};
    send_data_cmd(0x06, data, sizeof(data));
  }

  // set device config
  void set_device_config_json(std::string data)
  {
    StaticJsonBuffer<200> jb;
    JsonObject &obj = jb.parseObject(data);
    uint8_t index = obj["index"];
    uint8_t type = obj["type"];
    uint8_t zone = obj["zone"];
    set_device_config(index, type, zone);
  }

  void send_cmd(uint8_t *cmd, int size)
  {
    write_array(cmd, size);
  }

  void send_cmd(uint8_t cmd)
  {
    send_data_cmd(cmd, 0, 0);
  }

  void send_1byte_cmd(uint8_t cmd, uint8_t data)
  {
    uint8_t dddw[] = {data};
    send_data_cmd(cmd, dddw, 1);
  }

  void send_data_cmd(uint8_t cmd, uint8_t *data, uint16_t data_size)
  {
    // write header
    write(0x55);
    write(0xaa);
    // write version
    write(0x00);
    // write command
    write(cmd);
    // write data size
    uint8_t dataSizeA = data_size >> 8;
    uint8_t dataSizeB = data_size & 0x00FF;
    write(dataSizeA);
    write(dataSizeB);
    // write data
    write_array(data, data_size);
    // write checksum
    uint8_t cs = checksum(0x55, 0xaa);
    cs = checksum(cs, cmd);
    cs = checksum(cs, dataSizeA);
    cs = checksum(cs, dataSizeB);
    for (int i = 0; i < data_size; i++)
    {
      cs = checksum(cs, data[i]);
    }
    write(cs);
  }

  void writeHex(uint8_t d)
  {
    char s[3];
    sprintf(s, "%02x ", d);
    Serial.print(s);
  }

  uint8_t checksum(uint8_t acc, uint8_t data)
  {
    return ((uint16_t)acc + (uint16_t)data) % 256;
  }
};