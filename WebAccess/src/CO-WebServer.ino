
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <EEPROM.h>
#include <Thread.h>
#include <ThreadController.h>
#include <time.h>
#include <sys/time.h>
#include <CronAlarms.h>
#include <sntp.h>
#include "protocol.hpp"

#define DEBUG 0

#define STASSID "GuiEtJew"
#define STAPSK  "lithium est notre chat."

#define INIT_VALUE 10
#define DEFAULT_CRON_OPEN "0 0 5 * * *"
#define DEFAULT_CRON_CLOSE "0 0 22 * * *"


#define ADDR_INIT 0
#define ADDR_CRON_OPEN ADDR_INIT + sizeof(int)
#define ADDR_CRON_CLOSE ADDR_CRON_OPEN + 128
#define EEPROM_SIZE ADDR_CRON_CLOSE + 128

const char *ssid = STASSID;
const char *password = STAPSK;

bool open_door_request = false;
bool close_door_request = false;

WiFiUDP ntpUDP;
AsyncWebServer server(80);
ThreadController controller = ThreadController();
CronId cron_open = dtINVALID_ALARM_ID;
CronId cron_close = dtINVALID_ALARM_ID;

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = 0;
  return String(data);
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}
String processor(const String& var){
  if(var == "TIME"){
    time_t now = time(nullptr);
    return ctime(&now);
  } else if(var == "DOOR_STATE"){

    Serial.println(QUERY_STATE);
    String result = "";
    while(result=""){
      if (Serial.available()){
        result = Serial.readString();
        delay(10);
      }
    }
    //String result=RESPONSE_IS_OPEN;
    if(result.equals(RESPONSE_IS_OPEN)){
      return "OPEN";
    }
    if(result.equals(RESPONSE_IS_CLOSE)){
      return "CLOSE";
    }
    return "UNKNOWN";    
  } else if(var == "CRON_OPEN"){
    return readStringFromEEPROM(ADDR_CRON_OPEN);
  } else if(var == "CRON_CLOSE"){
    return readStringFromEEPROM(ADDR_CRON_CLOSE);
  }

  return "Unknown";
}

void updateCron(CronId id, int addrOffset, OnTick_t onTickHandler){
  if (DEBUG)
    Serial.println("RUN - updateCron");
  Cron.free(id);
  String cronExpression = readStringFromEEPROM(addrOffset);
  Serial.println(cronExpression);
  id = Cron.create(cronExpression.c_str(), onTickHandler, false);
  if (DEBUG)
    Serial.println("RUN - updateCron - OK");
}

Thread cronThread = Thread();


void setup(void) {

  Serial.begin(115200);
  Serial.println(QUERY_STATUS_INIT_START);

  // Init EEPREOM
  EEPROM.begin(EEPROM_SIZE);

  // Init EEPROM vars :
  if( EEPROM.read(ADDR_INIT) != INIT_VALUE){
    EEPROM.write(ADDR_INIT, INIT_VALUE);
    writeStringToEEPROM(ADDR_CRON_OPEN, DEFAULT_CRON_OPEN);
    writeStringToEEPROM(ADDR_CRON_CLOSE, DEFAULT_CRON_CLOSE);
    EEPROM.commit();
  }
  

  // Init Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println(QUERY_STATUS_INIT_CONNECTED);
  Serial.println(WiFi.localIP());
  
  // Init mDNS
  MDNS.begin("districroq");

  // Init FS
  LittleFS.begin();
  

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (DEBUG) Serial.println("RUN - call /");
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  server.on("/api/open", HTTP_GET, [](AsyncWebServerRequest *request){
    if (DEBUG) Serial.println("RUN - call /api/open");
    Serial.println(ACTION_OPEN);
    request->redirect("/");
  });
  server.on("/api/close", HTTP_GET, [](AsyncWebServerRequest *request){
    if (DEBUG) Serial.println("RUN - call /api/close");
    Serial.println(ACTION_OPEN);
    request->redirect("/");
  });
  server.on("/api/cron/open", HTTP_GET, [](AsyncWebServerRequest *request){
    if (DEBUG) Serial.println("RUN - call /api/cron/open");
    if (request->hasParam("cron_open")) {
      writeStringToEEPROM(ADDR_CRON_OPEN, request->getParam("cron_open")->value());
      EEPROM.commit();
      updateCron(cron_open, ADDR_CRON_OPEN, []() {
        open_door_request = true;
      });
    }
    request->redirect("/");
  });
  server.on("/api/cron/close", HTTP_GET, [](AsyncWebServerRequest *request){
    if (DEBUG) Serial.println("RUN - call /api/cron/close");
    if (request->hasParam("cron_close")) {
      writeStringToEEPROM(ADDR_CRON_CLOSE, request->getParam("cron_close")->value());
      EEPROM.commit();
      updateCron(cron_close, ADDR_CRON_CLOSE, []() {
        close_door_request = true;
      });
    }
    request->redirect("/");
  });
  
  // Route to load style.css file
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/main.css", "text/css");
  });

  server.onNotFound( [](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  // Start server
  server.begin();

  // Start NTP
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  ip4_addr ipaddr;
  IP4_ADDR(&ipaddr, 176,31,251,158);
	sntp_setserver(0, &ipaddr);
	sntp_init();

  cronThread.setInterval(1000);
  cronThread.onRun( []() {
    Cron.delay();
  });
  
  
  controller.add(&cronThread); 

  updateCron(cron_open, ADDR_CRON_OPEN, []() {
    open_door_request = true;
  });
  updateCron(cron_close, ADDR_CRON_CLOSE, []() {
    close_door_request = true;
  });

  Serial.println(QUERY_STATUS_INIT_DONE);
  
}

void loop(void) {
  controller.run();
}
