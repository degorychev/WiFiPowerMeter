void ICACHE_RAM_ATTR ISR();
#include <ESP8266WiFi.h>
#define CONFIG_BUTTON D6
const int CLKPin = D2; // Pin connected to CLK (D2 & INT0)
const int MISOPin = D5;  // Pin connected to MISO (D5)

const char* ssid     = "VEHomeLan2G";
const char* password = "qwertyui";
WiFiServer server(80);

PowerMeter PMobj(CLKPin, MISOPin);
void setup() {
  Serial.begin(115200); 
  Serial.println("Loading");
  delay(1000);
  Serial.println("1s");
  delay(1000);
  Serial.println("2s");
  delay(1000);
  Serial.println("3s");
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
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
  if(PMobj.tick())
    Serial.println(PMobj.GetStatus());  
  if (digitalRead(CONFIG_BUTTON) == LOW) 
    Serial.println("Кнопка");  
  webrequest();
  
}
