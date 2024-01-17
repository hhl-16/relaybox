/*
  ESP32 for relaybox
  ver 1.1
  api webserver
  recei uart from stm32

*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <AsyncElegantOTA.h>
#include "time.h"
#include <base64.h>

#define BAUD 115200
#define FLAG1 26
#define FLAG2 27
#define LED 18

AsyncWebServer server(80);

typedef struct {
  String ssid = "";
  String password = "";
} inforap;
inforap infor_ap;

typedef struct {
  String ssid = "";
  String password = "";
  String ip = "";
  String subnet = "";
  String gateway = "";
} inforsta;
inforsta infor_sta;
typedef struct {
  String ip = "";
  String subnet = "";
  String gateway = "";
} inforethernet;
inforethernet infor_ethernet;
struct tm timeinfo;
typedef struct {
  String state_relay1 = "OFF";
  String state_relay2 = "OFF";
  String state_relay3 = "OFF";
  String state_relay4 = "OFF";
  String state_relay5 = "OFF";

  String state_input1 = "OFF";
  String state_input2 = "OFF";
  String state_input3 = "OFF";
} displaystateweb;
displaystateweb display_state_web;

typedef struct {
  String path = "";
  String user = "";
  String password = "";
  bool state_update = false;
  uint8_t pos = 0;
} updateapipost;
updateapipost update_api_post;
String cam_api1 = "";
String cam_api2 = "";
String cam_api3 = "";
/*
   VARIABLE GLOBAL
*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 7;
const int   daylightOffset_sec = 3600;
bool state_time = false;
String update_ethernet = "";

bool state_request_relay = false;
String value_request_relay = "";
unsigned long previousMillis = 0;


IPAddress localIP;
IPAddress localGateway;
IPAddress subnet;
String ip_local = "";


String buffer_uart = "";

/*
  FUNCTION
*/

void read_uart_stm(); //read uart2
void route_webserver();
void write_spiffs(String, String); // write data to spiffs
void append_spiffs(String, String); // append data to spiffs
String read_spiffs(String); // read data from spiffs
void setup_spiffs(); // config init spiffs
void init_read_file(); //read config init in file txt
String processor(const String &var);
void mode_wifi_ap();
void delete_all_file();
bool init_wifi_sta();
void wifi_relay_log(String data);
String get_time();
void stm_relay_log(String data);
void config_init_stm();


//process function API
void updateDCode(String data); //update name device
void updateDescription(String data);  // update infor Description  device
void updateEthernetInfo(String data); //update ethe lan stm32
void updateWiFiSTAMode(String data); //update wifi mode STA
void updateWiFiAPMode(String data);  //update wifi mode AP
void updateDevicePassword(String data);
void updateapipostcamera(String data, uint8_t pos); //update api post camera
void updateapipostcameratostm(String data, uint8_t pos);
void updateapipostcamera_query(String data, uint8_t pos);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD);
  Serial2.begin(BAUD);
  Serial.println("==========RELAY VER 1.2========");
  pinMode(FLAG1, INPUT);
  pinMode(FLAG2, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  setup_spiffs();
  init_read_file();
  config_init_stm();
  if (init_wifi_sta()) {
    Serial.println("mode STA");
  }
  else {
    mode_wifi_ap();
  }
  route_webserver();

}

void loop() {
  // put your main code here, to run repeatedly:
  read_uart_stm() ;
  if (state_request_relay) {
    Serial.println("data request relay:" + value_request_relay);
    state_request_relay = false;
    wifi_relay_log(value_request_relay);
  }
  if (update_api_post.state_update) {
    update_api_post.state_update = false;
    if (update_api_post.pos == 1) {
      Serial.println("------------");
      Serial.println(update_api_post.path);
      Serial.println(update_api_post.user);
      Serial.println(update_api_post.password);
      Serial.println("------------");
      write_spiffs("/camApi1.txt", update_api_post.path + "|" + update_api_post.user + "|" + update_api_post.password);
      delay(1000);
    }
    else if (update_api_post.pos == 2) {
      write_spiffs("/camApi2.txt", update_api_post.path + "|" + update_api_post.user + "|" + update_api_post.password);
      delay(1000);
    }
    else if (update_api_post.pos == 3) {
      write_spiffs("/camApi3.txt", update_api_post.path + "|" + update_api_post.user + "|" + update_api_post.password);
      delay(1000);
    }
    ESP.restart();
  }

}
/*----------------------------------------------------------------
  read uart2

*/
void read_uart_stm() {
  if (Serial2.available()) {
    buffer_uart = Serial2.readStringUntil('\n');
    Serial.println("buffer_uart:" + buffer_uart);
    if (buffer_uart.indexOf("LOG") >= 0) {
      stm_relay_log(buffer_uart);
    }
    else if (buffer_uart.indexOf("LAN1") >= 0) {
      write_spiffs("/LANs.txt", "TRUE");
    }
    else if (buffer_uart.indexOf("LAN0") >= 0) {
      write_spiffs("/LANs.txt", "FALSE");
    }
    else if (buffer_uart.indexOf("RESTART_ESP") >= 0) {
      ESP.restart();
    }


    else if (buffer_uart.indexOf("DELETE ALL FILE") >= 0) {
      delete_all_file();
    }

  }
}
/*
  init spiffs
*/
void setup_spiffs() {
  if (!SPIFFS.begin(true)) {
    Serial.println("init fail spiffs");
  }
  else {
    Serial.println("init success spiffs");
  }
}

void write_spiffs(String namefile, String draw) {
  File file = SPIFFS.open(namefile, "w+");
  if (file)
  {
    file.print(draw);
  }
  else  {}
  file.close();
  delay(50);
}

String read_spiffs(String namefile) {
  String data = "";
  File file = SPIFFS.open(namefile, "r");
  delay(15);
  while (file.available())
  {
    data += (char)file.read();
  }
  file.close();
  delay(50);
  return data;
}

void append_spiffs(String namefile, String data) {
  File file = SPIFFS.open(namefile, "a");
  delay(15);
  if (file)
  {
    file.print(data);
  }
  else {}
  file.close();
  delay(50);
}
/*----------------------------------------------------------------
  READ DATA IN FILE TXT
*/

void init_read_file() {
  if (read_spiffs("/dPassword.txt") == "") {
    write_spiffs("/dPassword.txt", "Admin@1946@");
  }
  delay(5);
  update_ethernet = read_spiffs("/updatee.txt");
  delay(5);
  infor_ap.ssid = read_spiffs("/wAPssid.txt");
  delay(5);
  infor_ap.password = read_spiffs("/wAPpass.txt");
  delay(5);
  infor_sta.ssid = read_spiffs("/wSTAssid.txt");
  delay(5);
  infor_sta.password = read_spiffs("/wSTApass.txt");
  delay(5);
  infor_sta.ip = read_spiffs("/wSTAip.txt");
  delay(5);
  infor_sta.subnet = read_spiffs("/wSTAsub.txt");
  delay(5);
  infor_sta.gateway = read_spiffs("/wSTAgw.txt");
  delay(5);
  infor_ethernet.ip = read_spiffs("/eIP.txt");
  delay(5);
  infor_ethernet.subnet = read_spiffs("/eSUB.txt");
  delay(5);
  infor_ethernet.gateway = read_spiffs("/eGW.txt");
  delay(5);
  cam_api1 = read_spiffs("/camApi1.txt");
  delay(5);
  cam_api2 = read_spiffs("/camApi2.txt");
  delay(5);
  cam_api3 = read_spiffs("/camApi3.txt");

  /*
    Serial.println("infor_ap.ssid:" + infor_ap.ssid);
    Serial.println("infor_ap.password:" + infor_ap.password);
    Serial.println("infor_sta.ssid:" + infor_sta.ssid);
    Serial.println("infor_sta.password:" + infor_sta.password);
    Serial.println("infor_sta.ip:" + infor_sta.ip);
    Serial.println("infor_sta.subnet:" + infor_sta.subnet);
    Serial.println("infor_sta.gateway:" + infor_sta.gateway);
    Serial.println("infor_ethernet.ip:" + infor_ethernet.ip);
    Serial.println("infor_ethernet.subnet:" + infor_ethernet.subnet);
    Serial.println("infor_ethernet.gateway:"+ infor_ethernet.gateway);

  */
}

/*
  Route for root / web page
*/
void route_webserver() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

  server.on("/Input-event-history.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/Input-event-history.html", "text/html", false, processor);
  });
  server.on("/API-event-history.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/API-event-history.html", "text/html", false, processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/bootstrap.min.js", "text/javascrip");
  });
  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  });
  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/jquery-3.3.1.min.js", "text/javascrip");
  });
  server.on("/src/moment.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/moment.min.js", "text/javascrip");
  });
  server.on("/src/datetimepicker.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/datetimepicker.min.css", "text/css");
  });
  server.on("/src/datetimepicker.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/datetimepicker.min.js", "text/javascrip");
  });
  server.on("/src/xlsxmin.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/xlsxmin.min.js", "text/javascrip");
  });

  //---------------------------file txt--------------------------------------------------------
  server.on("/camApi1.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/camApi1.txt", "text/plain");
  });
  server.on("/camApi2.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/camApi2.txt", "text/plain");
  });
  server.on("/camApi3.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/camApi3.txt", "text/plain");
  });

  server.on("/dPassword.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/dPassword.txt", "text/plain");
  });

  server.on("/dCode.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/dCode.txt", "text/plain");
  });
  server.on("/dDescription.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/dDescription.txt", "text/plain");
  });
  server.on("/eGW.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/eGW.txt", "text/plain");
  });
  server.on("/eIP.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/eIP.txt", "text/plain");
  });
  server.on("/eSUB.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/eSUB.txt", "text/plain");
  });
  server.on("/LANs.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/LANs.txt", "text/plain");
  });
  server.on("/localIP.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/localIP.txt", "text/plain");
  });

  server.on("/wAPpass.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wAPpass.txt", "text/plain");
  });
  server.on("/wAPssid.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wAPssid.txt", "text/plain");
  });
  server.on("/wSTAgw.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wSTAgw.txt", "text/plain");
  });
  server.on("/wSTApass.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wSTApass.txt", "text/plain");
  });
  server.on("/wSTAip.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wSTAip.txt", "text/plain");
  });
  server.on("/wSTAssid.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wSTAssid.txt", "text/plain");
  });
  server.on("/wSTAsub.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wSTAsub.txt", "text/plain");
  });
  server.on("/eve.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/eve.txt", "text/plain");
  });
  server.on("/updatee.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/updatee.txt", "text/plain");
  });



  //----------------------API----------------------------------------------------------------

  //http://{myDomain}/updateWiFiAPMode?mySsid=${mySSID},myPassword=${myPassword};
  server.on("/updateWiFiAPMode", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("mySsid")) {
      input = request->getParam("mySsid")->value();
    }
    Serial.println("updateWiFiAPMode" + input);
    updateWiFiAPMode(input);
    request->send(200, "text/plain", "capnhatthanhcong");
    ESP.restart();
  });
  //http://${myDomain}/updateWiFiSTAMode?mySsid=${mySSID},myPassword=${myPassword},myIP=${myIP},mySubnet=${mySubnet},myGateway=${myGateway}`
  server.on("/updateWiFiSTAMode", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("mySsid")) {
      input = request->getParam("mySsid")->value();
    }
    Serial.println("updateWiFiSTAMode" + input);
    updateWiFiSTAMode(input);
    request->send(200, "text/plain", "capnhatthanhcong");
    ESP.restart();
  });
  //http://{myDomain}/updateEthernetInfo?myIP=${myIP},mySubnet=${mySubnet},myGateway=${myGateway}`;
  server.on("/updateEthernetInfo", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("myIP")) {
      input = request->getParam("myIP")->value();
    }
    Serial.println("updateEthernetInfo" + input);
    updateEthernetInfo(input);
    request->send(200, "text/plain", "capnhatthanhcong");
    ESP.restart();
  });
  //http://${myDomain}/updateDCode?data=${myDCode}`
  server.on("/updateDCode", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("data")) {
      input = request->getParam("data")->value();
    }
    Serial.println("updateDCode" + input);
    updateDCode(input);
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  //http://${myDomain}/updateDescription?data=${myDDescription}`
  server.on("/updateDescription", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("data")) {
      input = request->getParam("data")->value();
    }
    Serial.println("updateDescription" + input);
    updateDescription(input);
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  server.on("/updateDevicePassword", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String input;
    if (request->hasParam("data")) {
      input = request->getParam("data")->value();
    }
    Serial.println("updateDescription" + input);
    updateDevicePassword(input);
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  server.on("/delete_all_file", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    delete_all_file();
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  //http://IP_box/api/triggerRelay?relayID={1...5}&camID={value}&state={0 hoac 1}
  server.on("/api/triggerRelay", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    int index_get = request->params();
    String input[3];
    for (int i = 0; i < index_get; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      if (p->name() == "relayID")
        input[0] = p->value();
      else if (p->name() == "camID")
        input[1] = p->value();
      else if (p->name() == "state")
        input[2] = p->value();
    }
    state_request_relay = true;
    value_request_relay = input[0] + input[2] + input[1];
    request->send(200, "text/plain", "capnhatthanhcong");
  });

  //-------------------------------

  server.on("/updateApiInput1", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    int index_get = request->params();
    String input;
    for (int i = 0; i < index_get; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      input += p->name() +"="+p->value();
      if(p->value().indexOf("myUsername=") == -1 ){
        input += "&";
      }
    }
    Serial.print("URL1:");
    Serial.println(input);
    updateapipostcamera_query(input,1);
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  server.on("/updateApiInput2", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    int index_get = request->params();
    String input;
    for (int i = 0; i < index_get; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      input += p->name() +"="+p->value();
      if(p->value().indexOf("myUsername=") == -1 ){
        input += "&";
      }
    }
    Serial.print("URL2:");
    Serial.println(input);
    updateapipostcamera_query(input,2);
    request->send(200, "text/plain", "capnhatthanhcong");
  });
  server.on("/updateApiInput3", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    int index_get = request->params();
    String input;
    for (int i = 0; i < index_get; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      input += p->name() +"="+p->value();
      if(p->value().indexOf("myUsername=") == -1 ){
        input += "&";
      }
    }
    Serial.print("URL3:");
    Serial.println(input);
    updateapipostcamera_query(input,3);
    request->send(200, "text/plain", "capnhatthanhcong");
  });


















  //--------------------------------

  //http://${myDomain}/updateApiInput2?myPath=${myPath},myUsername=${myUsername},myPassword=${myPassword}`;
  //POST API to camera
//  server.on("/updateApiInput2", HTTP_GET, [](AsyncWebServerRequest * request)
//  {
//    String input;
//    if (request->hasParam("myPath")) {
//      input = request->getParam("myPath")->value();
//    }
//    Serial.println("post api camera2:" + input);
//    updateapipostcamera(input, 2);
//    request->send(200, "text/plain", "capnhatthanhcong");
//  });
//  server.on("/updateApiInput5", HTTP_GET, [](AsyncWebServerRequest * request)
//  {
//    String input;
//    if (request->hasParam("myPath")) {
//      input = request->getParam("myPath")->value();
//    }
//    Serial.println("post api camera1:" + input);
//    updateapipostcamera(input, 1);
//    request->send(200, "text/plain", "capnhatthanhcong");
//  });

//  server.on("/updateApiInput3", HTTP_GET, [](AsyncWebServerRequest * request)
//  {
//    String input;
//    if (request->hasParam("myPath")) {
//      input = request->getParam("myPath")->value();
//    }
//    Serial.println("post api camera3:" + input);
//    updateapipostcamera(input, 3);
//    request->send(200, "text/plain", "capnhatthanhcong");
//  });


  AsyncElegantOTA.begin(&server);
  server.begin();
}
String processor(const String &var) {
  if (var == "STATE")
  {
    return WiFi.softAPmacAddress();
  }
  else if (var == "StateRelay1") {
    return display_state_web.state_relay1;
  }
  else if (var == "StateRelay2") {
    return display_state_web.state_relay2;
  }
  else if (var == "StateRelay3") {
    return display_state_web.state_relay3;
  }
  else if (var == "StateRelay4") {
    return display_state_web.state_relay4;
  }
  else if (var == "StateRelay5") {
    return display_state_web.state_relay5;
  }

  else if (var == "StateInput1") {
    return display_state_web.state_input1;
  }
  else if (var == "StateInput2") {
    return display_state_web.state_input2;
  }
  else if (var == "StateInput3") {
    return display_state_web.state_input3;
  }
  return String();
}
void mode_wifi_ap() {
  WiFi.mode(WIFI_MODE_AP);
  if (infor_ap.ssid == "") {
    infor_ap.ssid  = "INS - " + WiFi.softAPmacAddress();
    // write_spiffs("/wAPssid.txt",infor_ap.ssid);
  }
  if (infor_ap.password == "") {
    infor_ap.password = "123456789";
    // write_spiffs("/wAPpass.txt",infor_ap.password);
  }
  Serial.println("----------------");
  Serial.println(infor_ap.ssid);
  Serial.println(infor_ap.password);
  Serial.println("----------------");

  WiFi.softAP(infor_ap.ssid.c_str(), infor_ap.password.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  write_spiffs("/localIP.txt", IP.toString());
  state_time = false;
}

void updateDCode(String data) {
  write_spiffs("/dCode.txt", data);
}
void updateDescription(String data) {
  write_spiffs("/dDescription.txt", data);
}
//updateEthernetInfo:11111,mySubnet=aaaaa,myGateway=bbbbb
void updateEthernetInfo(String data) {
  uint8_t index[3];
  index[0] = data.indexOf("mySubnet");
  index[1] = data.indexOf("myGateway");
  index[2] = data.length();
  infor_ethernet.ip = data.substring(0, index[0] - 1);
  infor_ethernet.subnet = data.substring(index[0] + 9, index[1] - 1);
  infor_ethernet.gateway = data.substring(index[1] + 10, index[2]);
  Serial.print("infor_ethernet:");
  Serial.println(infor_ethernet.ip);
  Serial.println(infor_ethernet.subnet);
  Serial.println(infor_ethernet.gateway);
  write_spiffs("/eIP.txt", infor_ethernet.ip);
  write_spiffs("/eSUB.txt", infor_ethernet.subnet);
  write_spiffs("/eGW.txt", infor_ethernet.gateway);
  write_spiffs("/updatee.txt", "TRUE");



}
//updateWiFiSTAMode:zzzzz,myPassword=xxxxx,myIP=qqqqq,mySubnet=wwwww,myGateway=eeeee
void updateWiFiSTAMode(String data) {
  uint8_t index[5];
  index[0] = data.indexOf("myPassword");
  index[1] = data.indexOf("myIP");
  index[2] = data.indexOf("mySubnet");
  index[3] = data.indexOf("myGateway");
  index[4] = data.length();

  infor_sta.ssid = data.substring(0, index[0] - 1);
  infor_sta.password = data.substring(index[0] + 11, index[1] - 1);
  infor_sta.ip = data.substring(index[1] + 5, index[2] - 1);
  infor_sta.subnet = data.substring(index[2] + 9, index[3] - 1);
  infor_sta.gateway = data.substring(index[3] + 10, index[4]);

  // Serial.print("infor_ethernet:");
  // Serial.println(infor_sta.ssid);
  // Serial.println(infor_sta.password);
  // Serial.println(infor_sta.ip);
  // Serial.println(infor_sta.subnet);
  // Serial.println(infor_sta.gateway);
  write_spiffs("/wSTAssid.txt", infor_sta.ssid);
  write_spiffs("/wSTApass.txt", infor_sta.password);
  write_spiffs("/wSTAip.txt", infor_sta.ip);
  write_spiffs("/wSTAsub.txt", infor_sta.subnet);
  write_spiffs("/wSTAgw.txt", infor_sta.gateway);

}
//updateWiFiAPMode:aaaaaaaaa,myPassword=baaaaaaaa
void updateWiFiAPMode(String data) {
  uint8_t index[2];
  index[0] = data.indexOf("myPassword");
  index[1] = data.length();
  infor_ap.ssid = data.substring(0, index[0] - 1);
  infor_ap.password = data.substring(index[0] + 11, index[1]);
  // Serial.print("infor_ap:");
  // Serial.println(infor_ap.ssid);
  // Serial.println(infor_ap.password);
  write_spiffs("/wAPssid.txt", infor_ap.ssid);
  write_spiffs("/wAPpass.txt", infor_ap.password);
}
void delete_all_file() {
  Serial.println("xoa file");
  SPIFFS.remove("/wAPpass.txt"); delay(15);
  SPIFFS.remove("/wAPssid.txt"); delay(15);
  SPIFFS.remove("/wSTAgw.txt"); delay(15);
  SPIFFS.remove("/wSTAip.txt"); delay(15);
  SPIFFS.remove("/wSTApass.txt"); delay(15);
  SPIFFS.remove("/wSTAssid.txt"); delay(15);
  SPIFFS.remove("/wSTAsub.txt"); delay(15);
  SPIFFS.remove("/eve.txt"); delay(15);
  SPIFFS.remove("/camApi1.txt"); delay(15);
  SPIFFS.remove("/camApi2.txt"); delay(15);
  SPIFFS.remove("/camApi3.txt"); delay(15);
  ESP.restart();
}
bool init_wifi_sta() {
  if (infor_sta.ssid == "" || infor_sta.password == "") {
    return false;
  }
  WiFi.mode(WIFI_STA);
  localIP.fromString(infor_sta.ip.c_str());
  localGateway.fromString(infor_sta.gateway.c_str());
  subnet.fromString(infor_sta.subnet.c_str());
  if (!WiFi.config(localIP, localGateway, subnet)) {
    SPIFFS.remove("/wSTAssid.txt"); delay(5);
    SPIFFS.remove("/wSTApasswd.txt");
    ESP.restart();
  }
  WiFi.begin(infor_sta.ssid.c_str(), infor_sta.password.c_str());
  previousMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - previousMillis >= 10000) {
      Serial.println("Failed to connect.");
      SPIFFS.remove("/wSTAssid.txt"); delay(5);
      SPIFFS.remove("/wSTApasswd.txt"); delay(5);
      ESP.restart();
    }
    Serial.println(".");
    delay(1000);
  }
  IPAddress IP = WiFi.localIP();
  Serial.println(IP.toString());
  write_spiffs("/localIP.txt", IP.toString());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  state_time = true;
  digitalWrite(LED, HIGH);

  return true;
}
String get_time() {
  if (state_time) {
    return String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + ":" + String(timeinfo.tm_sec) + " " + String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon + 1) + "/" + String(timeinfo.tm_year + 1900);
  }
  else {
    return "unknown";
  }
}
void wifi_relay_log(String data) {
  //Cam01,E,1,1,time|
  String log_data = "";
  log_data = "Cam" + data.substring(2, data.length()) + ",W," + data[0] + "," + data[1] + "," + get_time() + "|";
  append_spiffs("/eve.txt", log_data);
  Serial.println("log: " + log_data);
}
void stm_relay_log(String data) {
  append_spiffs("/eve.txt", data.substring(3, data.length()));
  if (data.indexOf(",E,") >= 0) {
    if (data.indexOf("E,1,") >= 0) {
      if (data.indexOf("E,1,1") >= 0) {
        display_state_web.state_relay1 = "ON";
      }
      else {
        display_state_web.state_relay1 = "OFF";
      }
    }
    else if (data.indexOf("E,2,") >= 0) {
      if (data.indexOf("E,2,1") >= 0) {
        display_state_web.state_relay2 = "ON";
      }
      else {
        display_state_web.state_relay2 = "OFF";
      }
    }
    else if (data.indexOf("E,3,") >= 0) {
      if (data.indexOf("E,3,1") >= 0) {
        display_state_web.state_relay3 = "ON";
      }
      else {
        display_state_web.state_relay3 = "OFF";
      }
    }
    else if (data.indexOf("E,4,") >= 0) {
      if (data.indexOf("E,4,1") >= 0) {
        display_state_web.state_relay4 = "ON";
      }
      else {
        display_state_web.state_relay4 = "OFF";
      }
    }
    else if (data.indexOf("E,5,") >= 0) {
      if (data.indexOf("E,5,1") >= 0) {
        display_state_web.state_relay5 = "ON";
      }
      else {
        display_state_web.state_relay5 = "OFF";
      }
    }
  }
  //--------------------LOG BUTTTON--------------------
  else if (data.indexOf(",B,") >= 0) {
    if (data.indexOf("B,1,") >= 0) {
      if (data.indexOf("B,1,1") >= 0) {
        display_state_web.state_input1 = "ON";
      }
      else {
        display_state_web.state_input1 = "OFF";
      }
    }
    else if (data.indexOf("B,2,") >= 0) {
      if (data.indexOf("B,2,1") >= 0) {
        display_state_web.state_input2 = "ON";
      }
      else {
        display_state_web.state_input2 = "OFF";
      }
    }
    else if (data.indexOf("B,3,") >= 0) {
      if (data.indexOf("B,3,1") >= 0) {
        display_state_web.state_input3 = "ON";
      }
      else {
        display_state_web.state_input3 = "OFF";
      }
    }
  }


}
void config_init_stm() {
  if (update_ethernet == "TRUE") {
    write_spiffs("/updatee.txt", "FALSE");
    Serial2.println("STM_RESET");
  }
  unsigned long timeout = millis();
  digitalWrite(FLAG2, HIGH);
  while ((digitalRead(FLAG1) == 0) && (millis() - timeout < 15000)) {
    delay(400);
  }
  if (digitalRead(FLAG1)) {
    delay(400);
    if (update_ethernet == "TRUE") {
      Serial2.println("STM_RESET");
      write_spiffs("/updatee.txt", "FALSE");
      ESP.restart();
    }
    if (cam_api1.length() > 2) {
      updateapipostcameratostm(cam_api1, 1);
    }
    if (cam_api2.length() > 2) {
      updateapipostcameratostm(cam_api2, 2);
    }
    if (cam_api3.length() > 2) {
      updateapipostcameratostm(cam_api3, 3);
    }

    Serial2.println("IP:" + infor_ethernet.ip); delay(40);
    Serial2.println("SUB:" + infor_ethernet.subnet); delay(40);
    Serial2.println("GW:" + infor_ethernet.gateway); delay(40);
  }
  digitalWrite(FLAG2, LOW);
  delay(300);
}
void updateDevicePassword(String data) {
  write_spiffs("/dPassword.txt", data);
}
//myPath=http://camera.qnvn.vn:7001/api/createEvent?source=2&abc=1&n=,myUsername=12345,myPassword=12345
void updateapipostcamera_query(String data, uint8_t pos) {
  uint16_t index[3];
  index[0] = data.indexOf(",myUsername");
  index[1] = data.indexOf(",myPassword");
  index[2] = data.length();
  
  update_api_post.path = data.substring(7, index[0]);
  update_api_post.user = data.substring(12 + index[0], index[1]);
  update_api_post.password =  data.substring(index[1] + 12, index[2]);
  Serial.println("########################");
  Serial.println(update_api_post.path);
  Serial.println(update_api_post.user);
  Serial.println(update_api_post.password);
  Serial.println("########################");

  update_api_post.state_update = true;
  update_api_post.pos = pos;
}
void updateapipostcamera(String data, uint8_t pos) {
  uint16_t index[3];
  index[0] = data.indexOf(",");
  index[1] = data.indexOf(",myPassword");
  index[2] = data.length();
  update_api_post.path = data.substring(0, index[0]);
  update_api_post.user = data.substring(12 + index[0], index[1]);
  update_api_post.password =  data.substring(index[1] + 12, index[2]);
  Serial.println("########################");
  Serial.println(update_api_post.path);
  Serial.println(update_api_post.user);
  Serial.println(update_api_post.password);
  Serial.println("########################");

  update_api_post.state_update = true;
  update_api_post.pos = pos;
}
void updateapipostcameratostm(String data, uint8_t pos) {
  //abc|abc|abc
  uint8_t index[2];
  String path = "";
  String authorization  = "";
  String encoded = "";
  index[0] = data.indexOf("|");
  index[1] = data.length();
  path = data.substring(0, index[0]);
  authorization = data.substring(index[0] + 1, index[1]);
  authorization.replace("|", ":");
  encoded = base64::encode(authorization);
  Serial2.println("PPATH" + String(pos) + ":" + path);
  Serial.println("PPATH" + String(pos) + ":" + path);
  delay(50);
  Serial2.println("PAUT" + String(pos) + ":" + encoded);
  Serial.println("PAUT" + String(pos) + ":" + encoded);
  delay(50);

}
