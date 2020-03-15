#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h> // needed by NTPClient.h
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include "config.h"
#include <EEPROM.h>
#include "settings.h" // Include my type definitions (must be in a separate file!)

// Constants
const char FIRMWARE_VERSION[] = "1.1";
const char COMPILE_DATE[] = __DATE__ " " __TIME__;
const int CURRENT_CONFIG_VERSION = 4;

const int HWPIN_PUSHBUTTON = D3;
const int HWPIN_LED_BOARD = LED_BUILTIN;
const int HWPIN_LED_WIFI = D8;
const int HWPIN_LED_MQTT = D7;

const int LED_MQTT_MIN_TIME = 500;
const int LED_WEB_MIN_TIME = 500;
const int TIME_BUTTON_LONGPRESS = 10000;
const int STATUS_PUBLISH_INTERVAL = 5000;
const int MQTT_RECONNECT_INTERVAL = 2000;
const int BEAMER_UPDATE_INTERVAL = 1000;

const int HWSERIAL_BAUD = 115200;
const int SWSERIAL_DEFAULT_BAUDRATE = 19200;

enum class Status
{
  //STARTING,
  ON,
  //SHUTDOWN,
  OFF,
  UNKNOWN
};
enum class BeamerModel
{
  BENQ,
  CANON,
  UNKNOWN
};

void HTMLHeader(const char section[], unsigned int refresh = 0, const char url[] = "/");

// buffers
String html;
char buff[255];

// Webserver
ESP8266WebServer server(80);

// Wifi Client
WiFiClient espClient;

// MQTT Client
PubSubClient client(espClient);

// Software
SoftwareSerial swSer(D6, D5);

// OTA Updater
ESP8266HTTPUpdateServer httpUpdater;

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Config
uint16_t cfgStart = 0;        // Start address in EEPROM for structure 'cfg'
configData_t cfg;             // Instance 'cfg' is a global variable with 'configData_t' structure now
bool configIsDefault = false; // true if no valid config found in eeprom and defaults settings loaded

// Runtime default config values
BeamerModel beamerModel = BeamerModel::UNKNOWN;
int ledBrightness = 1024;

// Misc
bool ledOneToggle = false;
bool ledTwoToggle = false;
Status lastBeamerStatus = Status::UNKNOWN;

unsigned long lastBeamerStatusTime = 0; // will store last beamer status time
unsigned long lastPublishTime = 0;      // will store last publish time
unsigned long ledOneTime = 0;           // will store last time LED was updated
unsigned long ledTwoTime = 0;           // will store last time LED was updated
unsigned long mqttLastReconnect = 0;    // will store last time reconnect to mqtt broker
bool previousButtonState = 1;           // will store last Button state. 1 = unpressed, 0 = pressed
unsigned long buttonTimer = 0;          // will store how long button was pressed

void clearSerialBuffer()
{
  while (swSer.available())
  {
    swSer.read();
  }
}

void HTMLHeader(const char *section, unsigned int refresh, const char *url)
{

  char title[50];
  char hostname[50];
  WiFi.hostname().toCharArray(hostname, 50);
  snprintf(title, 50, "Starting BeamerControl@%s - %s", hostname, section);

  html = "<!DOCTYPE html>";
  html += "<html>\n";
  html += "<head>\n";
  html += "<meta name='viewport' content='width=600' />\n";
  if (refresh != 0)
  {
    html += "<META http-equiv='refresh' content='";
    html += refresh;
    html += ";URL=";
    html += url;
    html += "'>\n";
  }
  html += "<title>";
  html += title;
  html += "</title>\n";
  html += "<style>\n";
  html += "body {\n";
  html += " background-color: #EDEDED;\n";
  html += " font-family: Arial, Helvetica, Sans-Serif;\n";
  html += " Color: #333;\n";
  html += "}\n";
  html += "\n";
  html += "h1 {\n";
  html += "  background-color: #333;\n";
  html += "  display: table-cell;\n";
  html += "  margin: 20px;\n";
  html += "  padding: 20px;\n";
  html += "  color: white;\n";
  html += "  border-radius: 10px 10px 0 0;\n";
  html += "  font-size: 20px;\n";
  html += "}\n";
  html += "\n";
  html += "ul {\n";
  html += "  list-style-type: none;\n";
  html += "  margin: 0;\n";
  html += "  padding: 0;\n";
  html += "  overflow: hidden;\n";
  html += "  background-color: #333;\n";
  html += "  border-radius: 0 10px 10px 10px;";
  html += "}\n";
  html += "\n";
  html += "li {\n";
  html += "  float: left;\n";
  html += "}\n";
  html += "\n";
  html += "li a {\n";
  html += "  display: block;\n";
  html += "  color: #FFF;\n";
  html += "  text-align: center;\n";
  html += "  padding: 16px;\n";
  html += "  text-decoration: none;\n";
  html += "}\n";
  html += "\n";
  html += "li a:hover {\n";
  html += "  background-color: #111;\n";
  html += "}\n";
  html += "\n";
  html += "#main {\n";
  html += "  padding: 20px;\n";
  html += "  background-color: #FFF;\n";
  html += "  border-radius: 10px;\n";
  html += "  margin: 10px 0;\n";
  html += "}\n";
  html += "\n";
  html += "#footer {\n";
  html += "  border-radius: 10px;\n";
  html += "  background-color: #333;\n";
  html += "  padding: 10px;\n";
  html += "  color: #FFF;\n";
  html += "  font-size: 12px;\n";
  html += "  text-align: center;\n";
  html += "}\n";

  html += "table  {\n";
  html += "border-spacing: 0;\n";
  html += "}\n";

  html += "table td, table th {\n";
  html += "padding: 5px;\n";
  html += "}\n";

  html += "table tr:nth-child(even) {\n";
  html += "background: #EDEDED;\n";
  html += "}";

  html += "input[type=\"submit\"] {\n";
  html += "background-color: #333;\n";
  html += "border: none;\n";
  html += "color: white;\n";
  html += "padding: 5px 25px;\n";
  html += "text-align: center;\n";
  html += "text-decoration: none;\n";
  html += "display: inline-block;\n";
  html += "font-size: 16px;\n";
  html += "margin: 4px 2px;\n";
  html += "cursor: pointer;\n";
  html += "}\n";

  html += "input[type=\"submit\"]:hover {\n";
  html += "background-color:#4e4e4e;\n";
  html += "}\n";

  html += "input[type=\"submit\"]:disabled {\n";
  html += "opacity: 0.6;\n";
  html += "cursor: not-allowed;\n";
  html += "}\n";

  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<h1>";
  html += title;
  html += "</h1>\n";
  html += "<ul>\n";
  html += "<li><a href='/'>Home</a></li>\n";
  html += "<li><a href='/switch'>Switch</a></li>\n";
  html += "<li><a href='/settings'>Settings</a></li>\n";
  html += "<li><a href='/wifiscan'>WiFi Scan</a></li>\n";
  html += "<li><a href='/fwupdate'>FW Update</a></li>\n";
  html += "<li><a href='/reboot'>Reboot</a></li>\n";
  html += "</ul>\n";
  html += "<div id='main'>";
}

void HTMLFooter()
{
  html += "</div>";
  html += "<div id='footer'>&copy; 2020 Fabian Otto - Firmware v";
  html += FIRMWARE_VERSION;
  html += " - Compiled at ";
  html += COMPILE_DATE;
  html += "</div>\n";
  html += "</body>\n";
  html += "</html>\n";
}

long dBm2Quality(long dBm)
{
  if (dBm <= -100)
    return 0;
  else if (dBm >= -50)
    return 100;
  else
    return 2 * (dBm + 100);
}

void updateStatus()
{
  size_t status_lenght;
  unsigned int i = 0;

  Serial.print("updateStatus: ");

  clearSerialBuffer();

  // Send Power Status qestion to Beamer
  if (beamerModel == BeamerModel::BENQ)
  {

    status_lenght = 50;
    char buffer[status_lenght];

    boolean readOn;
    readOn = false;

    swSer.print("\r*pow=?#\r");

    delay(100);
    while (swSer.available())
    {
      char c = char(swSer.read());

      if (c == 10)
      { // From, but without NL
        readOn = true;
      }
      else if (c != 13)
      { // Than all, but without CR
        if (readOn && i < status_lenght)
        {
          buffer[i] = c;
          i += 1;
        }
      }
    }
    buffer[i] = 0;

    Serial.println(buffer);

    if (strcmp(buffer, "*POW=OFF#") == 0)
    {
      lastBeamerStatus = Status::OFF;
    }
    else if (strcmp(buffer, "*POW=ON#") == 0)
    {
      lastBeamerStatus = Status::ON;
    }
    else
    {
      lastBeamerStatus = Status::UNKNOWN;
    }
  }
  else if (beamerModel == BeamerModel::CANON)
  {

    // Request    00H BFH 00H 00H 01H 02H C2H = 7
    // Response   20H BFH 01H xxH 10H DATA01 to DATA16 CKS = 22
    status_lenght = 22;
    byte buffer[status_lenght];
    byte checksum = 0;

    byte GetData[] = {0x00, 0xbf, 0x00, 0x00, 0x01, 0x02, 0xc2};
    swSer.write(GetData, 7);

    delay(100);
    while (swSer.available())
    {

      byte b = swSer.read();
      //int i = int(swSer.read());
      Serial.printf("%02x ", b);

      if (i < status_lenght)
      {
        if (i < status_lenght - 1)
        {
          checksum += b;
        }
        buffer[i] = b;
        i += 1;
      }
    }
    buffer[i] = 0;

    Serial.printf(" (Checksum: %02x, Last byte: %02x, Result: ", checksum, buffer[21]);

    if (buffer[0] != 0x20)
    {
      // Response, but not success
      Serial.println("No success response!)");
    }
    else if (buffer[21] != checksum)
    {
      // Chekcsum wrong
      Serial.println("Checksum wrong!)");
      lastBeamerStatus = Status::UNKNOWN;
    }
    else
    {
      // Checksum verified
      Serial.println("Checksum verified!)");

      switch (buffer[6])
      {
      case 0x00: // Idle
        lastBeamerStatus = Status::OFF;
        break;
      case 0x03: // Undocumented: "Starting"
        lastBeamerStatus = Status::ON;
        break;
      case 0x04: // Power On
        lastBeamerStatus = Status::ON;
        break;
      case 0x05: // Cooling
        lastBeamerStatus = Status::ON;
        break;
      case 0x06: // Idle(Error Standby)
        lastBeamerStatus = Status::OFF;
        break;
      default:
        lastBeamerStatus = Status::UNKNOWN;
        break;
      }
    }
  }
  else
  {
    lastBeamerStatus = Status::UNKNOWN;
  }
}

Status getStatus()
{
  return lastBeamerStatus;
}

String getBeamerInfo()
{
  String str;
  switch (beamerModel)
  {
  case BeamerModel::CANON:
    str = "Canon";
    break;
  case BeamerModel::BENQ:
    str = "Benq";
    break;
  default:
    str = "Unkown";
    break;
  }
  return str + " (" + (configIsDefault ? SWSERIAL_DEFAULT_BAUDRATE : cfg.beamerbaudrate) + " Baud)";
}

String getStatusString()
{
  switch (getStatus())
  {
    /*case Status::STARTING:
    return "Starting";
    break;*/
  case Status::ON:
    return "On";
    break;
    /*case Status::SHUTDOWN:
    return "Shutdown";
    break;*/
  case Status::OFF:
    return "Off";
    break;
  default:
    return "Unkown";
    break;
  }
}

void showMQTTAction()
{
  analogWrite(HWPIN_LED_MQTT, 0);
  ledTwoTime = millis();
}

void showWEBAction()
{
  analogWrite(HWPIN_LED_WIFI, 0);
  ledOneTime = millis();
}

void publishStatus()
{
  showMQTTAction();
  Serial.println("-----------------------");
  Serial.println("Send mqtt Status message: ");

  switch (getStatus())
  {
  /*case Status::STARTING:
    Serial.println("Publish Status STARTING");
    client.publish(cfg.mqtt_topicPublish, "#starting");
    break;*/
  case Status::ON:
    Serial.println("Publish Status ON");
    client.publish(cfg.mqtt_topicPublish, "#on");
    break;
  /*case Status::SHUTDOWN:
    Serial.println("Publish Status SHUTDOWN");
    client.publish(cfg.mqtt_topicPublish, "#shutdown");
    break;*/
  case Status::OFF:
    Serial.println("Publish Status OFF");
    client.publish(cfg.mqtt_topicPublish, "#off");
    break;
  case Status::UNKNOWN:
    Serial.println("Publish Status UNKNOWN");
    client.publish(cfg.mqtt_topicPublish, "#unkown");
    break;
  }
}

void setStatus(Status status)
{

  // Switch Beamer ON
  if (status == Status::ON)
  {
    Serial.println("Sending power ON sequence...");
    if (beamerModel == BeamerModel::BENQ)
    {
      swSer.print("\r*pow=on#\r");
    }
    else
    {
      byte GetData[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
      swSer.write(GetData, 6);
      delay(500);
      clearSerialBuffer();
    }

    // Switch Beamer OFF
  }
  else
  {
    Serial.println("Sending power OFF sequence...");
    if (beamerModel == BeamerModel::BENQ)
    {
      swSer.print("\r*pow=off#\r");
    }
    else
    {
      byte GetData[] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x03};
      swSer.write(GetData, 6);
      delay(500);
      clearSerialBuffer();
    }
  }
}

void toggleStatus()
{

  switch (getStatus())
  {
  case Status::ON:
    setStatus(Status::OFF);
    break;
  case Status::OFF:
    setStatus(Status::ON);
    break;
  default:
    //do nothing
    break;
  }
}

void BeamerControl(char *message)
{

  if (strcmp(message, "#on") == 0)
  {
    setStatus(Status::ON);
  }
  else if (strcmp(message, "#off") == 0)
  {
    setStatus(Status::OFF);
  }
  else if (strcmp(message, "#status") == 0)
  {
    //getStatus();
    publishStatus();
  }
  else
  {
    Serial.print("Unkown command");
  }
}

void saveConfig()
{
  EEPROM.begin(512);
  EEPROM.put(cfgStart, cfg);
  delay(200);
  EEPROM.commit(); // Only needed for ESP8266 to get data written
  EEPROM.end();
}

void eraseConfig()
{
  Serial.print("Erase EEPROM config...");
  EEPROM.begin(512);
  for (uint16_t i = cfgStart; i < sizeof(cfg); i++)
  {
    EEPROM.write(i, 0);
    //Serial.printf("Block %i of %i\n", i, sizeof(cfg));
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();
  Serial.print("done\n");
}

void handleSwitch()
{
  showWEBAction();

  if (!server.authenticate(cfg.admin_username, cfg.admin_password))
  {
    return server.requestAuthentication();
  }
  else
  {

    if (server.method() == HTTP_POST)
    {
      for (uint8_t i = 0; i < server.args(); i++)
      {
        if (server.argName(i) == "action")
        {
          if (server.arg(i) == "On")
          {
            setStatus(Status::ON);
          }
          else if (server.arg(i) == "Off")
          {
            setStatus(Status::OFF);
          }
        }
      }
    }

    HTMLHeader("Switch Socket");
    html += "<form method='POST' action='/switch'>";
    html += "<input type='submit' name='action' value='On'>";
    html += "<input type='submit' name='action' value='Off'>";
    html += "</form>";

    HTMLFooter();
    server.send(200, "text/html", html);
  }
}

void handleFWUpdate()
{
  showWEBAction();
  if (!server.authenticate(cfg.admin_username, cfg.admin_password))
  {
    return server.requestAuthentication();
  }
  else
  {
    HTMLHeader("Firmware Update");

    html += "<form method='POST' action='/dofwupdate' enctype='multipart/form-data'>\n";
    html += "<table>\n";
    html += "<tr>\n";
    html += "<td>Current version</td>\n";
    html += String("<td>") + FIRMWARE_VERSION + String("</td>\n");
    html += "</tr>\n";
    html += "<tr>\n";
    html += "<td>Compiled</td>\n";
    html += String("<td>") + COMPILE_DATE + String("</td>\n");
    html += "</tr>\n";
    html += "<tr>\n";
    html += "<td>Firmware file</td>\n";
    html += "<td><input type='file' name='update'></td>\n";
    html += "</tr>\n";
    html += "</table>\n";
    html += "<br />";
    html += "<input type='submit' value='Update'>";
    html += "</form>";
    HTMLFooter();
    server.send(200, "text/html", html);
  }
}

void handleNotFound()
{
  showWEBAction();
  HTMLHeader("File Not Found");
  html += "URI: ";
  html += server.uri();
  html += "<br />\nMethod: ";
  html += (server.method() == HTTP_GET) ? "GET" : "POST";
  html += "<br />\nArguments: ";
  html += server.args();
  html += "<br />\n";
  HTMLFooter();
  for (uint8_t i = 0; i < server.args(); i++)
  {
    html += " " + server.argName(i) + ": " + server.arg(i) + "<br />\n";
  }

  server.send(404, "text/html", html);
}

void handleWiFiScan()
{
  showWEBAction();
  if (!server.authenticate(cfg.admin_username, cfg.admin_password))
  {
    return server.requestAuthentication();
  }
  else
  {

    HTMLHeader("WiFi Scan");

    int n = WiFi.scanNetworks();
    if (n == 0)
    {
      html += "No networks found.\n";
    }
    else
    {
      html += "<table>\n";
      html += "<tr>\n";
      html += "<th>#</th>\n";
      html += "<th>SSID</th>\n";
      html += "<th>Channel</th>\n";
      html += "<th>Signal</th>\n";
      html += "<th>RSSI</th>\n";
      html += "<th>Encryption</th>\n";
      html += "<th>BSSID</th>\n";
      html += "</tr>\n";
      for (int i = 0; i < n; ++i)
      {
        html += "<tr>\n";
        snprintf(buff, sizeof(buff), "%02d", (i + 1));
        html += String("<td>") + buff + String("</td>");
        html += "<td>\n";
        if (WiFi.isHidden(i))
        {
          html += "[hidden SSID]";
        }
        else
        {
          html += "<a href='/settings?ssid=";
          html += WiFi.SSID(i).c_str();
          html += "'>";
          html += WiFi.SSID(i).c_str();
          html += "</a>";
        }
        html += "</td>\n<td>";
        html += WiFi.channel(i);
        html += "</td>\n<td>";
        html += dBm2Quality(WiFi.RSSI(i));
        html += "%</td>\n<td>";
        html += WiFi.RSSI(i);
        html += "dBm</td>\n<td>";
        switch (WiFi.encryptionType(i))
        {
        case ENC_TYPE_WEP: // 5
          html += "WEP";
          break;
        case ENC_TYPE_TKIP: // 2
          html += "WPA TKIP";
          break;
        case ENC_TYPE_CCMP: // 4
          html += "WPA2 CCMP";
          break;
        case ENC_TYPE_NONE: // 7
          html += "OPEN";
          break;
        case ENC_TYPE_AUTO: // 8
          html += "WPA";
          break;
        }
        html += "</td>\n<td>";
        html += WiFi.BSSIDstr(i).c_str();
        html += "</td>\n";
        html += "</tr>\n";
      }
      html += "</table>";
    }

    HTMLFooter();

    server.send(200, "text/html", html);
  }
}

void handleReboot()
{
  showWEBAction();
  if (!server.authenticate(cfg.admin_username, cfg.admin_password))
  {
    return server.requestAuthentication();
  }
  else
  {
    boolean reboot = false;
    if (server.method() == HTTP_POST)
    {
      HTMLHeader("Reboot", 10, "/");
      html += "Reboot in progress...";
      reboot = true;
    }
    else
    {
      HTMLHeader("Reboot");
      html += "<form method='POST' action='/reboot'>";
      html += "<input type='submit' value='Reboot'>";
      html += "</form>";
    }
    HTMLFooter();

    server.send(200, "text/html", html);

    if (reboot)
    {
      delay(200);
      ESP.reset();
    }
  }
}

void handleRoot()
{
  showWEBAction();

  HTMLHeader("Main");

  html += "<table>\n";

  char timebuf[20];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  int days = hr / 24;
  snprintf(timebuf, 20, " %02d:%02d:%02d:%02d", days, hr % 24, min % 60, sec % 60);

  html += "<tr>\n<td>Uptime</td>\n<td>";
  html += timebuf;
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Current Time</td>\n<td>";
  html += timeClient.getFormattedDate();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Firmware</td>\n<td>v";
  html += FIRMWARE_VERSION;
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Compiled</td>\n<td>";
  html += COMPILE_DATE;
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>MQTT Status</td>\n<td>";
  if (client.connected())
  {
    html += "Connected";
  }
  else
  {
    html += "Not Connected";
  }
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Beamer model</td>\n<td>";
  html += getBeamerInfo();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Power status</td>\n<td>";
  html += getStatusString();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Note</td>\n<td>";
  if (strcmp(cfg.note, "") == 0)
  {
    html += "---";
  }
  else
  {
    html += cfg.note;
  }
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Hostname</td>\n<td>";
  html += WiFi.hostname().c_str();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>IP</td>\n<td>";
  html += WiFi.localIP().toString();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Gateway</td>\n<td>";
  html += WiFi.gatewayIP().toString();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Subnetmask</td>\n<td>";
  html += WiFi.subnetMask().toString();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>MAC</td>\n<td>";
  html += WiFi.macAddress().c_str();
  html += "</td>\n</tr>\n";

  html += "<tr>\n<td>Signal strength</td>\n<td>";
  html += dBm2Quality(WiFi.RSSI());
  html += "% (";
  html += WiFi.RSSI();
  html += "dBm)</td>\n</tr>\n";

  html += "<tr>\n<td>Client IP</td>\n<td>";
  html += server.client().remoteIP().toString().c_str();
  html += "</td>\n</tr>\n";

  html += "</table>\n";

  HTMLFooter();
  server.send(200, "text/html", html);
}

void handleSettings()
{
  showWEBAction();
  Serial.println("Site: handleSettings");
  // HTTP Auth
  if (!server.authenticate(cfg.admin_username, cfg.admin_password))
  {
    return server.requestAuthentication();
  }
  else
  {
    Serial.println("Auth okay!");
    boolean saveandreboot = false;
    String value;
    if (server.method() == HTTP_POST)
    { // Save Settings

      for (uint8_t i = 0; i < server.args(); i++)
      {
        // Trim String
        value = server.arg(i);
        value.trim();

        // RF Note
        if (server.argName(i) == "note")
        {
          value.toCharArray(cfg.note, sizeof(cfg.note) / sizeof(*cfg.note));

        } // HTTP Auth Adminaccess Username
        else if (server.argName(i) == "admin_username")
        {
          value.toCharArray(cfg.admin_username, sizeof(cfg.admin_username) / sizeof(*cfg.admin_username));

        } // HTTP Auth Adminaccess Password
        else if (server.argName(i) == "admin_password")
        {
          value.toCharArray(cfg.admin_password, sizeof(cfg.admin_password) / sizeof(*cfg.admin_password));

        } // WiFi SSID
        else if (server.argName(i) == "ssid")
        {
          value.toCharArray(cfg.wifi_ssid, sizeof(cfg.wifi_ssid) / sizeof(*cfg.wifi_ssid));

        } // WiFi PSK
        else if (server.argName(i) == "psk")
        {
          value.toCharArray(cfg.wifi_psk, sizeof(cfg.wifi_psk) / sizeof(*cfg.wifi_psk));

        } // Hostname
        else if (server.argName(i) == "hostname")
        {
          value.toCharArray(cfg.hostname, sizeof(cfg.hostname) / sizeof(*cfg.hostname));

        } // Beamer Model
        else if (server.argName(i) == "beamermodel")
        {
          value.toCharArray(cfg.beamermodel, sizeof(cfg.beamermodel) / sizeof(*cfg.beamermodel));
        } // Beamer Baud Rate
        else if (server.argName(i) == "beamerbaudrate")
        {
          cfg.beamerbaudrate = value.toInt();

        } // MQTT Server
        else if (server.argName(i) == "mqtt_server")
        {
          value.toCharArray(cfg.mqtt_server, sizeof(cfg.mqtt_server) / sizeof(*cfg.mqtt_server));

        } // MQTT Port
        else if (server.argName(i) == "mqtt_port")
        {
          cfg.mqtt_port = value.toInt();

        } // MQTT User
        else if (server.argName(i) == "mqtt_user")
        {
          value.toCharArray(cfg.mqtt_user, sizeof(cfg.mqtt_user) / sizeof(*cfg.mqtt_user));

        } // MQTT Password
        else if (server.argName(i) == "mqtt_password")
        {
          value.toCharArray(cfg.mqtt_password, sizeof(cfg.mqtt_password) / sizeof(*cfg.mqtt_password));

        } // MQTT Topic Subscribe
        else if (server.argName(i) == "mqtt_topicSubscribe")
        {
          value.toCharArray(cfg.mqtt_topicSubscribe, sizeof(cfg.mqtt_topicSubscribe) / sizeof(*cfg.mqtt_topicSubscribe));

        } // MQTT Topic Publish
        else if (server.argName(i) == "mqtt_topicPublish")
        {
          value.toCharArray(cfg.mqtt_topicPublish, sizeof(cfg.mqtt_topicPublish) / sizeof(*cfg.mqtt_topicPublish));

        } // LED Brightness
        else if (server.argName(i) == "led_brightness")
        {
          cfg.led_brightness = value.toInt();
        }

        saveandreboot = true;
      }
    }

    if (saveandreboot)
    {
      HTMLHeader("Settings", 10, "/settings");
      html += ">>> New Settings saved! Device will be reboot <<< ";
    }
    else
    {
      HTMLHeader("Settings");

      html += "Current Settings Source is ";
      html += (configIsDefault ? "NOT " : "");
      html += "from EEPROM.<br />";
      html += "<br />\n";

      html += "<form action='/settings' method='post'>\n";
      html += "<table>\n";
      html += "<tr>\n";
      html += "<td>Hostname:</td>\n";
      html += "<td><input name='hostname' type='text' maxlength='30' autocapitalize='none' placeholder='";
      html += WiFi.hostname().c_str();
      html += "' value='";
      html += cfg.hostname;
      html += "'></td></tr>\n";

      html += "<tr>\n<td>\nSSID:</td>\n";
      html += "<td><input name='ssid' type='text' autocapitalize='none' maxlength='30' value='";
      bool showssidfromcfg = true;
      if (server.method() == HTTP_GET)
      {
        if (server.arg("ssid") != "")
        {
          html += server.arg("ssid");
          showssidfromcfg = false;
        }
      }
      if (showssidfromcfg)
      {
        html += cfg.wifi_ssid;
      }
      html += "'> <a href='/wifiscan' onclick='return confirm(\"Go to scan site? Changes will be lost!\")'>Scan</a></td>\n</tr>\n";

      html += "<tr>\n<td>\nPSK:</td>\n";
      html += "<td><input name='psk' type='password' maxlength='30' value='";
      html += cfg.wifi_psk;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nNote:</td>\n";
      html += "<td><input name='note' type='text' maxlength='30' value='";
      html += cfg.note;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nAdminaccess Username:</td>\n";
      html += "<td><input name='admin_username' type='text' maxlength='30' autocapitalize='none' value='";
      html += cfg.admin_username;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nAdminaccess Password:</td>\n";
      html += "<td><input name='admin_password' type='password' maxlength='30' value='";
      html += cfg.admin_password;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>LED Brightness:</td>\n";
      html += "<td><select name='led_brightness'>";
      html += "<option value='25'";
      html += (cfg.led_brightness == 25 ? " selected" : "");
      html += ">25%</option>";
      html += "<option value='50'";
      html += (cfg.led_brightness == 50 ? " selected" : "");
      html += ">50%</option>";
      html += "<option value='75'";
      html += (cfg.led_brightness == 75 ? " selected" : "");
      html += ">75%</option>";
      html += "<option value='100'";
      html += (cfg.led_brightness == 100 ? " selected" : "");
      html += ">100%</option>";
      html += "</select>";
      html += "</td>\n</tr>\n";

      html += "<tr>\n<td>Beamer Model:</td>\n";
      html += "<td><select name='beamermodel'>";
      html += "<option value='benq'";
      html += (strcmp("benq", cfg.beamermodel) == 0 ? " selected" : "");
      html += ">BENQ</option>";
      html += "<option value='canon'";
      html += (strcmp("canon", cfg.beamermodel) == 0 ? " selected" : "");
      html += ">Canon</option>";
      html += "</select>";
      html += "</td>\n</tr>\n";

      html += "<tr>\n<td>Beamer baud rate:</td>\n";
      html += "<td><select name='beamerbaudrate'>";
      html += "<option value='19200'";
      html += (cfg.beamerbaudrate == 19200 ? " selected" : "");
      html += ">19200</option>";
      html += "<option value='115200'";
      html += (cfg.beamerbaudrate == 115200 ? " selected" : "");
      html += ">115200</option>";
      html += "</select>";
      html += "</td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT Server:</td>\n";
      html += "<td><input name='mqtt_server' type='text' maxlength='30' autocapitalize='none' value='";
      html += cfg.mqtt_server;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT Port:</td>\n";
      html += "<td><input name='mqtt_port' type='text' maxlength='5' autocapitalize='none' value='";
      html += cfg.mqtt_port;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT User:</td>\n";
      html += "<td><input name='mqtt_user' type='text' maxlength='30' autocapitalize='none' value='";
      html += cfg.mqtt_user;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT Password:</td>\n";
      html += "<td><input name='mqtt_password' type='password' maxlength='30' autocapitalize='none' value='";
      html += cfg.mqtt_password;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT Topic Subscribe:</td>\n";
      html += "<td><input name='mqtt_topicSubscribe' type='text' maxlength='30' autocapitalize='none' value='";
      html += cfg.mqtt_topicSubscribe;
      html += "'></td>\n</tr>\n";

      html += "<tr>\n<td>\nMQTT Topic Publish:</td>\n";
      html += "<td><input name='mqtt_topicPublish' type='text' maxlength='30' autocapitalize='none' value='";
      html += cfg.mqtt_topicPublish;
      html += "'></td>\n</tr>\n";

      html += "</table>\n";

      html += "<br />\n";
      html += "<input type='submit' value='Save'>\n";
      html += "</form>\n";
    }
    HTMLFooter();
    server.send(200, "text/html", html);

    if (saveandreboot)
    {
      saveConfig();
      ESP.reset();
    }
  }
}

void MQTTcallback(char *topic, byte *payload, unsigned int length)
{
  showMQTTAction();
  Serial.println("-----------------------");
  Serial.println("New Message");
  Serial.print("> Lenght: ");
  Serial.println(length);
  Serial.print("> Topic: ");
  Serial.println(topic);

  Serial.print("> Message: ");

  char *message = new char[length + 1];

  //message = (char *)payload;
  /*
  for (unsigned int i = 0; i < length; i++)
  {
    //message = message + (char)payload[i]; //Conver *byte to String
    message[i] = (char)payload[i];
  }
  */
  memcpy(message, payload, length);
  message[length] = 0;
  Serial.print(message);
  Serial.println();

  BeamerControl(message);

  delete message;
}

boolean MQTTconnect()
{
  Serial.print("Connecting to MQTT...");
  if (strcmp(cfg.mqtt_server, "") == 0)
  {
    Serial.println("failed. No server configured.");
    return false;
  }
  else
  {
    client.setServer(cfg.mqtt_server, cfg.mqtt_port);
    client.setCallback(MQTTcallback);

    if (client.connect(cfg.hostname, cfg.mqtt_user, cfg.mqtt_password))
    {
      Serial.println("connected!");
      client.subscribe(cfg.mqtt_topicSubscribe);
      Serial.printf("Subscribed to topic %s\n", cfg.mqtt_topicSubscribe);
      return true;
    }
    else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      return false;
    }
  }
}

void loadDefaults()
{

  // Config NOT from EEPROM
  configIsDefault = true;

  // Valid-Falg to verify config
  cfg.configversion = CURRENT_CONFIG_VERSION;

  // Note
  memcpy(cfg.note, "", sizeof(cfg.note) / sizeof(*cfg.note));

  memcpy(cfg.wifi_ssid, "", sizeof(cfg.wifi_ssid) / sizeof(*cfg.wifi_ssid));
  memcpy(cfg.wifi_psk, "", sizeof(cfg.wifi_psk) / sizeof(*cfg.wifi_psk));

  memcpy(cfg.hostname, "", sizeof(cfg.hostname) / sizeof(*cfg.hostname));
  memcpy(cfg.note, "", sizeof(cfg.note) / sizeof(*cfg.note));

  memcpy(cfg.beamermodel, "", sizeof(cfg.beamermodel) / sizeof(*cfg.beamermodel));

  memcpy(cfg.admin_username, "", sizeof(cfg.admin_username) / sizeof(*cfg.admin_username));
  memcpy(cfg.admin_password, "", sizeof(cfg.admin_password) / sizeof(*cfg.admin_password));

  memcpy(cfg.mqtt_server, "", sizeof(cfg.mqtt_server) / sizeof(*cfg.mqtt_server));
  memcpy(cfg.mqtt_user, "", sizeof(cfg.mqtt_user) / sizeof(*cfg.mqtt_user));
  cfg.mqtt_port = 1883;
  memcpy(cfg.mqtt_password, "", sizeof(cfg.mqtt_password) / sizeof(*cfg.mqtt_password));
  memcpy(cfg.mqtt_topicSubscribe, "", sizeof(cfg.mqtt_topicSubscribe) / sizeof(*cfg.mqtt_topicSubscribe));
  memcpy(cfg.mqtt_topicPublish, "", sizeof(cfg.mqtt_topicPublish) / sizeof(*cfg.mqtt_topicPublish));
}

void loadConfig()
{
  EEPROM.begin(512);
  EEPROM.get(cfgStart, cfg);
  EEPROM.end();

  if (cfg.configversion != CURRENT_CONFIG_VERSION)
  {
    loadDefaults();
  }
  else
  {
    configIsDefault = false; // Config from EEPROM
  }
}

void handleButton()
{
  bool inp = digitalRead(HWPIN_PUSHBUTTON);
  //Serial.printf("Button state: %d\n", inp);
  if (inp == 0) // Button pressed
  {
    if (inp != previousButtonState)
    {
      Serial.printf("Button short press @ %lu\n", millis());
      toggleStatus();
      buttonTimer = millis();
    }
    if ((millis() - buttonTimer >= TIME_BUTTON_LONGPRESS))
    {
      Serial.printf("Button long press @ %lu\n", millis());
      eraseConfig();
      ESP.reset();
    }

    // Delay a little bit to avoid bouncing
    delay(50);
  }
  previousButtonState = inp;
}

void setup(void)
{
  // LED Basic Setup
  pinMode(HWPIN_LED_BOARD, OUTPUT);
  pinMode(HWPIN_LED_WIFI, OUTPUT);
  pinMode(HWPIN_LED_MQTT, OUTPUT);
  digitalWrite(HWPIN_LED_BOARD, 1); // OFF
  digitalWrite(HWPIN_LED_WIFI, 0);  // OFF
  digitalWrite(HWPIN_LED_MQTT, 0);  // OFF

  // GPIO Basic Setup
  pinMode(HWPIN_PUSHBUTTON, INPUT_PULLUP);

  // Load Config
  loadConfig();

  Serial.begin(HWSERIAL_BAUD);
  delay(1000);
  Serial.printf("\nWelcome to BeamerControl v%s\n", FIRMWARE_VERSION);
  WiFi.mode(WIFI_OFF);

  // AP or Infrastructire mode
  if (configIsDefault)
  {
    // Start AP
    Serial.println("Default Config loaded.");
    Serial.println("Starting WiFi SoftAP");
    WiFi.softAP("BeamerControl", "");
    analogWrite(HWPIN_LED_WIFI, ledBrightness);
  }
  else
  {

    // LED brightness
    ledBrightness = (1024 / 100) * cfg.led_brightness;

    WiFi.mode(WIFI_STA);
    if (strcmp(cfg.hostname, "") != 0)
    {
      WiFi.hostname(cfg.hostname);
    }
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_psk);

    Serial.printf("Connecing to '%s'. Please wait", cfg.wifi_ssid);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(250);
      Serial.print(".");
      if (ledOneToggle)
      {
        analogWrite(HWPIN_LED_WIFI, ledBrightness);
      }
      else
      {
        analogWrite(HWPIN_LED_WIFI, 0);
      }
      ledOneToggle = !ledOneToggle;

      handleButton();
    }

    Serial.printf("\nConnected to '%s'\n", cfg.wifi_ssid);
    WiFi.printDiag(Serial);
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

    analogWrite(HWPIN_LED_WIFI, ledBrightness);

    // Beamermodel
    if (strcmp(cfg.beamermodel, "benq") == 0)
    {
      beamerModel = BeamerModel::BENQ;
    }
    else if (strcmp(cfg.beamermodel, "canon") == 0)
    {
      beamerModel = BeamerModel::CANON;
    }

    // Beamer baud rate
    if (!configIsDefault)
    {
      swSer.begin(cfg.beamerbaudrate);
    }
    else
    {
      swSer.begin(19200);
    }

    // MDNS responder
    if (MDNS.begin(cfg.hostname))
    {
      Serial.println("MDNS responder started");
    }

    // NTPClient
    timeClient.begin();
  }

  // Arduino OTA Update
  httpUpdater.setup(&server, "/dofwupdate", cfg.admin_username, cfg.admin_password);

  // Webserver
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.on("/fwupdate", handleFWUpdate);
  server.on("/switch", handleSwitch);
  server.on("/reboot", handleReboot);
  server.on("/wifiscan", handleWiFiScan);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started");
}

void loop(void)
{

  // Switch back on WiFi LED after Webserver access
  if ((millis() - ledOneTime) > LED_WEB_MIN_TIME)
  {
    analogWrite(HWPIN_LED_WIFI, ledBrightness);
  }

  // Handle Button
  handleButton();

  // Handle Webserver
  server.handleClient();

  // NTPClient Update
  timeClient.update();

  // Update Beamer Status
  if ((millis() - lastBeamerStatusTime) > BEAMER_UPDATE_INTERVAL)
  {
    lastBeamerStatusTime = millis();
    updateStatus();
  }

  // Config valid and WiFi connection
  if (!configIsDefault && WiFi.status() == WL_CONNECTED)
  {

    if (client.connected())
    {

      // Switch back on MQTT LED if we have server connection
      if ((millis() - ledTwoTime) > LED_MQTT_MIN_TIME)
      {
        analogWrite(HWPIN_LED_MQTT, ledBrightness);
      }

      // Handle MQTT msgs
      client.loop();

      if (millis() - lastPublishTime >= STATUS_PUBLISH_INTERVAL)
      {
        lastPublishTime = millis();
        publishStatus();
      }
    }
    else
    {
      // MQTT connect
      if ((millis() - mqttLastReconnect) >= MQTT_RECONNECT_INTERVAL)
      {
        MQTTconnect();
        mqttLastReconnect = millis();
      }
    }
  }
}
