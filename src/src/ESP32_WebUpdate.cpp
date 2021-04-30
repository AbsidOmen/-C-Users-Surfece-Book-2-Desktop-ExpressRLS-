#ifdef PLATFORM_ESP32

#include "targets.h"

#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
#include "SX127xDriver.h"
extern SX127xDriver Radio;
#endif

#if defined(Regulatory_Domain_ISM_2400)
#include "SX1280Driver.h"
extern SX1280Driver Radio;
#endif

#include "ESP32_hwTimer.h"
extern hwTimer hwTimer;

#include "CRSF.h"
extern CRSF crsf;

#include "config.h"
extern TxConfig config;

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <Update.h>
#include <set>

#include "ESP32_WebContent.h"
#include "flag_png.h"
#include "main_css.h"

uint8_t target_seen = 0;
uint8_t target_pos = 0;

const char *ssid = "ExpressLRS TX Module"; // The name of the Wi-Fi network that will be created
const char *password = "expresslrs";       // The password required to connect to it, leave blank for an open network
const char *myHostname = "elrs_tx";
wifi_mode_t wifiMode = WIFI_MODE_NULL;
wifi_mode_t changeMode = WIFI_MODE_NULL;

const byte DNS_PORT = 53;
IPAddress apIP(10, 0, 0, 1);
IPAddress netMsk(255, 255, 255, 0);
DNSServer dnsServer;
WebServer server(80);

/** Is this an IP? */
boolean isIp(String str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String toStringIp(IPAddress ip)
{
    String res = "";
    for (int i = 0; i < 3; i++)
    {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

bool captivePortal()
{
    if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local"))
    {
        Serial.println("Request redirected to captive portal");
        server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
        server.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        server.client().stop();             // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

void WebUpdateSendCSS()
{
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/css", CSS, sizeof(CSS));
}

void WebUpdateSendJS()
{
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/css", SCAN_JS, sizeof(SCAN_JS));
}

void WebUpdateSendPNG()
{
  server.send_P(200, "image/png", PNG, sizeof(PNG));
}

void WebUpdateHandleRoot()
{
  if (captivePortal())
  { // If captive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/html", INDEX_HTML, sizeof(INDEX_HTML));
}

void WebUpdateSendMode()
{
  String s;
  if (wifiMode == WIFI_STA) {
    s = String("{\"mode\":\"STA\",\"ssid\":\"") + config.GetSSID() + "\"}";
  } else {
    s = String("{\"mode\":\"AP\",\"ssid\":\"") + config.GetSSID() + "\"}";
  }
  server.send(200, "application/json", s);
}

void WebUpdateSendNetworks()
{
  int numNetworks = WiFi.scanComplete();
  if (numNetworks >= 0) {
    Serial.printf("Found %d networks\n", numNetworks);
    std::set<String> vs;
    String s="[";
    for(int i=0 ; i<numNetworks ; i++) {
      String w = WiFi.SSID(i);
      Serial.printf("found %s\n", w.c_str());
      if (vs.find(w)==vs.end() && w.length()>0) {
        if (!vs.empty()) s += ",";
        s += "\"" + w + "\"";
        vs.insert(w);
      }
    }
    s+="]";
    server.send(200, "application/json", s);
  } else {
    server.send(204, "application/json", "[]");
  }
}

void WebUpdateAccessPoint(void)
{
  Serial.println("Starting Access Point");
  String msg = String("Access Point starting, please connect to access point '") + ssid + "' with password '" + password + "'";
  server.send(200, "text/plain", msg);
  changeMode = WIFI_AP;
}

void WebUpdateConnect(void)
{
  Serial.println("Connecting to home network");
  String msg = String("Connected to network '") + ssid + "', connect to http://elrs_tx.local from a browser on that network";
  server.send(200, "text/plain", msg);
  changeMode = WIFI_STA;
}

void WebUpdateSetHome(void)
{
  String ssid = server.arg("network");
  String password = server.arg("password");

  Serial.printf("Setting home network %s\n", ssid.c_str());
  config.SetSSID(ssid.c_str());
  config.SetPassword(password.c_str());
  config.Commit();
  WebUpdateConnect();
}

void WebUpdateForget(void)
{
  Serial.println("Forget home network");
  config.SetSSID("");
  config.SetPassword("");
  config.Commit();
  String msg = String("Home network forgotten, please connect to access point '") + ssid + "' with password '" + password + "'";
  server.send(200, "text/plain", msg);
  changeMode = WIFI_AP;
}

void WebUpdateHandleNotFound()
{
    if (captivePortal())
    { // If captive portal redirect instead of displaying the error page.
        return;
    }
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += server.args();
    message += F("\n");

    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
    }
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(404, "text/plain", message);
}

void startWifi() {
  WiFi.persistent(false);
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.setHostname(myHostname);
  delay(500);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    switch(event) {
      case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("Access Point enabled");
        wifiMode = WIFI_AP;
        WiFi.mode(wifiMode);
        WiFi.softAPConfig(apIP, apIP, netMsk);
        WiFi.softAP(ssid, password);
        WiFi.scanNetworks(true);
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("Connected as Wifi station");
        break;
      default:
        break;
    }
  });
  if (config.GetSSID()[0]==0) {
    Serial.println("Access Point enabled");
    wifiMode = WIFI_AP;
    WiFi.mode(wifiMode);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid, password);
    WiFi.scanNetworks(true);
  } else {
    Serial.printf("Connecting to home network '%s'\n", config.GetSSID());
    wifiMode = WIFI_STA;
    WiFi.mode(wifiMode);
    WiFi.begin(config.GetSSID(), config.GetPassword());
  }
}

void BeginWebUpdate()
{
    hwTimer.stop();
    Radio.End();
    crsf.End();

    Serial.println("Begin Webupdater");
    Serial.println("Stopping Radio");

    startWifi();

    server.on("/", WebUpdateHandleRoot);
    server.on("/main.css", WebUpdateSendCSS);
    server.on("/scan.js", WebUpdateSendJS);
    server.on("/flag.png", WebUpdateSendPNG);
    server.on("/mode.json", WebUpdateSendMode);
    server.on("/networks.json", WebUpdateSendNetworks);
    server.on("/sethome", WebUpdateSetHome);
    server.on("/forget", WebUpdateForget);
    server.on("/connect", WebUpdateConnect);
    server.on("/access", WebUpdateAccessPoint);

    server.on("/generate_204", WebUpdateHandleRoot); // handle Andriod phones doing shit to detect if there is 'real' internet and possibly dropping conn.
    server.on("/gen_204", WebUpdateHandleRoot);
    server.on("/library/test/success.html", WebUpdateHandleRoot);
    server.on("/hotspot-detect.html", WebUpdateHandleRoot);
    server.on("/connectivity-check.html", WebUpdateHandleRoot);
    server.on("/check_network_status.txt", WebUpdateHandleRoot);
    server.on("/ncsi.txt", WebUpdateHandleRoot);
    server.on("/fwlink", WebUpdateHandleRoot);
    server.onNotFound(WebUpdateHandleNotFound);

    server.on(
        "/update", HTTP_POST, []() {
      server.client().setNoDelay(true);
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", target_seen ? ((Update.hasError()) ? "FAIL" : "OK") : "WRONG FIRMWARE");
      delay(100);
      server.client().stop();
      ESP.restart(); },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin()) { //start with max available size
          Update.printError(Serial);
        }
        target_seen = 0;
        target_pos = 0;
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
        if (!target_seen) {
          for (int i=0 ; i<upload.currentSize ;i++) {
            if (upload.buf[i] == target_name[target_pos]) {
              ++target_pos;
              if (target_pos >= target_name_size) {
                target_seen = 1;
              }
            }
            else {
              target_pos = 0; // Startover
            }
          }
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (target_seen) {
          if (Update.end(true)) { //true to set the size to the current progress
            Serial.printf("Upload Success: %ubytes\nPlease wait for LED to resume blinking before disconnecting power\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        } else {
          Update.abort();
          Serial.printf("Wrong firmware uploaded, not %s, update aborted\n", &target_name[4]);
        }
        Serial.setDebugOutput(false);
      } else if(upload.status == UPLOAD_FILE_ABORTED){
        Update.abort();
        Serial.println("Update was aborted");
      } });

    dnsServer.start(DNS_PORT, "*", apIP);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

    if (!MDNS.begin(myHostname))
    {
      Serial.println("Error starting mDNS");
      return;
    }
    MDNS.addService("http", "tcp", 80);

    server.begin();
}

void HandleWebUpdate()
{
    if (changeMode != wifiMode && changeMode != WIFI_MODE_NULL) {
      switch(changeMode) {
        case WIFI_AP:
          WiFi.disconnect();
          break;
        case WIFI_STA:
          WiFi.mode(WIFI_STA);
          wifiMode = WIFI_STA;
          WiFi.begin(config.GetSSID(), config.GetPassword());
        default:
          break;
      }
      changeMode = WIFI_MODE_NULL;
    }
    dnsServer.processNextRequest();
    server.handleClient();
    yield();
}

#endif
