/*
 * JADWAL WAKTU SHOLAT MENGGUNAKAN NODEMCU ESP8266, LED P10, RTC DS3241, BUZZER
 * FITUR :  JADWAL SHOLAT 5 WAKTU DAN TANBIH IMSAK, JAM BESAR, TANGGAL, SUHU, ALARAM ADZAN DAN TANBIH IMSAK,
 *          DAN HITUNG MUNDUR IQOMAH DAN UBAH WAKTU LEWAT WIFI DENGAN BROWSER.
 * 

Pin on  DMD P10     GPIO      NODEMCU               Pin on  DS3231      NODEMCU                   Pin on  Buzzer       NODEMCU
        2  A        GPIO16    D0                            SCL         D1 (GPIO 5)                       +            RX (GPIO 3)
        4  B        GPIO12    D6                            SDA         D2 (GPIO 4)                       -            GND
        8  CLK      GPIO14    D5                            VCC         3V
        10 SCK      GPIO0     D3                            GND         GND
        12 R        GPIO13    D7
        1  NOE      GPIO15    D8
        3  GND      GND       GND

Catatan : 
o Perlu Power Eksternal 5V ke LED P10.
o Saat Flashing (upload program) cabut sementara pin untuk buzzer.

Eksternal Library
- HJS589(DMD porting for ESP8266 by dmk007)
  < DMD (https://rweather.github.io/arduino-projects/classDMD.html)
- PrayerTime : https://github.com/asmaklad/Arduino-Prayer-Times
- NTPClient
- ArduinoJson V6 : https://github.com/bblanchon/ArduinoJson



Updated : 10 Februari 2019
*/


#include <SPI.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//#include <time.h>
//#include <NTPClient.h>
#include "NTPClient.h"
#include <WiFiUdp.h>
#include <TimeLib.h>    

#include <HJS589.h>

#include <fonts/ElektronMart6x8.h>
#include <fonts/ElektronMart6x16.h>
#include <fonts/ElektronMart5x6.h>
#include <fonts/ElektronMartArabic6x16.h>
#include <fonts/ElektronMartArabic5x6.h>

#include "PrayerTimes.h"
#include "WebPage.h"

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


// JADWAL SHOLAT
double times[sizeof(TimeName)/sizeof(char*)];

// Durasi waktu iqomah
struct Config {
  int iqmhs;
  int iqmhd;
  int iqmha;
  int iqmhm;
  int iqmhi;
  int durasiadzan;
  int ihti; // Koreksi Waktu Menit Jadwal Sholat
  float latitude;
  float longitude;
  int zonawaktu;
  char nama[64];
  char info1[512];
  char info2[512];
};

int iqmh;
int menitiqmh;
int detikiqmh = 60;

struct ConfigWifi {
  char wifissid[64];
  char wifipassword[64];
};

struct ConfigDisp {
  int cerah;
};



// BUZZER
const int buzzer = 3; // Pin GPIO Buzzer - RX


// LED Internal
uint8_t pin_led = 2;


//SETUP RTC
//year, month, date, hour, min, sec and week-day(Senin 0 sampai Ahad 6)
//DateTime dt(2018, 12, 20, 16, 30, 0, 3);

char weekDay[][7] = {"AHAD", "SENIN", "SELASA", "RABU", "KAMIS", "JUM'AT", "SABTU", "AHAD"}; // array hari, dihitung mulai dari senin, hari senin angka nya =0,
char monthYear[][4] = { "DES", "JAN", "FEB", "MAR", "APR", "MEI", "JUN", "JUL", "AGU", "SEP", "OKT", "NOV", "DES" };


//SETUP DMD
#define DISPLAYS_WIDE 1
#define DISPLAYS_HIGH 1
HJS589 Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Jumlah Panel P10 yang digunakan (KOLOM,BARIS)


//WEB Server
ESP8266WebServer server(80);

const char* password = "123456789";
const char* mySsid = "ESP8266"; //kalau gagal konek

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress netmask(255, 255, 255, 0);


const char *fileconfigdisp = "/configdisp.json";
ConfigDisp configdisp;

const char *fileconfigjws = "/configjws.json";
Config config;

const char *fileconfigwifi = "/configwifi.json";
ConfigWifi configwifi;



//----------------------------------------------------------------------
// XML UNTUK JEMBATAN DATA MESIN DENGAN WEB

void buildXML(){

  
  XML="<?xml version='1.0'?>";
  XML+="<t>";
    XML+="<rWifissid>";
    XML+= configwifi.wifissid;
    XML+="</rWifissid>";
    XML+="<rYear>";
    XML+=timeClient.getYear();
    XML+="</rYear>";
    XML+="<rMonth>";
    XML+=timeClient.getMonth();
    XML+="</rMonth>";
    XML+="<rDay>";
    XML+=timeClient.getDate();
    XML+="</rDay>";
    XML+="<rHour>";
      if(timeClient.getHours()<10){
        XML+="0";
        XML+=timeClient.getHours();
      }else{    XML+=timeClient.getHours();}
    XML+="</rHour>";
    XML+="<rMinute>";
      if(timeClient.getMinutes()<10){
        XML+="0";
        XML+=timeClient.getMinutes();
      }else{    XML+=timeClient.getMinutes();}
    XML+="</rMinute>";
    XML+="<rSecond>";
      if(timeClient.getSeconds()<10){
        XML+="0";
        XML+=timeClient.getSeconds();
      }else{    XML+=timeClient.getSeconds();}
    XML+="</rSecond>";
    XML+="<rTemp>";
    XML+= random(10,30);
    XML+="</rTemp>";
    XML+="<rIqmhs>";
    XML+= config.iqmhs;
    XML+="</rIqmhs>";
    XML+="<rIqmhd>";
    XML+= config.iqmhd;
    XML+="</rIqmhd>";
    XML+="<rIqmha>";
    XML+= config.iqmha;
    XML+="</rIqmha>";
    XML+="<rIqmhm>";
    XML+= config.iqmhm;
    XML+="</rIqmhm>";
    XML+="<rIqmhi>";
    XML+= config.iqmhi;
    XML+="</rIqmhi>";
    XML+="<rDurasiAdzan>";
    XML+= config.durasiadzan;
    XML+="</rDurasiAdzan>";
    XML+="<rIhti>";
    XML+= config.ihti;
    XML+="</rIhti>";
    XML+="<rLatitude>";
    XML+= config.latitude;
    XML+="</rLatitude>";
    XML+="<rLongitude>";
    XML+= config.longitude;
    XML+="</rLongitude>";
    XML+="<rZonaWaktu>";
    XML+= config.zonawaktu;
    XML+="</rZonaWaktu>";
    XML+="<rNama>";
    XML+= config.nama;
    XML+="</rNama>";
    XML+="<rInfo1>";
    XML+= config.info1;
    XML+="</rInfo1>";
    XML+="<rInfo2>";
    XML+= config.info2;
    XML+="</rInfo2>";
    XML+="<rCerah>";
    XML+= configdisp.cerah;
    XML+="</rCerah>";
  XML+="</t>"; 
}

void handleXML(){
  buildXML();
  server.send(200,"text/xml",XML);
}



//----------------------------------------------------------------------
// PENGATURAN WIFI AUTO MODE STATION ATAU ACCESS POINT

void wifiConnect() {

  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(1000);

  Serial.println("Wifi Sation Mode");
  WiFi.mode(WIFI_STA);
  WiFi.begin(configwifi.wifissid,configwifi.wifipassword);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(pin_led, !digitalRead(pin_led));
    if (millis() - startTime > 30000) break;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(pin_led, HIGH);
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wifi AP Mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, netmask);
    WiFi.softAP(mySsid, password);
    digitalWrite(pin_led, LOW);
  }

  //Serial.println("");
  WiFi.printDiag(Serial);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

}



//----------------------------------------------------------------------
// HJS589 P10 FUNGSI TAMBAHAN UNTUK NODEMCU ESP8266

void ICACHE_RAM_ATTR refresh() { 
  
  Disp.refresh();
  timer0_write(ESP.getCycleCount() + 40000);  

}

void Disp_init() {
  
  Disp.start();
  timer0_attachInterrupt(refresh);
  timer0_write(ESP.getCycleCount() + 40000);
  Disp.clear();
  
}



//----------------------------------------------------------------------
// HJS589 P10 FUNGSI TAMBAHAN UNTUK NODEMCU ESP8266

void LoadDataAwal() {


  if (config.iqmhs == 0) {
    config.iqmhs = 12;    
  }

  if (config.iqmhd == 0) {
    config.iqmhd = 8;    
  }

  if (config.iqmha == 0) {
    config.iqmha = 6;    
  }

  if (config.iqmhm == 0) {
    config.iqmhm = 5;    
  }

  if (config.iqmhi == 0) {
    config.iqmhi = 5;    
  }

  if (config.durasiadzan == 0) {
    config.durasiadzan = 1;    
  }

  if (config.ihti == 0) {
    config.ihti = 2;    
  }

  if (config.latitude == 0) {
    config.latitude = -6.16;    
  }

  if (config.longitude == 0) {
    config.longitude = 106.61;    
  }

  if (config.zonawaktu == 0) {
    config.zonawaktu = 7;    
  }

  if (strlen(config.nama) == 0) {
    strlcpy(config.nama, "MASJID AL KAUTSAR", sizeof(config.nama));
  }

  if (strlen(config.info1) == 0) {
    strlcpy(config.info1, "info 1", sizeof(config.info1));
  }

  if (strlen(config.info2) == 0) {
    strlcpy(config.info2, "info 2", sizeof(config.info2));
  }

  if (strlen(configwifi.wifissid) == 0) {
    strlcpy(configwifi.wifissid, "JWSP10", sizeof(configwifi.wifissid));
  }

  if (strlen(configwifi.wifipassword) == 0) {
    strlcpy(configwifi.wifipassword, "password", sizeof(configwifi.wifipassword));
  }

  if (configdisp.cerah == 0) {
    configdisp.cerah = 100;    
  }
  
}



//----------------------------------------------------------------------
// SETUP

void setup() {

  Serial.begin(9600);

  //Buzzer
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  delay(50);

 
  //WIFI

  pinMode(pin_led, OUTPUT);

  SPIFFS.begin();
  
  loadWifiConfig(fileconfigwifi, configwifi);
  loadJwsConfig(fileconfigjws, config);
  loadDispConfig(fileconfigdisp, configdisp);

  LoadDataAwal();
   
  WiFi.hostname("elektronmart");
  WiFi.begin(configwifi.wifissid, configwifi.wifipassword);

  wifiConnect();

//  configTime(config.zonawaktu * 3600, 0, "pool.ntp.org", "time.nist.gov");
  timeClient.begin();
  timeClient.update();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(config.zonawaktu * 3600);
  
  server.on("/", []() {
    server.send_P(200, "text/html", setwaktu);

    // Kalau ada perubahan tanggal
    if (server.hasArg("date")) {
    
      uint16_t jam;
      uint8_t menit;
      uint8_t detik;      
      String sd=server.arg("date");
      String lastSd;
      
      jam = ((sd[0]-'0')*1000)+((sd[1]-'0')*100)+((sd[2]-'0')*10)+(sd[3]-'0');
      menit = ((sd[5]-'0')*10)+(sd[6]-'0');
      detik = ((sd[8]-'0')*10)+(sd[9]-'0');
      
      if (sd != lastSd){
        
        uint8_t hour = timeClient.getHours();
        uint8_t minute = timeClient.getMinutes();
//        Rtc.SetDateTime(RtcDateTime(jam, menit, detik, hour, minute, 0));
        lastSd=sd;
      }
  
     server.send ( 404 ,"text", message );
    
    }

    // Kalau ada perubahaan jam
    if (server.hasArg("time")) {
      
      String st=server.arg("time");
      String lastSt;
      uint8_t jam = ((st[0]-'0')*10)+(st[1]-'0');
      uint8_t menit = ((st[3]-'0')*10)+(st[4]-'0');
      
      if (st != lastSt){
         
         uint16_t year = timeClient.getYear();
         uint8_t month = timeClient.getMonth();
         uint8_t day = timeClient.getDate();
//         Rtc.SetDateTime(RtcDateTime(year, month, day, jam, menit, 0));
         lastSt=st;}
      server.send ( 404 ,"text", message );
  
    }
  });

  server.on("/toggle", toggleLED);

  server.on("/setwifi", []() {
    server.send_P(200, "text/html", setwifi);
  });  
  
  server.on("/settingwifi", HTTP_POST, handleSettingWifiUpdate); 
  
  server.on("/setjws", []() {
    server.send_P(200, "text/html", setjws);
  });
  
  server.on("/settingjws", HTTP_POST, handleSettingJwsUpdate);

  server.on("/setdisplay", []() {
    server.send_P(200, "text/html", setdisplay);
  });  

  server.on("/settingdisp", HTTP_POST, handleSettingDispUpdate);

  server.on ( "/xml", handleXML) ;  
  
  server.begin();
  Serial.println("HTTP server started");  
  

  //Buzzer

  BuzzerPendek();

  //DMD
  Disp_init();
  
  Disp.setBrightness(configdisp.cerah);

  
}



void loadDispConfig(const char *fileconfigdisp, ConfigDisp &configdisp) {

  File configFileDisp = SPIFFS.open(fileconfigdisp, "r");

  if (!configFileDisp) {
    Serial.println("Gagal membuka fileconfigdisp untuk dibaca");
    return;
  }

  size_t size = configFileDisp.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileDisp.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("Gagal parse fileconfigdisp");
    return;
  }
  
  configdisp.cerah = doc["cerah"];

  configFileDisp.close();

}



void handleSettingDispUpdate() {

  timer0_detachInterrupt();
  
  String datadisp = server.arg("plain");
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, datadisp);

  File configFileDisp = SPIFFS.open(fileconfigdisp, "w");
  
  if (!configFileDisp) {
    Serial.println("Gagal membuka Display configFile untuk ditulis");
    return;
  }
  
  serializeJson(doc, configFileDisp);

  if (error) {
    
    Serial.print(F("deserializeJson() gagal kode sebagai berikut: "));
    Serial.println(error.c_str());
    return;
    
  } else {
    
    configFileDisp.close();
    Serial.println("Berhasil mengubah configFileDisp");

    server.send(200, "application/json", "{\"status\":\"ok\"}");

    loadDispConfig(fileconfigdisp, configdisp);
    
    delay(500);
    timer0_attachInterrupt(refresh);
    timer0_write(ESP.getCycleCount() + 40000);

    Disp.setBrightness(configdisp.cerah);
  
  }  

}






void loadJwsConfig(const char *fileconfigjws, Config &config) {

  File configFileJws = SPIFFS.open(fileconfigjws, "r");
  
  if (!configFileJws) {
    Serial.println("Gagal membuka fileconfigjws untuk dibaca");
    return;
  }

  size_t size = configFileJws.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileJws.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("Gagal parse fileconfigjws");
    return;
  }

  config.iqmhs = doc["iqmhs"];
  config.iqmhd = doc["iqmhd"];
  config.iqmha = doc["iqmha"];
  config.iqmhm = doc["iqmhm"];
  config.iqmhi = doc["iqmhi"];
  config.durasiadzan = doc["durasiadzan"];
  config.ihti = doc["ihti"];
  config.latitude = doc["latitude"];
  config.longitude = doc["longitude"];
  config.zonawaktu = doc["zonawaktu"];
  strlcpy(config.nama, doc["nama"] | "MASJID AL KAUTSAR", sizeof(config.nama));  // Set awal Nama
  strlcpy(config.info1, doc["info1"] | "info 1", sizeof(config.info1));  // Set awal Info1 
  strlcpy(config.info2, doc["info2"] | "info 2", sizeof(config.info2));  // Set awal Info2

  configFileJws.close();

}



void handleSettingJwsUpdate() {

  timer0_detachInterrupt();

  String datajws = server.arg("plain");
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, datajws);

  File configFileJws = SPIFFS.open(fileconfigjws, "w");
  
  if (!configFileJws) {
    Serial.println("Gagal membuka JWS configFile untuk ditulis");
    return;
  }
  
  serializeJson(doc, configFileJws);

  if (error) {
    
    Serial.print(F("deserializeJson() gagal kode sebagai berikut: "));
    Serial.println(error.c_str());
    return;
    
  } else {
    
    configFileJws.close();
    Serial.println("Berhasil mengubah configFileJws");
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");    
    
    loadJwsConfig(fileconfigjws, config);
    
    delay(500);
    timer0_attachInterrupt(refresh);
    timer0_write(ESP.getCycleCount() + 40000);
  
  }  

}



void loadWifiConfig(const char *fileconfigwifi, ConfigWifi &configwifi) {

  File configFileWifi = SPIFFS.open(fileconfigwifi, "r");
  
  if (!configFileWifi) {
    Serial.println("Gagal membuka fileconfigwifi untuk dibaca");
    return;
  }

  size_t size = configFileWifi.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileWifi.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("Gagal untuk parse config file");
    return;
  }

  strlcpy(configwifi.wifissid, doc["wifissid"] | "grobak.net", sizeof(configwifi.wifissid));
  strlcpy(configwifi.wifipassword, doc["wifipassword"] | "12345", sizeof(configwifi.wifipassword));

  configFileWifi.close();

}



void handleSettingWifiUpdate() {

  timer0_detachInterrupt();
  
  String data = server.arg("plain");

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, data);

  File configFile = SPIFFS.open("/configwifi.json", "w");
  if (!configFile) {
    Serial.println("Error opening Wifi configFile for writing");
    return;
    
  }
  
  serializeJson(doc, configFile);

  if (error) {
    
    Serial.print(F("deserializeJson() gagal kode sebagai berikut: "));
    Serial.println(error.c_str());
    return;
    
  } else {

    configFile.close();
    Serial.println("Berhasil mengubah configFileWifi");
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    loadWifiConfig(fileconfigwifi, configwifi);

    delay(500);
    timer0_attachInterrupt(refresh);
    timer0_write(ESP.getCycleCount() + 40000);

  } 

}



//----------------------------------------------------------------------
// LOOP

uint8_t tampilanutama;

void loop() {

  server.handleClient();

  switch(tampilanutama) {
    case 0 :
      tampilan();
      break;
    case 1 :
      Iqomah();
      break;
  }  


}



//----------------------------------------------------------------------
//MODE TAMPILAN JAM

uint8_t tampilanjam;

void tampilan() {
  
  if (tampilanjam > 8) {
    tampilanjam = 0;
  }

  switch(tampilanjam) {

    case 0 :
      animLogoX();
      AlarmSholat();
      break;  
      
    case 1 :
      JamJatuhPulse();
      AlarmSholat();
      break;

    case 2 :
      JamArabJatuhPulse();
      AlarmSholat();
      break;

    case 3 :
      TampilTanggal();
      AlarmSholat();
      break;

    case 4 :
//      TampilSuhu();
      TampilJamTanggal();
      AlarmSholat();
      break;
      
    case 5 :
      TampilJadwalSholat();
      AlarmSholat();
      break;

    case 6 :
      TeksJalanNama();
      AlarmSholat();
      break;
    case 7 :
      TeksJalanInfo1();
      AlarmSholat();
      break;
    case 8 :
      TeksJalanInfo2();
      AlarmSholat();
      break;
  }

}



void JamJatuhPulse() {
    
  static uint8_t y;
  static uint8_t d;           
  static uint32_t pM;
  uint32_t cM = millis();

  static uint32_t pMPulse;
  static uint8_t pulse;
  
  
  if (cM - pMPulse >= 100) {
    pMPulse = cM;
    pulse++;
  }
  
  if (pulse > 8) {
    pulse=0;
  }

  if(cM - pM > 25) { 
    if(d == 0 and y < 32) {
      pM=cM;
      y++;
    }
    if(d  == 1 and y > 0) {
      pM=cM;
      y--;
    }    
  }
  
  if(cM - pM > 15000 and y == 32) {
    d=1;
  }
  
  if(y == 32) {
    Disp.drawRect(15,3+pulse,18,11-pulse,0,1);       
  }
  
  if(y < 32) {
    Disp.drawRect(15,3,18,11,0,0);
  }
   
  if(y == 0 and d == 1) {
    d=0;
    Disp.clear();
    tampilanjam = 2;
  }
  
  TampilJamDinamis(y - 32);
  
    
}



void JamArabJatuhPulse() {
    
  static uint8_t y;
  static uint8_t d;           
  static uint32_t pM;
  uint32_t cM = millis();

  static uint32_t pMPulse;
  static uint8_t pulse;
  
  
  if (cM - pMPulse >= 100) {
    pMPulse = cM;
    pulse++;
  }
  
  if (pulse > 8) {
    pulse=0;
  }

  if(cM - pM > 25) { 
    if(d == 0 and y < 32) {
      pM=cM;
      y++;
    }
    if(d  == 1 and y > 0) {
      pM=cM;
      y--;
    }    
  }
  
  if(cM - pM > 15000 and y == 32) {
    d=1;
  }
  
  if(y == 32) {
    Disp.drawRect(14,3+pulse,17,11-pulse,0,1);       
  }
  
  if(y < 32) {
    Disp.drawRect(14,3,17,11,0,0);
  }
   
  if(y == 0 and d == 1) {
    d=0;
    Disp.clear();
    tampilanjam = 3;
  }
  
  TampilJamArabDinamis(y - 32);
  
    
}


void JamJatuh() {
    
  static uint8_t y;
  static uint8_t d;              
  static uint32_t pM;
  uint32_t cM = millis();
  
  static uint32_t pMKedip;
  static boolean kedip;  

  if (cM - pMKedip >= 500) {
    pMKedip = cM;
    kedip = !kedip;    
  }

  if(cM - pM > 50) { 
    if(d == 0 and y < 32){
      pM = cM;
      y++;
    }
    
    if(d == 1 and y > 0){
      pM=cM;
      y--;
    }    
  }
  
  if(cM - pM > 15000 and y == 32) {
    d=1;    
  }
  
  if (y==32) {
    
    if (kedip) {
        // TITIK DUA ON
        Disp.drawRect(18,3,19,5,1,1);
        Disp.drawRect(18,9,19,11,1,1);
    } else {
        // TITIK DUA OFF
        Disp.drawRect(18,3,19,5,0,0);
        Disp.drawRect(18,9,19,11,0,0);          
    }
      
  }
  
  if (y < 32) {
    Disp.drawRect(18,3,19,5,0,0);
    Disp.drawRect(18,9,19,11,0,0); 
  }
   
  if (y == 3 and d==1) {
    d=0;
    Disp.clear();
    tampilanjam = 2;
  }  
  
  TampilJamDinamis(y-32); 
    
}


void TampilJamDinamis(uint32_t y) {

  
  char jam[3];
  char menit[3];
  char detik[3];
  sprintf(jam,"%02d", timeClient.getHours());
  sprintf(menit,"%02d", timeClient.getMinutes());
  sprintf(detik,"%02d", timeClient.getSeconds());

  //JAM
  Disp.setFont(ElektronMart6x16);
  Disp.drawText(1, y, jam);

  //MENIT          
  Disp.setFont(ElektronMart5x6);
  Disp.drawText(20, y, menit);

  //DETIK          
  Disp.setFont(ElektronMart5x6);
  Disp.drawText(20, y+8, detik);

 
}


void TampilJamArabDinamis(uint32_t y) {

  
  char jam[3];
  char menit[3];
  char detik[3];
  sprintf(jam,"%02d", timeClient.getHours());
  sprintf(menit,"%02d", timeClient.getMinutes());
  sprintf(detik,"%02d", timeClient.getSeconds());

  //JAM
  Disp.setFont(ElektronMartArabic6x16);
  Disp.drawText(0, y, jam);

  //MENIT          
  Disp.setFont(ElektronMartArabic5x6);
  Disp.drawText(20, y, menit);

  //DETIK          
  Disp.setFont(ElektronMartArabic5x6);
  Disp.drawText(20, y+8, detik);
 
}



//----------------------------------------------------------------------
//TAMPILKAN JAM BESAR

void TampilJam() {

  static uint8_t d;
  static uint32_t pM;
  static uint32_t pMJam;
  static uint32_t pMKedip;
  uint32_t cM = millis();
  static boolean kedip;
  
  
  char jam[3];
  char menit[3];
  char detik[3];
  
  if (cM - pMJam >= 1000) {
   
    pMJam = cM;
    d++;
    
    //JAM
    sprintf(jam,"%02d", timeClient.getHours());
    Disp.setFont(ElektronMart6x16);
    Disp.drawText(3, 0, jam);
  
    //MENIT
    sprintf(menit,"%02d", timeClient.getMinutes());
    Disp.setFont(ElektronMart5x6);
    Disp.drawText(22, 0, menit);
  
    //DETIK
    sprintf(detik,"%02d", timeClient.getSeconds());
    Disp.setFont(ElektronMart5x6);
    Disp.drawText(22, 8, detik);

    if (d >= 10) {
      d = 0;
      Disp.clear();
      tampilanjam = 2;
    }
        
  }

  //KEDIP DETIK
  if (millis() - pMKedip >= 500) {
    pMKedip = millis();
    kedip = !kedip;
  }

  if (kedip) {    
      Disp.drawRect(18,3,19,5,1,1);
      Disp.drawRect(18,9,19,11,1,1);
  } else {
      Disp.drawRect(18,3,19,5,0,0);
      Disp.drawRect(18,9,19,11,0,0);
  }
  
}



void TampilJamKecil() {

  static uint32_t pM;
  static uint32_t pMJam;
  uint32_t cM = millis();
  static boolean kedip;
  
  
  char jam[3];
  char menit[3];
  //char detik[3];
  
  if (cM - pMJam >= 1000) {
   
    pMJam = cM;
    
    //JAM
    //sprintf(jam,"%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
    sprintf(jam,"%02d:%02d", timeClient.getHours(), timeClient.getMinutes());
    Disp.setFont(ElektronMart5x6);
    textCenter(0,jam);
        
  }
 
}



//----------------------------------------------------------------------
//TAMPILKAN TANGGAL

void TampilTanggal() {

  
  static uint8_t d;
  static char hari[8];
  static char tanggal[18];

  static uint32_t pM;
  uint32_t cM = millis();

  if (cM - pM > 2000) {
    pM = cM;
    d++;
  
    sprintf(hari, "%s", weekDay[timeClient.getDay()]);
    sprintf(tanggal, "%02d %s", timeClient.getDate(), monthYear[timeClient.getMonth()]);  
    
    Disp.setFont(ElektronMart5x6);
    textCenter(0,hari);;
    textCenter(8,tanggal);

    if (d >= 2) {
      d = 0;
      Disp.clear();
      tampilanjam = 4;
    }
  } 
  
}

void TampilJamTanggal(){
  static uint32_t pM;
  static uint32_t x;
  static uint32_t Speed = 50;
  static char buff[50];
  int width = Disp.width();
  sprintf(buff, "%s, %02d %s %04d",weekDay[timeClient.getDay()], timeClient.getDate(), monthYear[timeClient.getMonth()],timeClient.getYear());
  Disp.setFont(ElektronMart6x8);
  int fullScroll = Disp.textWidth(buff) + width;
  if((millis() - pM) > Speed) { 
    pM = millis();
    if (x < fullScroll) {
      ++x;
    } else {
      x = 0;
      tampilanjam = 5;
      return;
    }
    Disp.drawText(width - x, 4, buff);
  }  
}

//----------------------------------------------------------------------
// TAMPILKAN SUHU

void TampilSuhu(){
  //Tampilkan Suhu
//  RtcTemperature temp = Rtc.GetTemperature();
  int celsius = random(10,30);
  char suhu[2];
  int koreksisuhu = 2; // Perkiraan selisih suhu ruangan dan luar ruangan

  static uint8_t d;
  static uint32_t pM;
  uint32_t cM = millis();

  if (cM - pM > 2000) {
    pM = cM;
    d++;
  
    Disp.setFont(ElektronMart5x6);    
    textCenter(0, "SUHU");
    sprintf(suhu,"%dC*",celsius - koreksisuhu);
    Disp.setFont(ElektronMart6x8);
    textCenter(8, suhu);

    if (d >= 2) {
      d = 0;
      Disp.clear();
      tampilanjam = 5;
    }  

  } 
}



//----------------------------------------------------------------------
// PARAMETER PENGHITUNGAN JADWAL SHOLAT

void JadwalSholat() {

  

  int tahun = timeClient.getYear();
  int bulan = timeClient.getMonth();
  int tanggal = timeClient.getDate();
  
  set_calc_method(Karachi);
  set_asr_method(Shafii);
  set_high_lats_adjust_method(AngleBased);
  set_fajr_angle(20);
  set_isha_angle(18);  

  get_prayer_times(tahun, bulan, tanggal, config.latitude, config.longitude, config.zonawaktu, times);

}



//----------------------------------------------------------------------
// TAMPILAN JADWAL SHOLAT

void TampilJadwalSholat() {
  
  JadwalSholat();

  static uint8_t i;
  static uint32_t pM;
  uint32_t cM = millis();
  
  char sholat[7];
  char jam[5];
  char TimeName[][8] = {"SUBUH","TERBIT","DZUHUR","ASHAR","TRBNM","MAGHRIB"," ISYA'"};
  int hours, minutes;    
  
  if (cM - pM >= 2000) {
    
    pM = cM;    
    Disp.clear();

    if (i == 1) {i = 2;} // Abaikan Terbit
    if (i == 4) {i = 5;} // Abaikan Terbenam

    get_float_time_parts(times[i], hours, minutes);
  
    minutes = minutes + config.ihti;
    
    if (minutes >= 60) {
      minutes = minutes - 60;
      hours ++;
    }
    
    String sholat = TimeName[i];
    sprintf(jam,"%02d:%02d", hours, minutes);     
    
    Disp.setFont(ElektronMart5x6);
    textCenter(0,sholat);
    Disp.setFont(ElektronMart6x8);
    textCenter(8,jam);
    
    i++;    
    
    if (i > 7) {
      get_float_time_parts(times[0], hours, minutes);
      minutes = minutes + config.ihti;
      if (minutes < 11) {
        minutes = 60 - minutes;
        hours --;
      } else {
        minutes = minutes - 10 ;
      }
      sprintf(jam,"%02d:%02d", hours, minutes);
      Disp.clear();
      Disp.setFont(ElektronMart5x6);
      textCenter(0,"TANBIH");
      Disp.setFont(ElektronMart6x8);
      textCenter(8,jam);
      
      if (i > 8) {
        i = 0;
        Disp.clear();
        tampilanjam = 6;
      }
      
    }  
     
  }
    
}



//----------------------------------------------------------------------
// ALARM SHOLAT BERJALAN SAAT MASUK WAKTU SHOLAT

void AlarmSholat() {

  

  int Hari = timeClient.getDay();
  int Hor = timeClient.getHours();
  int Min = timeClient.getMinutes();
  int Sec = timeClient.getSeconds();
  int adzan = config.durasiadzan * 60000;

  JadwalSholat();
  int hours, minutes, seconds;

  // Tanbih Imsak
  get_float_time_parts(times[0], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes < 10) {
    minutes = 60 - minutes;
    hours --;
  } else {
    minutes = minutes - 10 ;
  }

  if (Hor == hours && Min == minutes) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "TANBIH");
    textCenter(8, "IMSAK");
    delay(adzan);
    Disp.clear();

  }

  // Subuh
  get_float_time_parts(times[0], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes >= 60) {
    minutes = minutes - 60;
    hours ++;
  }

  if (Hor == hours && Min == minutes) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "SUBUH");
    delay(adzan);
    Disp.clear();
    iqmh = config.iqmhs;
    menitiqmh = iqmh - 1;
    tampilanutama = 1;
  }

  // Dzuhur
  get_float_time_parts(times[2], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes >= 60) {
    minutes = minutes - 60;
    hours ++;
  }

  if (Hor == hours && Min == minutes && Hari != 5) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "DZUHUR");
    delay(adzan);
    Disp.clear();
    iqmh = config.iqmhd;
    menitiqmh = iqmh - 1;
    tampilanutama = 1;

  } else if (Hor == hours && Min == minutes && Hari == 5) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "JUM'AT");
    delay(adzan);

  }

  // Ashar
  get_float_time_parts(times[3], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes >= 60) {
    minutes = minutes - 60;
    hours ++;
  }

  if (Hor == hours && Min == minutes) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "ASHAR");
    delay(adzan);
    Disp.clear();
    iqmh = config.iqmha;
    menitiqmh = iqmh - 1;
    tampilanutama = 1;
  }

  // Maghrib
  get_float_time_parts(times[5], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes >= 60) {
    minutes = minutes - 60;
    hours ++;
  }

  if (Hor == hours && Min == minutes) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "MAGHRIB");
    delay(adzan);
    Disp.clear();
    iqmh = config.iqmhm;
    menitiqmh = iqmh - 1;
    tampilanutama = 1;
  }

  // Isya'
  get_float_time_parts(times[6], hours, minutes);
  minutes = minutes + config.ihti;

  if (minutes >= 60) {
    minutes = minutes - 60;
    hours ++;
  }

  if (Hor == hours && Min == minutes) {
    BuzzerPendek();
    Disp.clear();
    Disp.setFont(ElektronMart5x6);
    textCenter(0, "ADZAN");
    textCenter(8, "ISYA'");
    delay(adzan);
    Disp.clear();
    iqmh = config.iqmhi;
    menitiqmh = iqmh - 1;
    tampilanutama = 1;
  }

}


//----------------------------------------------------------------------
// HITUNG MUNDUR WAKTU SETELAH ADZAN SAMPAI MULAI IQOMAH

void Iqomah() {  

  static uint32_t pMIqmh;
  uint32_t cM = millis();
  static char hitungmundur[6];

  Disp.setFont(ElektronMart5x6);
  textCenter(0, "IQOMAH");

  if (detikiqmh == 60) {
    detikiqmh = 0;
  }  

  if(cM - pMIqmh >= 1000) {
    
    pMIqmh = cM;    
    detikiqmh--;
    
    if (menitiqmh == 0 && detikiqmh == 0){
        Disp.clear();
        textCenter(0, "LURUS 8");
        textCenter(9, "RAPAT");
        BuzzerPanjang();
        detikiqmh = 59;
        Disp.clear();
        tampilanutama = 0;
    }

    if (detikiqmh < 0) {
      detikiqmh = 59;
      menitiqmh--;            
    }
    
  }

  sprintf(hitungmundur, "%02d:%02d", menitiqmh, detikiqmh);
  Disp.setFont(ElektronMart6x8);
  textCenter(8, hitungmundur);  
  
}



//----------------------------------------------------------------------
// BUNYIKAN BEEP BUZZER

void BuzzerPanjang() {
  
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
  delay(1000);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
  delay(50);
  
}

void BuzzerPendek() {
  
  digitalWrite(buzzer, HIGH);
  delay(200);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(200);
  digitalWrite(buzzer, LOW);
  delay(50);
  
}



//-------------------------------------------------------
// TAMPILKAN SCROLLING TEKS NAMA

static char *nama[] = {config.nama};

void TeksJalanNama() {

  static uint32_t pM;
  static uint32_t x;
  static uint32_t Speed = 50;
  int width = Disp.width();
  Disp.setFont(ElektronMart6x8);
  int fullScroll = Disp.textWidth(nama[0]) + width;
  if((millis() - pM) > Speed) { 
    pM = millis();
    if (x < fullScroll) {
      ++x;
    } else {
      x = 0;
      tampilanjam = 7;
      return;
    }
    Disp.drawText(width - x, 4, nama[0]);
  }  

}



//-------------------------------------------------------
// TAMPILKAN SCROLLING TEKS INFO1

static char *info1[] = {config.info1};

void TeksJalanInfo1() {

  TampilJamKecil();
  
  static uint32_t pM;
  static uint32_t x;
  static uint32_t Speed = 50;
  int width = Disp.width();
  Disp.setFont(ElektronMart6x8);
  int fullScroll = Disp.textWidth(info1[0]) + width;
  if((millis() - pM) > Speed) { 
    pM = millis();
    if (x < fullScroll) {
      ++x;
    } else {
      x = 0;
//      Disp.clear();
      tampilanjam = 8;
      return;
    }
    Disp.drawText(width - x, 8, info1[0]);
  }  

}



//-------------------------------------------------------
// TAMPILKAN SCROLLING TEKS INFO2

static char *info2[] = {config.info2};

void TeksJalanInfo2() {

  TampilJamKecil();

  static uint32_t pM;
  static uint32_t x;
  static uint32_t Speed = 50;
  int width = Disp.width();
  Disp.setFont(ElektronMart6x8);
  int fullScroll = Disp.textWidth(info2[0]) + width;
  if((millis() - pM) > Speed) { 
    pM = millis();
    if (x < fullScroll) {
      ++x;
    } else {
      x = 0;
      Disp.clear();
      tampilanjam = 0;
      return;
    }
    Disp.drawText(width - x, 8, info2[0]);
  }

}



//----------------------------------------------------------------------
// FORMAT TEKS

void textCenter(int y,String Msg) {
  
  int center = int((Disp.width()-Disp.textWidth(Msg)) / 2);
  Disp.drawText(center,y,Msg);
  
}



//----------------------------------------------------------------------
// ANIMASI LOGO

void animLogoX() {

  static uint8_t x;
  static uint8_t s; // 0=in, 1=out
  static uint32_t pM;
  uint32_t cM = millis();

  if ((cM - pM) > 35) {
    if (s == 0 and x < 16) {
      pM = cM;
      x++;
    }
    if (s == 1 and x > 0) {
      pM = cM;
      x--;
    }
  }
  
  if ((cM - pM) > 5000 and x == 16) {
    s = 1;
  }

  if (x == 0 and s == 1) {    
    s = 0;
    tampilanjam = 1;
  }

  logoax(x - 16);
  logobx(32 - x);

}


void logoax(uint32_t x) {
  static const uint8_t logoax[] PROGMEM = {
    16, 16,
    B00000000,B00000000,
    B01100110,B01111110,
    B01100110,B01111110,
    B01100110,B01100110,
    B01100110,B01100110,
    B01111110,B01111110,
    B01111110,B01111110,
    B01100000,B01100000,
    B01100000,B01100000,
    B01111110,B01111110,
    B01111110,B01111110,
    B01100110,B00000110,
    B01100110,B00000110,
    B01111111,B11111110,
    B01111111,B11111110,
    B00000000,B00000000
  };
  Disp.drawBitmap(x, 0, logoax);
}

void logobx(uint32_t x) {
  static const uint8_t logobx[] PROGMEM = {
    16, 16,
    B00000000,B00000000,
    B01111111,B11111110,
    B01111111,B11111110,
    B00000000,B00000000,
    B00000000,B00000000,
    B01111110,B01100110,
    B01111110,B01100110,
    B00000110,B01100110,
    B00000110,B01100110,
    B01111110,B01100110,
    B01111110,B01100110,
    B01100110,B01100110,
    B01100110,B01100110,
    B01111111,B11111110,
    B01111111,B11111110,
    B00000000,B00000000
  };
  Disp.drawBitmap(x, 0, logobx);
}



//----------------------------------------------------------------------
// TOGGLE LED INTERNAL UNTUK STATUS MODE WIFI

void toggleLED() {

  digitalWrite(pin_led, !digitalRead(pin_led));
  server.send_P(200, "text/html", setwaktu);

}



//----------------------------------------------------------------------
// HJS589 P10 utility Function






