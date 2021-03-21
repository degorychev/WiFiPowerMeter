#include <WiFiManager.h>
String Hostname;
boolean debug = true;
bool SendToNarodmon(float volts, float watts) { // Собственно формирование пакета и отправка.
  WiFiClient client;
  String buf;
  buf = "#" + Hostname + "\r\n"; // заголовок

  buf = buf + "#1";
  buf = buf + "#" + String(volts) + "\r\n"; //и температура
  buf = buf + "#2";
  buf = buf + "#" + String(watts) + "\r\n"; //и температура

  buf = buf + "##\r\n"; // закрываем пакет

  if (!client.connect("narodmon.ru", 8283)) { // попытка подключения
    Serial.println("connection failed");
    return false; // не удалось;
  } else
  {
    client.print(buf); // и отправляем данные
    if (debug) Serial.print(buf);
    while (client.available()) {
      String line = client.readStringUntil('\r'); // если что-то в ответ будет - все в Serial
      Serial.print(line);
    }
  }
  return true; //ушло
}
class AvgValue {
  public:
    AvgValue() {
      totalValue = 0;
      count = 0;
    }
    void AddValue(float value) {
      totalValue += value;
      count++;
    }
    float GetValue() {
      return totalValue / count;
    }
  private:
    float totalValue;
    int count;
};
