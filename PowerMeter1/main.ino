void ICACHE_RAM_ATTR ISR();
#include <ESP8266WiFi.h>
#define CONFIG_BUTTON D6
const int CLKPin = D2; // Pin connected to CLK (D2 & INT0)
const int MISOPin = D5;  // Pin connected to MISO (D5)

PowerMeter PMobj(CLKPin, MISOPin);
void setup() {
  Serial.begin(115200); Serial.println();
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
  if (digitalRead(CONFIG_BUTTON) == LOW) {
    Serial.println("Кнопка");  
  }
}
