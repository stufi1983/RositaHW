/*
  Repair D1 Mini <---> D2
  5V - 5V (3V3 current is poor)
  GND - GND
  D4 - D4 (GPIO2)
  D3 - D3 (GPIO0, low to flash)
  RX - RX
  TX - TX
  D8 - D8 (GPIO15)
  RST - RST

  ESP12
  D3 - GPIO0 (high, low to flashing)
  D4 - GPI02 (high)
  D8 - GPIO15 (low)
  EN - high
  TX (TX)
  RX (RX)
  VDD (VDD)
  GND (GND)
  BOARD: WEMOS D1/MINI

  ESP14
  GPIO0 (high, low to flashing)
  MVDD; EVDD - (VDD)
  MPD5 (RX WEMOS)
  MPD4 (TX WEMOS)
  GND (GND)
  BOARD: GENERIC ESP8266 MODULE
  FLASH MODE: QIO
  FLASH FREQ: 40MHz
  CPU FREQ: 80MHz
  FLASH SIZE: 2M (1M SPIFFS)
  UPLOAD SPD: 115200

  ESP01
  GPIO0 (high, low to flashing)
  CHPD / CHIP_EN (high, to enable ESP)
  -|_|-|_------
  GND(GND)       N/C        GPIO0       RX (RX WEMOS)
  X               X           X             X
  X               X           X             x
  TX(TX WEMOS) CHPD(3V3)   /RST(3V3)      VCC(3V3)
  BOARD: GENERIC ESP8266 MODULE
  FLASH MODE: DIO
  FLASH FREQ: 40MHz
  CPU FREQ: 80MHz
  FLASH SIZE: 512K (64 SPIFFS)
  UPLOAD SPD: 115200

  D5 pull-up 10k to 3V3
  D5 Ground on Reset --> Clear EEPROM
  Please DEFINE CETAK or PANGGIL BELOW

  Tombol CETAK, PULL-UP: D5, D6, D7, D8, D0, D1, D2, A0
  D3,D4 dan D8 jangan dipakai, D3 LOW is flashing mode, D8 LOW is SS / Chip select
    Boot fails if pulled HIGH:  D8
  Boot fails if pulled LOW:  D3, D4, TX
  Safer to be input pin: D3 & D4 (Don't pull down on reset) or D5,D6,D7 (High at Boot for 110ms) or D1,D2 (safest)
*/
#define YA  true


//Ganti jadi CETAK atau PANGGIL
#define PANGGIL     YA

//Jika simpan EEPROM tutup baris berikut
#define SIMPANRAM   YA

//Jika tidak ingin DEBUG tutup baris berikut
#define DEBUG       YA


//Nama perangkat
const char* ssid = "ROS-01";
const char* password = "1234567890";

//Nama aksespoin / router, password dan IP
const char* ssid_elcon = "ROSITA";
const char* pass_elcon = "elektroump";
char ipa = 192, ipb = 168, ipc = 2, ipd = 2;

//Loket dan layanan
char kodelayanan = '0';   //A --> 0, B --> 1, ... J --> :, dst
char nomorloket = '/';    //Tanpa nomor --> /, 1 --> 1, ... 9 --> 9

//Batas waktu scanning network
#define MAXRETRY 37


/*== == == == == == == == == == == == == == == == == == == == == == == == JANGAN DIUBAH == == == == == == == == == == == == == == == == == == == == == == == =*/
#define MODEUDP 0
#define MODEHTTP 1

#ifdef DEBUG
#define DLD(x,y) Serial.println(x,y)
#define DL(x) Serial.println(x)
#define D(x) Serial.print(x)
#else
#define DLD(x,y) {}
#define DL(x) {}
#define D(x) {}
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Ticker.h>

String loketNum = "/";

IPAddress ipAdrress(192, 168, 2, 2);  //hack 100
IPAddress gateway(192, 168, 2, 1);  //hack 100
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

ESP8266WebServer server(80);

String content;
int statusCode;
char modeStatus = MODEHTTP;
String thisIP;
bool lanjut = false;

WiFiUDP Udp;
unsigned int localUdpPort = 51000;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
//:5.9:0.0:0.0:0.0:0.0
char  replyPacekt[] = "Hi there! Got the message :-)";  // a reply string to send back

int counter = 0;
char layanan;// = char(EEPROM.read(99));
char nomor;// = char(EEPROM.read(100));
char IPA;// = char(EEPROM.read(101));
char IPB;// = char(EEPROM.read(102));
char IPC;// = char(EEPROM.read(103));
char IPD;// = char(EEPROM.read(104));

#ifdef PANGGIL
unsigned char LEDcounter = 0;
bool LEDOn = false;
Ticker blinker;
void changeState()
{ if (LEDOn) {
    digitalWrite(D1, !(digitalRead(D1)));  //Invert Current State of LED
  }
}
#endif
void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

#ifdef SIMPANRAM
  layanan = kodelayanan;
  nomor = nomorloket;
  IPA = ipa; IPB = ipb; IPC = ipc; IPD = ipd;
  IPAddress ip(IPA, IPB, IPC, IPD);  //hack 100

#else#
  EEPROM.begin(512);                    //Start EEPROM, 512KB
  delay(1000);                          //Give time for EEPROM to be ready

  if (!digitalRead(D5)) {               //reset setting value, connect D5 to ground when power up
    delay(1000);
    if (!digitalRead(D5)) {               //reset setting value, connect D5 to ground when power up
      for (int i = 0; i < 117; ++i) {     //clear eeprom (set to zero), this will affect RAM, not EEPROM
        EEPROM.write(i, 0);
      }
      EEPROM.commit();                    //Apply (RAM Value) to EEPROM
      delay(1000);                        //Give time for EEPROM to write
      DL("RESET");
    }
  }
  String esid;
  for (int i = 0; i < 32; ++i)          //Read EEPROM SSID value, 32 bytes
  {
    esid += char(EEPROM.read(i));
  }

  String epass = "";
  for (int i = 32; i < 96; ++i)         //Read EEPROM password value, 64 bytes
  {
    epass += char(EEPROM.read(i));
  }
  layanan = char(EEPROM.read(99));
  nomor = char(EEPROM.read(100));
  IPA = char(EEPROM.read(101));
  IPB = char(EEPROM.read(102));
  IPC = char(EEPROM.read(103));
  IPD = char(EEPROM.read(104));
  IPAddress ip(IPA, IPB, IPC, IPD);  //hack 100

  D("ssid: ");
  DL(esid);
  D("pass: ");
  DL(epass);

#endif


  // prepare GPIO2
  pinMode(2, OUTPUT);

  //WiFi.config(ip, gateway, subnet);  //static IP
  WiFi.config(ip, subnet, gateway, dns);

#ifdef SIMPANRAM
  modeStatus = MODEUDP;
  WiFi.begin(ssid_elcon, pass_elcon);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
#else
  //Trying to connect using ssid&password setting
  if (esid[0] != 0)
  {
    //Connect to WiFi network
    modeStatus = MODEUDP;
    WiFi.mode(WIFI_STA);
    D("Connect to:"); DL(esid);
    WiFi.begin(esid.c_str(), epass.c_str());

    D("Connecting to: ");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); counter ++;
      D(".");
      if (counter == MAXRETRY) {                      //failed to connect, change to HTTP Server
        modeStatus = MODEHTTP;
        break;
      }
    }
  } else {
    //if SSID & Password null, change to HTTP Server
    modeStatus = MODEHTTP;
    counter = MAXRETRY;
  }


  DL("");
  // Start the server
  server.begin();

  if (modeStatus == MODEHTTP) {
    //Create AP
    D("Default sid:"); DL(ssid);
    //WiFi.begin(ssid, password);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    D("AP IP address: ");
    DL(myIP);
    server.on("/", handleRoot);
    server.on("/settingset", handleSettingset);
    server.on("/setting", handleSetting);
    server.begin();
    DL("HTTP server started");
  }
#endif

  if (modeStatus == MODEUDP)
  {
    Udp.begin(localUdpPort);
    thisIP = WiFi.localIP().toString();
    long rssi = WiFi.RSSI();
    D("UDP IP address: ");
    DL(thisIP);
    D("port: ");
    DL(localUdpPort);
    D("strength: ");
    DL(rssi);                //rssi must be >= -70
    digitalWrite(2, 0);                   //if ESP01, change 0 to 1(HIGH)
    DL("LED ON");
  }
}


void handleRoot() {
  String header;
  String content = "<html><body><h2>ROSITA</h2><br />";
  if (server.hasHeader("User-Agent")) {
    content += "<p>Diakses lewat: " + server.header("User-Agent") + "</p><br />";
  }
  content += "<p>Robot Asisten Perawat (C) 2020, Teknik Elektro Universitas Muhammadiyah Purwokerto</p></body></html>";
  server.send(200, "text/html", content);
}

void handleSettingset() {
  String qreset = server.arg("reset");
  if (qreset == "true") {
    delay(1000);
    ESP.reset();
  }
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");

  String qipa = server.arg("ipa");
  String qipb = server.arg("ipb");
  String qipc = server.arg("ipc");
  String qipd = server.arg("ipd");

  String qnomor = server.arg("nomor");

  if (qsid.length() > 0 && qpass.length() > 0) {
    DL("clearing data");
    for (int i = 0; i < 104; ++i) {
      EEPROM.write(i, 0);
    }

    //DL("writing eeprom ssid:");
    for (int i = 0; i < qsid.length(); ++i)
    {
      EEPROM.write(i, qsid[i]);
      //D("Wrote: ");
      //DL(qsid[i]);
    }

    //DL("writing eeprom pass:");
    for (int i = 0; i < qpass.length(); ++i)
    {
      EEPROM.write(32 + i, qpass[i]);
      //D("Wrote: ");
      //DL(qpass[i]);
    }

    //    char thelayanan = qlayanan[0] - 17;
    //    DL("writing layanan:");
    //    EEPROM.write(99, thelayanan);
    //    D("Wrote: ");
    //    DL(thelayanan);

    char thenomor = qnomor[0] - 1;
    DL("writing nomor:");
    EEPROM.write(100, thenomor);
    D("Wrote: ");
    DL(thenomor);

    DL("writing eeprom ip:");
    char ipa = qipa.toInt();
    EEPROM.write(101, ipa);
    DLD(ipa, DEC);
    char ipb = qipb.toInt();
    EEPROM.write(102, ipb);
    DLD(ipb, DEC);
    char ipc = qipc.toInt();
    EEPROM.write(103, ipc);
    DLD(ipc, DEC);
    char ipd = qipd.toInt();
    EEPROM.write(104, ipd);
    DLD(ipd, DEC);

    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<p>Setting telah disimpan, silahan klik [<a href='?reset=true'>RESET</a>]</p></html>";
    server.send(200, "text/html", content);

    EEPROM.commit();
    delay(1000);
    //content = "{\"Success\":\"setting disimpan... reset atau matikan dan pasang kembali\"}";
    //statusCode = 200;
    //    delay(2000);
    //    ESP.reset();
  } else {
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<p>Setting kembali ke dafault</p></html>";
    server.send(200, "text/html", content);
    DL("menghapus setting ...");
    for (int i = 0; i < 104; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    delay(1000);
  }
  server.send(statusCode, "application/json", content);
  delay(1000);
  //auto reset
  ESP.reset();
}

void handleSetting() {
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  IPAddress ips = WiFi.softAPIP();
  String ipStrs = String(ips[0]) + '.' + String(ips[1]) + '.' + String(ips[2]) + '.' + String(ips[3]);
  content = "<!DOCTYPE html>\r\n<html><body><h2>IP Rosita ";
  content += ipStrs;
  content += "</h2>";
  content += "<p>";
  content += "</p><form method='get' action='settingset'>";

  content += "<label>Nama AP(SSID): <input type='text' name='ssid' size='32' maxlength='32' required><br />";
  content += "<label>Password: <input type='text' name='pass' size='32' maxlength='32' pattern='^.{8,}$' required><br />";
  content += "<label>IP Rosita: <input type='text' name='ipa' size='3' maxlength='3' pattern='([0-9]|[1-8][0-9]|9[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' required>.<input type='text' name='ipb' size='3' maxlength='3' pattern='([0-9]|[1-8][0-9]|9[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' required>.<input type='text' name='ipc' pattern='([0-9]|[1-8][0-9]|9[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' size='3' maxlength='3' required>.<input type='text' name='ipd' size='3' maxlength='3' pattern='([0-9]|[1-8][0-9]|9[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' required><br/>";
  content += "<label>Nomor <input type='text' name='nomor' size='1' maxlength='1' pattern='[0-9]{1}' required><br /><br />";
  content += "<input type='submit' value='Simpan'>";

  content += "</form>";
  content += "<p><strong>Catatan:</strong><br />Nama AP tidak dapat menggunakan simbol<br />Pasword minimal 8 karakter<br /><br /> Nomor diisi dari 1 hingga 9<br /></p>";
  content += "<h4>Robot Asisten Perawat (C) 2020, Teknik Elektro Universitas Muhammadiyah Purwokerto</h4></body></html>";
  server.send(200, "text/html", content);

}

void loop(void) {
  if (modeStatus == MODEHTTP) {
    server.handleClient();
    //DL("Http ready");
  }
  else if (modeStatus == MODEUDP) {
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
      // receive incoming UDP packets
      int len = Udp.read(incomingPacket, 255);
      if (len > 0)
      {
        incomingPacket[len] = 0;
        if (incomingPacket[0] == '{')
        {
          Serial.println(incomingPacket);
        }
        if (incomingPacket[0] == '?') {
          IPAddress SendIP(192, 168, 2, 9); //UDP Broadcast IP data sent to all devicess on same network
          Udp.beginPacket(SendIP, 50000);
          int ADC = analogRead(A0);
          String x = "?";
          x += String(WiFi.RSSI());
          x += ":";
          x += String(ADC);
          x += "\n";
          int str_len = x.length() + 1;
          char char_array[str_len];
          x.toCharArray(char_array, str_len);
          Udp.write(char_array);
          Serial.println(x);
          Udp.endPacket();
        }
      }
      //send back a reply, to the IP address and port we got the packet from
      //Udp.beginPacket(Udp.remoteIP(), 50000);
      //Udp.write(replyPacekt);
      //Udp.endPacket();
    }
  }
}

/*

  REMARK:
  boot on low:

  D3
  ets Jan  8 2013,rst cause:2, boot mode:(1,6)
  D4
  Fatal exception (0):
  epc1=0x40100003, epc2=0x00000000, epc3=0x00000000, excvaddr=0x00000000, depc=0x00000000
  D5
  ets Jan  8 2013,rst cause:2, boot mode:(3,6)

*/

