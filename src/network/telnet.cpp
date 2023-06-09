#include "telnet.h"

#include <Arduino.h>
#include <ESPTelnet.h>
#include <WString.h>

#include "../board_preference.h"
#include "../log.h"
#include "../sensor_helper.h"
#include "../utils.h"
#include "../version.h"
#include "wifi.h"

static String string_divide_by(String *s, char delim) {
  size_t i = 0;
  while (i < s->length() && (*s)[i] != delim) {
    i++;
  }
  String result = s->substring(0, i);
  *s            = s->substring(i);
  return result;
}

typedef void (*cmd_on_run)(String cmd);

struct TelnetCommand {
  String     name;
  cmd_on_run run;
  String     help;
};

static ESPTelnet telnet;

static void cmd_sensor_info(String);
static void cmd_sensor_list(String);
static void cmd_sensor_add(String);
static void cmd_sensor_rm(String);
static void cmd_board_host(String);
static void cmd_board_location(String);
static void cmd_board_room(String);
static void cmd_mac_address(String);
static void cmd_temp_offset(String);
static void cmd_uptime(String);
static void cmd_ping(String);
static void cmd_reboot(String);
static void cmd_version(String);
static void cmd_quit(String);
static void cmd_help(String);

static const TelnetCommand commands[] = {
    {"sensor-info",    cmd_sensor_info,    "retrieve the values from all available sensors"        },
    {"sensor-list",    cmd_sensor_list,    "list the available sensors"                            },
    {"sensor-add",     cmd_sensor_add,     "mark a sensor as available"                            },
    {"sensor-rm",      cmd_sensor_rm,      "mark a sensor as no longer available"                  },
    {"board-host",     cmd_board_host,     "get or set board's host name"                          },
    {"board-location", cmd_board_location, "get or set board's location"                           },
    {"board-room",     cmd_board_room,     "get or set board's room"                               },
    {"board-mac",      cmd_mac_address,    "get or set MAC address (off to use default)"           },
    {"temp-offset",    cmd_temp_offset,    "get or set offset subtracted by each temperature value"},
    {"uptime",         cmd_uptime,         "get the time elapsed since the board's boot"           },
    {"ping",           cmd_ping,           "ping the board"                                        },
    {"reboot",         cmd_reboot,         "reboot the board"                                      },
    {"version",        cmd_version,        "show current firmware version"                         },
    {"quit",           cmd_quit,           "exit"                                                  },
    {"help",           cmd_help,           "show this help message"                                },
};

static void cmd_sensor_info(String _) {
  telnet.println("Values from available sensors:");
  print_available_sensors_info([](String line) { telnet.println(line); });
}

static void cmd_sensor_list(String _) {
  telnet.println(Preference.available_sensors_to_String());
}

static void cmd_sensor_add(String sensor_str) {
  app_traceln("cmd_sensor_add: enable sensor '" + sensor_str + "'");

  sensor_str.trim();
  sensor_str.toUpperCase();
  if (sensor_str.isEmpty()) {
    telnet.println(SENSORS_CMD_ERROR_STR("sensors-add"));
    return;
  }

  SensorType type;
  if (!SensorType_by_name(sensor_str, &type)) {
    telnet.println("unknown sensor with name '" + sensor_str + "'");
    return;
  }

  if (!Preference.add_sensor(type)) {
    telnet.println("error adding sensor '" + sensor_str + "'");
  }
}

static void cmd_sensor_rm(String sensor_str) {
  app_traceln("cmd_sensor_rm: disable sensor '" + sensor_str + "'");

  sensor_str.trim();
  sensor_str.toUpperCase();
  if (sensor_str.isEmpty()) {
    telnet.println(SENSORS_CMD_ERROR_STR("sensors-rm"));
    return;
  }

  SensorType type;
  if (!SensorType_by_name(sensor_str, &type)) {
    telnet.println("unknown sensor with name '" + sensor_str + "'");
    return;
  }

  if (!Preference.remove_sensor(type)) {
    telnet.println("error removing sensor '" + sensor_str + "'");
  }
}

static void cmd_board_host(String new_name) {
  new_name.trim();
  if (new_name.isEmpty()) {
    app_traceln("cmd_board_host: want to get the board host name");
    telnet.println(Preference.get_board_host_name());
  } else {
    app_traceln("cmd_board_host: wanto to set the board host name to: '" + new_name + "'");
    Preference.set_board_host_name(new_name);
  }
}

static void cmd_board_location(String new_loc) {
  new_loc.trim();
  if (new_loc.isEmpty()) {
    app_traceln("cmd_board_location: wanto to get the board location");
    telnet.println(Preference.get_board_location());
  } else {
    app_traceln("cmd_board_location: wanto to set the board location to '" + new_loc + "'");
    Preference.set_board_location(new_loc);
  }
}

static void cmd_board_room(String new_room) {
  new_room.trim();
  if (new_room.isEmpty()) {
    app_traceln("cmd_board_room: wanto to get the board room");
    telnet.println(Preference.get_board_room());
  } else {
    app_traceln("cmd_board_room: wanto to set the board room to '" + new_room + "'");
    Preference.set_board_room(new_room);
  }
}

static void cmd_mac_address(String mac_addr) {
  mac_addr.trim();
  if (mac_addr.isEmpty()) {
    app_traceln("cmd_mac_address: wants to get current MAC address");
    telnet.println(wifi_get_mac_address());
    return;
  }

  if (mac_addr == "off") {
    app_traceln("cmd_mac_address: wants to disable MAC address spoofing");
    if (!Preference.set_spoofed_mac("")) {
      app_errorln("cmd_mac_address: error disabling MAC address spoofing");
      telnet.println("cannot disable MAC spoofing");
    }
    return;
  }

  app_traceln("cmd_mac_address: want to set MAC address to: '" + mac_addr + "'");
  if (!Preference.set_spoofed_mac(mac_addr)) {
    telnet.println("cannot set MAC address");
  }
}

static void cmd_temp_offset(String temp_str) {
  temp_str.trim();
  if (temp_str.isEmpty()) {
    app_traceln("cmd_temp_offset: wants to get current temperature offset");
    telnet.println(String(Preference.get_temperature_offset()));
  } else {
    app_traceln("cmd_temp_offset: wants to set temperature offset");
    int temp_offset = (int)temp_str.toInt();
    if (!Preference.set_temperature_offset(temp_offset)) {
      telnet.println("error setting temperature offset");
    }
  }
}

static void cmd_uptime(String _) {
  telnet.println(board_uptime());
}

static void cmd_ping(String _) {
  telnet.println("pong");
}

static void cmd_reboot(String _) {
  telnet.println("rebooting board... you will be disconnected");
  telnet.disconnectClient();
  reboot_board(BOARD_REBOOT_DELAY_NOW);
}

static void cmd_version(String cmd) {
  telnet.println("IoT UniUrb version " VERSION);
}

static void cmd_quit(String cmd) {
  telnet.println("disconnecting you... bye!");
  telnet.disconnectClient();
}

static void cmd_help(String cmd) {
  telnet.println("IoT UniUrb");
  telnet.println("");
  telnet.println("commands:");

  for (int i = 0; i < size_of_array(commands); i++) {
    telnet.print("  " + commands[i].name);
    String spaces;
    for (int s = 0; s < (20 - commands[i].name.length()); s++) {
      spaces += ' ';
    }
    telnet.print(spaces);
    telnet.println(commands[i].help);
  }
}

static void telnet_on_connect_cb(String ip) {
  app_debugln("Telnet: IP '" + ip + "' connected");
  telnet.println("\nWelcome to IoT UniUrb " + telnet.getIP());
  telnet.println("Use help for all commands, quit to disconnect.");
  telnet.print("> ");
}

static void telnet_on_connection_attempt_cb(String ip) {
  app_debugln("Telnet: IP '" + ip + "' is trying to connect");
}

static void telnet_on_reconnect_cb(String ip) {
  app_debugln("Telnet: IP '" + ip + "' reconnected");
}

static void telnet_on_disconnect_cb(String ip) {
  app_debugln("Telnet: IP '" + ip + "' disconnected");
}

static void telnet_on_input_received_cb(String input) {
  String cmd;

  app_debugln("Telnet: new input '" + input + "'");
  // Parse command
  input.trim();
  if (input.isEmpty()) {
    goto telnet_parse_input_defer;
  }

  cmd = string_divide_by(&input, ' ');
  for (int i = 0; i < size_of_array(commands); i++) {
    if (cmd == commands[i].name) {
      commands[i].run(input);
      goto telnet_parse_input_defer;
    }
  }
  telnet.println("command not found: " + cmd);

telnet_parse_input_defer:
  telnet.print("> ");
}

bool telnet_init(int port) {
  app_traceln("telnet_init: init telnet server on port " + String(port));

  telnet.onConnect(telnet_on_connect_cb);
  telnet.onConnectionAttempt(telnet_on_connection_attempt_cb);
  telnet.onReconnect(telnet_on_reconnect_cb);
  telnet.onDisconnect(telnet_on_disconnect_cb);
  telnet.onInputReceived(telnet_on_input_received_cb);

  if (!telnet.begin(port)) {
    app_errorln("telnet_init: init error");
    return false;
  }

  app_infoln("telnet_init: running");
  return true;
}

void telnet_task_code(void *args) {
  TickType_t last_loop_time = xTaskGetTickCount();
  for (;;) {
    xTaskDelayUntil(&last_loop_time, pdMS_TO_TICKS(TELNET_LOOP_DELAY_MS));
    telnet.loop();
  }
}
