
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

#define STASSID "***"
#define STAPSK  "***"

#define SWITCH_OPEN_PIN 13
#define SWITCH_CLOSE_PIN 12
#define MOTOR_1_PIN 2
#define MOTOR_2_PIN 15


#define INIT_VALUE 56
#define DEFAULT_CRON_OPEN "0 0 6 * * *"
#define DEFAULT_CRON_CLOSE "0 0 20 * * *"


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

bool door_is_open(){
  return ! digitalRead(SWITCH_OPEN_PIN);
}

bool door_is_close(){
  return ! digitalRead(SWITCH_CLOSE_PIN);
}

String processor(const String& var){
  if(var == "TIME"){
    time_t now = time(nullptr);
    return ctime(&now);
  } else if(var == "DOOR_STATE"){
    if (door_is_open())
      return "OPEN";
    if (door_is_close())
      return "CLOSE";
    return "UNKNOWN";    
  } else if(var == "CRON_OPEN"){
    return readStringFromEEPROM(ADDR_CRON_OPEN);
  } else if(var == "CRON_CLOSE"){
    return readStringFromEEPROM(ADDR_CRON_CLOSE);
  }

  return "Unknown";
}

void updateCron(CronId id, int addrOffset, OnTick_t onTickHandler){
  Serial.println("RUN - updateCron");
  Cron.free(id);
  String cronExpression = readStringFromEEPROM(addrOffset);
  Serial.println(cronExpression);
  id = Cron.create(cronExpression.c_str(), onTickHandler, false);
  Serial.println("RUN - updateCron - OK");
}

void updateVars(const AsyncWebServerRequest *request){
  Serial.println("RUN - updateVars");

  if (request->hasParam("cron_open")){
    writeStringToEEPROM(ADDR_CRON_OPEN, request->getParam("cron_open")->value());
    EEPROM.commit();
    updateCron(cron_open, ADDR_CRON_OPEN, []() {
      open_door_request = true;
    });
  }

  if (request->hasParam("cron_close")){
    writeStringToEEPROM(ADDR_CRON_CLOSE, request->getParam("cron_close")->value());
    EEPROM.commit();
    updateCron(cron_close, ADDR_CRON_CLOSE, []() {
      close_door_request = true;
    });
  }
  
  Serial.println("RUN - updateVars - OK");
}


Thread motorThread = Thread();
Thread cronThread = Thread();


void setup(void) {

  Serial.begin(115200);
  Serial.println("INIT - Start");

  // Init EEPREOM
  EEPROM.begin(EEPROM_SIZE);

  // Init EEPROM vars :
  if( EEPROM.read(ADDR_INIT) != INIT_VALUE){
    Serial.println("INIT - Reset EEPROM value");
    EEPROM.write(ADDR_INIT, INIT_VALUE);
    writeStringToEEPROM(ADDR_CRON_OPEN, DEFAULT_CRON_OPEN);
    writeStringToEEPROM(ADDR_CRON_CLOSE, DEFAULT_CRON_CLOSE);
    EEPROM.commit();
  }
  


  // Init stepper motor
  Serial.println("INIT - Init Motor");
  pinMode(SWITCH_OPEN_PIN, INPUT);
  pinMode(SWITCH_CLOSE_PIN, INPUT);
  pinMode(MOTOR_1_PIN, OUTPUT);
  pinMode(MOTOR_2_PIN, OUTPUT);
  
  digitalWrite(MOTOR_1_PIN, LOW);
  digitalWrite(MOTOR_2_PIN, LOW);
  Serial.println("INIT - Init Motor - OK");

  // Init Wifi
  Serial.println("INIT - Init Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print("INIT - Wifi Connected : ");
  Serial.println(WiFi.localIP());
  
  // Init mDNS
  if (MDNS.begin("districroq")) {
    Serial.println("INIT - mDNS OK");
  }

  // Init FS
  Serial.println("INIT - LittleFS");
  LittleFS.begin();
  

  Serial.println("INIT - Route init");
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    updateVars(request);
    if (request->hasParam("open")) {
      open_door_request = true;
    }
    if (request->hasParam("close")) {
      close_door_request = true;
    }
    request->send(LittleFS, "/index.html", String(), false, processor);
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
  Serial.println("INIT - Route init - OK");

  // Start server
  server.begin();
  Serial.println("INIT - HTTP Server started");

  // Start NTP
  Serial.println("INIT - NTP start");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  ip4_addr ipaddr;
  IP4_ADDR(&ipaddr, 176,31,251,158);
	sntp_setserver(0, &ipaddr);
	sntp_init();
  Serial.println("INIT - NTP start - OK");

  cronThread.setInterval(100);
  motorThread.onRun( []() {
    
    if(open_door_request){
      if(door_is_open()){
        open_door_request = false;
      } else {
        digitalWrite(MOTOR_1_PIN, LOW);
        digitalWrite(MOTOR_2_PIN, HIGH);
      }
    } else
    if(close_door_request){
      if(door_is_close()){
        close_door_request = false;
      } else {
        digitalWrite(MOTOR_1_PIN, HIGH);
        digitalWrite(MOTOR_2_PIN, LOW);
      }
    } else {
      digitalWrite(MOTOR_1_PIN, LOW);
      digitalWrite(MOTOR_2_PIN, LOW);
    }
      
  });

  cronThread.setInterval(100);
  cronThread.onRun( []() {
    Cron.delay();
  });
  
  
  controller.add(&motorThread); 
  controller.add(&cronThread); 

  updateCron(cron_open, ADDR_CRON_OPEN, []() {
    open_door_request = true;
  });
  updateCron(cron_close, ADDR_CRON_CLOSE, []() {
    close_door_request = true;
  });
  
}

void loop(void) {
  controller.run();
}
