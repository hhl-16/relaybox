/*
  relay box
  http server
  interrupt 3 button
  uart with esp
  flag 1,2
  update time
  save eve esp
  control 5 relay
  button post api to client another
*/


#include <SPI.h>
#include <Ethernet.h>
#include <TimeLib.h>

#define SS PA4
#define RELAY1 PA15
#define RELAY2 PB3
#define RELAY3 PB4
#define RELAY4 PB5
#define RELAY5 PB6

#define BT1 PA11
#define BT2 PB15
#define BT3 PA12

#define RESET_F PB14
#define FLAG1 PB13
#define FLAG2 PB12

#define BUZZER PB2
#define TIMEOUTBUTTON 2000
HardwareSerial Serial3(USART3);

typedef struct {
  String ip = "";
  String gateway = "";
  String subnet = "";
  String dns = "";
} inforethernet;

inforethernet infor_ethernet;
typedef struct {
  bool state1 = false;
  bool state2 = false;
  bool state3 = false;
} buttonrelay;

buttonrelay button_relay;
typedef struct {
  bool api1 = false; //check exist path api1 api2 api3
  bool api2 = false;
  bool api3 = false;
  bool status_post_relay1 = false; // check post api when press button
  bool status_post_relay2 = false;
  bool status_post_relay3 = false;
} stateapi;
stateapi state_api;
typedef struct {
  String path = "";
  String aut = "";
} apipost;
apipost api_post1;
apipost api_post2;
apipost api_post3;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
unsigned long initMillis = 0 ;

unsigned long timeout_button1 = 0;
unsigned long timeout_button2 = 0;
unsigned long timeout_button3 = 0;
bool check_timeout1 = true;
bool check_timeout2 = true;
bool check_timeout3 = true;




const long timeoutTime = 500;
String header;
String buffer_rev;
String data_request = "";
String HTTP_METHOD = "POST";
void updateThingSpeak(String path, String aut, String state);
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};


const char timeSrvr[] = "time.nist.gov";
EthernetUDP ethernet_UDP;
unsigned int localPort = 8888;
time_t prevDisplay = 0;
byte messageBuffer[48];

IPAddress ip;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;
EthernetServer server(80);
EthernetClient client;



/*
   FUNCTION USED
*/
void http_server(); //API http
void send_esp(); //send data to esp
void button_interrupt(); //ngắt  button1 button2 button3 resetbutton
void setup_gpio(); // config gpio
void setup_interrupt_func();
void setup_ethernet(); // setup init for ethernet
void config_rev_esp();
void control_relay(String data);
String get_time();
void button_control_relay(uint8_t pos);

// infor_ethernet.ip = "192.168.137.19";
// infor_ethernet.gateway = "192.168.137.1";
// infor_ethernet.subnet = "255.255.255.0";
// infor_ethernet.dns = "8.8.8.8";




void setup() {


  setup_gpio();
  Serial.begin(115200);
  Serial3.begin(115200);
  Serial.println("===========================================");
  Serial.println("=================RELAY BOX=================");
  Serial.println("===========================================");
  initMillis = millis();
  while (digitalRead(FLAG2) == 0) {
    Serial.println("waiting esp...");
    Serial.println(digitalRead(FLAG2));
    if ((millis() - initMillis) > 3000)
      break;
    delay(150);
  }
  config_rev_esp();
  setup_interrupt_func();
  setup_ethernet();
  ethernet_UDP.begin(localPort);
  setSyncProvider(getTime);
}

void loop() {
  http_server();
  config_rev_esp();
  if (data_request != "") {
    Serial.println("data_request:" + data_request);
    control_relay(data_request);
    data_request = "";
  }
  if (state_api.status_post_relay1) { // có nhấn button1
    if (state_api.api1) { //có path api post
      Serial.println("pppppppppppppppppppppppppppppppppppp api1");
      updateThingSpeak(api_post1.path, api_post1.aut, "on");
    }
    state_api.status_post_relay1 = false;
  }
  if (state_api.status_post_relay2) { // có nhấn button2
    if (state_api.api2) { //có path api post
      Serial.println("pppppppppppppppppppppppppppppppppppp api2");
      updateThingSpeak(api_post2.path, api_post2.aut, "on");
    }
    state_api.status_post_relay2 = false;
  }
  if (state_api.status_post_relay3) { // có nhấn button3
    if (state_api.api3) { //có path api post
      Serial.println("pppppppppppppppppppppppppppppppppppp api3");
      updateThingSpeak(api_post3.path, api_post3.aut, "on");
    }
    state_api.status_post_relay3 = false;
  }
  if((millis()- timeout_button1) > TIMEOUTBUTTON){
    check_timeout1 = true;
    timeout_button1 = 0;
  }
  if((millis()- timeout_button2) > TIMEOUTBUTTON){
    check_timeout2 = true;
    timeout_button2 = 0;
  }
  if((millis()- timeout_button3) > TIMEOUTBUTTON){
    check_timeout3 = true;
    timeout_button3 = 0;
  }


}


void http_server() {
  EthernetClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          // http://IP_box/api/triggerRelay?relayID={1...5}&camID={value}&state={0 hoac 1}

          if (currentLine.length() == 0) {
            // turns the GPIOs on and off
            if (header.indexOf("GET /api") >= 0) {
              //              Serial.print("===========>data:");
              // Serial.println(header.substring(0, header.indexOf("HTTP")));
              data_request = header.substring(0, header.indexOf("HTTP"));
            }

            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    //    Serial.println("Client disconnected.");
    //    Serial.println("");
  }
}
/*
  ===========================================================
*/

/*
  ===========================================================
*/
void setup_gpio() {
  pinMode(BT1, INPUT);
  pinMode(BT2, INPUT);
  pinMode(BT3, INPUT);

  pinMode(RESET_F, INPUT);
  pinMode(FLAG1, OUTPUT);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);
  digitalWrite(RELAY5, LOW);

  pinMode(FLAG2, INPUT);
  pinMode(BUZZER, OUTPUT);
}
void setup_interrupt_func() {
  attachInterrupt(digitalPinToInterrupt(BT1), button_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BT2), button_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BT3), button_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_F), button_interrupt, FALLING);
}

void setup_ethernet() {
  if (infor_ethernet.ip.length() < 5 || infor_ethernet.gateway.length() < 5) {
    infor_ethernet.ip = "192.168.137.19";
    infor_ethernet.gateway = "192.168.137.1";
    infor_ethernet.subnet = "255.255.255.0";

  }
  infor_ethernet.dns = "8.8.8.8";

  delay(300);


  ip.fromString(infor_ethernet.ip.c_str());
  gateway.fromString(infor_ethernet.gateway.c_str());
  subnet.fromString(infor_ethernet.subnet.c_str());
  dns.fromString(infor_ethernet.dns.c_str());


  delay(300);
  Ethernet.init(SS);  // ESP32 with Adafruit Featherwing Ethernet
  Serial.println("----------------");
  Serial.println(ip);
  Serial.println(gateway);
  Serial.println(subnet);
  Serial.println(dns);
  Serial.println("----------------");

  Ethernet.begin(mac, ip, dns, gateway, subnet);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    Serial.println("LAN0");
    Serial3.println("LAN0");
  }
  else {
    Serial.println("LAN1");
    Serial3.println("LAN1");
  }
  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.dnsServerIP());
}

void button_interrupt() { //ngắt button
  delay(50);
  if (!digitalRead(BT1)) {
    Serial.println("button1");
    button_control_relay(1);
  }
  else if (!digitalRead(BT2)) {
    Serial.println("button3");
    button_control_relay(3);
  }
  else if (!digitalRead(BT3)) {
    Serial.println("button2");
    button_control_relay(2);
  }
  else if (!digitalRead(RESET_F)) {
    Serial.println("BUTTON RESET");
    unsigned long timereset = millis();
    uint8_t cout = 0;
    while (millis() - timereset < 10000) {
      cout++;
      if (digitalRead(RESET_F)) {
        break;
      }
      else {
        if (cout >= 20) {
          Serial.println("-------RESET---------");
          Serial3.println("DELETE ALL FILE");
          delay(2000);
          NVIC_SystemReset();
          break;
        }
      }
      delay(100);
    }
  }
}

void config_rev_esp() {
  while (digitalRead(FLAG2) == 1) {
    digitalWrite(FLAG1, HIGH);
    // put your main code here, to run repeatedly:
    if (Serial3.available()) {
      buffer_rev = Serial3.readStringUntil('\n');
      Serial.println("buffer_rev:" + buffer_rev);
      if (buffer_rev.indexOf("IP:") >= 0) {
        infor_ethernet.ip = buffer_rev.substring(3, buffer_rev.length() - 1);
      }
      else if (buffer_rev.indexOf("SUB:") >= 0) {
        infor_ethernet.subnet = buffer_rev.substring(4, buffer_rev.length() - 1);
      }
      else if (buffer_rev.indexOf("GW:") >= 0) {
        infor_ethernet.gateway = buffer_rev.substring(3, buffer_rev.length() - 1);
      }
      else if (buffer_rev.indexOf("STM_RESET") >= 0) {
        NVIC_SystemReset();
      }
      else if (buffer_rev.indexOf("PPATH") >= 0) {
        if (buffer_rev.indexOf("PPATH1") >= 0) {
          state_api.api1 = true;
          api_post1.path = buffer_rev.substring(7, buffer_rev.length());
          //          Serial.println("---------->" + api_post1.path);
        }
        else if (buffer_rev.indexOf("PPATH2") >= 0) {
          state_api.api2 = true;
          api_post2.path = buffer_rev.substring(7, buffer_rev.length());
          //          Serial.println("---------->" + api_post2.path);
        }
        else if (buffer_rev.indexOf("PPATH3") >= 0) {
          state_api.api3 = true;
          api_post3.path = buffer_rev.substring(7, buffer_rev.length());
          //          Serial.println("---------->" + api_post3.path);
        }
      }
      else if (buffer_rev.indexOf("PAUT") >= 0) {
        if (buffer_rev.indexOf("PAUT1") >= 0) {
          api_post1.aut = buffer_rev.substring(6, buffer_rev.length());
          //          Serial.println("---------->" + api_post1.aut);
        }
        else if (buffer_rev.indexOf("PAUT2") >= 0) {
          api_post2.aut = buffer_rev.substring(6, buffer_rev.length());
          //          Serial.println("---------->" + api_post2.aut);
        }
        else if (buffer_rev.indexOf("PAUT3") >= 0) {
          api_post3.aut = buffer_rev.substring(6, buffer_rev.length());
          //          Serial.println("---------->" + api_post3.aut);
        }
      }
    }
  }
}
//GET /api/triggerRelay?relayID=1&camID=2&state=0
// //Cam01,E,1,1,time|
void control_relay(String data) {
  String send;
  String relay;
  String state;
  uint8_t index[3];
  if (data.indexOf("GET") == -1) {
    return;
  }
  index[0] = data.indexOf("relayID");
  index[1] = data.indexOf("camID");
  index[2] = data.indexOf("state");
  relay = data.substring(index[0] + 8, index[1] - 1);
  state = data.substring(index[2] + 6, index[2] + 7);
  if (relay == "1") {
    if (state == "1") {
      digitalWrite(RELAY1, HIGH);
    }
    else {
      digitalWrite(RELAY1, LOW);
    }
  }
  else if (relay == "2") {
    if (state == "1") {
      digitalWrite(RELAY2, HIGH);
    }
    else {
      digitalWrite(RELAY2, LOW);
    }
  }
  else if (relay == "3") {
    if (state == "1") {
      digitalWrite(RELAY3, HIGH);
    }
    else {
      digitalWrite(RELAY3, LOW);
    }
  }
  else if (relay == "4") {
    if (state == "1") {
      digitalWrite(RELAY4, HIGH);
    }
    else {
      digitalWrite(RELAY4, LOW);
    }
  }
  else if (relay == "5") {
    if (state == "1") {
      digitalWrite(RELAY5, HIGH);
    }
    else {
      digitalWrite(RELAY5, LOW);
    }
  }
  if (timeStatus() != timeNotSet) {   // check if the time is successfully updated
    if (now() != prevDisplay) {       // update the display only if time has changed
      prevDisplay = now();
      //      digitalClockDisplay();          // display the current date and time
    }
  }
  send = "LOGCam" + data.substring(index[1] + 6, index[2] - 1) + ",E," + relay + "," + state + "," + get_time() + "|";
  Serial.println(send);
  Serial3.println(send);
}
String get_time() {
  return String(hour()) + ":" + String(minute()) + ":" + String(second()) + " " + String(day()) + "/" + String(month()) + "/" + String(year());
}
void button_control_relay(uint8_t pos) {
  Serial.println("vao mode button control");
  if ((pos == 1) && (check_timeout1 == true)) {
    Serial.println("vao mode 1");
    state_api.status_post_relay1 = true;
    timeout_button1 = millis();
    Serial.println(timeout_button1);
    if (button_relay.state1 == false) {
      digitalWrite(RELAY1, HIGH);
      button_relay.state1 = true;
      Serial3.println("LOG0,B,1,1," + get_time() + "|");
      check_timeout1 = false;

    }
    else if (button_relay.state1 == true) {
      digitalWrite(RELAY1, LOW);
      button_relay.state1 = false;
      Serial3.println("LOG0,B,1,0," + get_time() + "|");
      check_timeout1 = false;
    }

  }
  else if ((pos == 2) && (check_timeout2 == true)) {
    state_api.status_post_relay2 = true;
    Serial.println("vao mode 2");
    timeout_button2 = millis();
    if (button_relay.state2 == false) {
      digitalWrite(RELAY2, HIGH);
      button_relay.state2 = true;
      Serial3.println("LOG0,B,2,1," + get_time() + "|");
      Serial.println("LOG0,B,2,1," + get_time() + "|");
      check_timeout2 = false;

    }
    else if (button_relay.state2 == true) {
      digitalWrite(RELAY2, LOW);
      button_relay.state2 = false;
      Serial3.println("LOG0,B,2,0," + get_time() + "|");
      Serial.println("LOG0,B,2,0," + get_time() + "|");
      check_timeout2 = false;

    }

  }
  else if ((pos == 3)&& (check_timeout3 == true)) {
    state_api.status_post_relay3 = true;
    Serial.println("vao mode 3");
    timeout_button3 = millis();
    if (button_relay.state3 == false) {
      digitalWrite(RELAY3, HIGH);
      button_relay.state3 = true;
      Serial3.println("LOG0,B,3,1," + get_time() + "|");
      check_timeout3 = false;
    }
    else if (button_relay.state3 == true) {
      digitalWrite(RELAY3, LOW);
      button_relay.state3 = false;
      Serial3.println("LOG0,B,3,0," + get_time() + "|");
      check_timeout3 = false;
    }

  }

}


time_t getTime() //get time in internet
{
  while (ethernet_UDP.parsePacket() > 0) ; // discard packets remaining to be parsed

  Serial.println("Transmit NTP Request message");

  // send packet to request time from NTP server
  sendRequest(timeSrvr);

  // wait for response
  uint32_t beginWait = millis();

  while (millis() - beginWait < 1500) {

    int size = ethernet_UDP.parsePacket();

    if (size >= 48) {
      Serial.println("Receiving NTP Response");

      // read data and save to messageBuffer
      ethernet_UDP.read(messageBuffer, 48);

      // NTP time received will be the seconds elapsed since 1 January 1900
      unsigned long secsSince1900;

      // convert to an unsigned long integer the reference timestamp found at byte 40 to 43
      secsSince1900 =  (unsigned long)messageBuffer[40] << 24;
      secsSince1900 |= (unsigned long)messageBuffer[41] << 16;
      secsSince1900 |= (unsigned long)messageBuffer[42] << 8;
      secsSince1900 |= (unsigned long)messageBuffer[43];

      // returns UTC time
      return secsSince1900 - 2208988800UL + 25200;
    }
  }

  // error if no response
  Serial.println("Error: No Response.");
  return 0;
}

/*
   helper function for getTime()
   this function sends a request packet 48 bytes long
*/
void sendRequest(const char * address)
{
  // set all bytes in messageBuffer to 0
  memset(messageBuffer, 0, 48);

  // create the NTP request message

  messageBuffer[0] = 0b11100011;  // LI, Version, Mode
  messageBuffer[1] = 0;           // Stratum, or type of clock
  messageBuffer[2] = 6;           // Polling Interval
  messageBuffer[3] = 0xEC;        // Peer Clock Precision
  // array index 4 to 11 is left unchanged - 8 bytes of zero for Root Delay & Root Dispersion
  messageBuffer[12]  = 49;
  messageBuffer[13]  = 0x4E;
  messageBuffer[14]  = 49;
  messageBuffer[15]  = 52;

  // send messageBuffer to NTP server via UDP at port 123
  ethernet_UDP.beginPacket(address, 123);
  ethernet_UDP.write(messageBuffer, 48);
  ethernet_UDP.endPacket();
}
///https://172.16.1.2:7001/api/createEvent?source=test
void updateThingSpeak(String path, String aut, String state)
{
  uint16_t index[3];
  char *host_array;
  String host_name = "";
  String port_name = "";
  String path_name = "";

  index[0] = path.indexOf(":", 14);
  index[1] = path.indexOf("/", index[0]);
  index[2] = path.length();
  host_name = path.substring(8, index[0]);
  port_name = path.substring(index[0] + 1 , index[1]);
  path_name = path.substring(index[1], index[2] - 1);
  host_array = (char*) malloc(host_name.length() + 1);
  host_name.toCharArray(host_array, host_name.length() + 1);

  Serial.println(host_array);
  Serial.println(port_name.toInt());
  Serial.println(path_name);
  Serial.println(aut);
  Serial.println("===========================");
if (client.connect(host_array, port_name.toInt())) {
//  if (client.connect("192.168.110.190", 7001)) {
    // if connected:
    Serial.println("Connected to server");
    // make a HTTP request:
    // send HTTP header/api/createEvent?source=test
    client.println(HTTP_METHOD + " " + path_name + " HTTP/1.1");
//    client.println(HTTP_METHOD + " /api/createEvent?source=test HTTP/1.1");
    client.println("Authorization: Basic " + aut );
//    client.println("Authorization: Basic YWRtaW46QWRtaW5AMTIz");
    client.println("Accept: */*");
    client.println("Cache-Control: no-cache");
    client.println("Host: " + host_name + ":" + port_name);
    client.println("Accept-Encoding: gzip, deflate, br");
    client.println("Connection: keep-alive");
    client.println("Content-Length: 0");
    client.println();

    // send HTTP body
    //    client.println(queryString);

    while (client.connected()) {
      if (client.available()) {
        // read an incoming byte from the server and print it to serial monitor:
        char c = client.read();
        Serial.print(c);
      }
    }

    // the server's disconnected, stop the client:
    client.stop();
    Serial.println();
    Serial.println("disconnected");
  } else {// if not connected:
    Serial.println("connection failed");
  }

}
