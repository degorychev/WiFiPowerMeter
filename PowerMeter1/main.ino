void ICACHE_RAM_ATTR ISR();
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#define postingInterval  300000  
#define CONFIG_BUTTON D6
const int CLKPin = D2; // Pin connected to CLK (D2 & INT0)
const int MISOPin = D5;  // Pin connected to MISO (D5)

const char* ssid     = "VEHomeLan2G";
const char* password = "qwertyui";
WiFiServer server(80);
unsigned long lastConnectionTime = 0;

PowerMeter PMobj(CLKPin, MISOPin);
AvgValue avgU = AvgValue();//Среднее напряжение
AvgValue avgP = AvgValue();//Средняя мощность

void setup() {
  Serial.begin(115200); 
  Serial.println("Loading");
  delay(1000);
  Serial.println("1s");
  delay(1000);
  Serial.println("2s");
  delay(1000);
  Serial.println("3s");

  
  Hostname = "ESP"+WiFi.macAddress();
  Hostname.replace(":","");
  WiFi.hostname(Hostname);
  Serial.println(WiFi.localIP()); Serial.println(WiFi.macAddress()); Serial.print("Narodmon ID: "); Serial.println(Hostname);
  lastConnectionTime = millis() - postingInterval + 15000; //первая передача на народный мониторинг через 15 сек.

  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    if (++counter > 100) {
      Serial.print("rebooting...");
      ESP.restart();
    }
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  if (digitalRead(CONFIG_BUTTON) == LOW) {
    Serial.println("config button pressed");  
  }
  Serial.println("Конфиг после перевода на ООП");
  
  attachInterrupt(digitalPinToInterrupt(CLKPin), ISR, RISING);
  pinMode(CLKPin, INPUT);
  pinMode(MISOPin, INPUT);
}

void ISR(){
  PMobj.CLK_ISR();
}

void loop() {
  if(PMobj.tick())//Если пришли новые данные с ваттметра
  {
    avgU.AddValue(PMobj.GetVolts());
    avgP.AddValue(PMobj.GetWatts());
    Serial.println(PMobj.GetStatus());  
  }
  yield();
  if (digitalRead(CONFIG_BUTTON) == LOW) 
    Serial.println("Кнопка");  
  webrequest();
  if (millis() - lastConnectionTime > postingInterval) { // ждем 5 минут и отправляем
    if (WiFi.status() == WL_CONNECTED) { // ну конечно если подключены
      if (SendToNarodmon(avgU.GetValue(), avgP.GetValue())) {
        Serial.println("Данные ушли V:"+String(avgU.GetValue())+" P:"+String(avgP.GetValue()));          
        avgU = AvgValue();
        avgP = AvgValue();//Обнуляем среднее чтобы получить более резкий график
        
        lastConnectionTime = millis();
      }else{  lastConnectionTime = millis() - postingInterval + 15000; }//следующая попытка через 15 сек    
    }else{  lastConnectionTime = millis() - postingInterval + 15000; Serial.println("Not connected to WiFi");}//следующая попытка через 15 сек
  }  
  yield();
}
