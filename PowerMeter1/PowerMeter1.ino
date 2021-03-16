class PowerMeter
{
  public:
    PowerMeter(int CLKPin = D2, int MISOPin = D5);
    void CLK_ISR();
    boolean tick();
    String GetStatus();
    float GetVolts();
    float GetWatts();

  private:
    void doInSync();
    void updateTallys(float volts, float watts);

    int _CLKPin;
    int _MISOPin;

    //All variables that are changed in the interrupt function must be volatile to make sure changes are saved.
    volatile int Ba = 0;   //Store MISO-byte 1
    volatile int Bb = 0;   //Store MISO-byte 2
    volatile int Bc = 0;   //Store MISO-byte 2
    float U = 0;    //voltage
    float P = 0;    //power

    volatile long CountBits = 0;      //Count read bits
    volatile long ClkHighCount = 0;   //Number of CLK-highs (find start of a Byte)
    volatile boolean inSync = false;  //as long as we ar in SPI-sync
    volatile boolean NextBit = true;  //A new bit is detected
    boolean event = false;
    volatile unsigned int isrTriggers; // for debugging to see if ISR routine is being called
    float volts, watts;
};

PowerMeter::PowerMeter(int CLKPin, int MISOPin) {
  _CLKPin = CLKPin;
  _MISOPin = MISOPin;
}
boolean PowerMeter::tick() {
  if (inSync == true) {
    doInSync();
  }
  return event;
}
float PowerMeter::GetVolts() {
  return volts;
}
float PowerMeter::GetWatts() {
  return watts;
}
String PowerMeter::GetStatus() {
  event = false;
  return "Volts: " + String(volts) + " " + "Watts: " + watts;
}
void PowerMeter::CLK_ISR() {
  isrTriggers += 1;
  //если мы пытаемся найти время синхронизации (CLK становится высоким на 1-2 мс)
  if (inSync == false) {
    ClkHighCount = 0;
    //Зарегистрируйте, как долго ClkHigh высокий, чтобы оценить, находимся ли мы в той части, где clk повышается на 1-2 мс
    while (digitalRead(_CLKPin) == HIGH) {
      ClkHighCount += 1;
      delayMicroseconds(30);  // в прерывании можно использовать только delayMicroseconds.
    }
    //если Clk был высоким между 1 и 2 мс, это начало передачи SPI
    if (ClkHighCount >= 33 && ClkHighCount <= 67) {
      inSync = true;
    }
  }
  else { // мы синхронизируемся и регистрируем максимумы CLK
    // увеличиваем целое число, чтобы отслеживать, сколько бит мы прочитали.
    CountBits += 1;
    NextBit = true;
  }
}

void PowerMeter::doInSync() {
  CountBits = 0; // CLK-прерывание увеличивает CountBits при получении нового бита (Функция выше)
  while (CountBits < 40) {} //пропустить неинтересные 5 первых байтов
  CountBits = 0;
  Ba = 0;
  Bb = 0;
  while (CountBits < 24) { //Прокрутите следующие 3 байта (6-8) и сохраните байты 6 и 7 в Ba и Bb
    if (NextBit == true) { //при обнаружении нарастающего фронта на CLK NextBit = true в прерывании.
      if (CountBits < 9) { //первый байт / 8 бит в Ba
        Ba = (Ba << 1);  //Сдвиньте Ba на один бит влево и сохраните значение MISO (0 или 1) (см. http://arduino.cc/en/Reference/Bitshift)
        //прочитать MISO-контакт, если высокий: сделать Ba[0] = 1
        if (digitalRead(_MISOPin) == HIGH) {
          Ba |= (1 << 0); //изменяет первый бит Ba на "1"
        }   //не требует "else", потому что BaBb [0] равен нулю, если не изменен.
        NextBit = false; //сбросить NextBit в ожидании следующего прерывания CLK
      }
      else if (CountBits < 17) { //бит 9-16 - это байт 7, хранить в Bb
        Bb = Bb << 1;  //Сдвиньте Ba на один бит влево и сохраните значение MISO (0 или 1)
        //прочитать MISO-контакт, если высокий: сделать Bb[0] = 1
        if (digitalRead(_MISOPin) == HIGH) {
          Bb |= (1 << 0); //изменяет первый бит Bb на "1"
        }
        NextBit = false; //сбросить NextBit в ожидании следующего прерывания CLK
      }
    }
  }
  if (Bb != 3) { //если бит Bb не равен 3, мы достигли важной части, U уже находится в Ba и Bb, и следующие 8 байтов дадут нам мощность.

    //Voltage = 2*(Ba+Bb/255)
    U = 2.0 * ((float)Ba + (float)Bb / 255.0);

    //Power:
    CountBits = 0;
    while (CountBits < 40) {} //Начните читать следующие 8 байтов, пропуская первые 5 неинтересных

    CountBits = 0;
    Ba = 0;
    Bb = 0;
    Bc = 0;
    while (CountBits < 24) { //хранить байты 6, 7 и 8 в Ba и Bb & Bc
      if (NextBit == true) {
        if (CountBits < 9) {
          Ba = (Ba << 1);  //Сдвиньте Ba на один бит влево и сохраните значение MISO (0 или 1)
          //прочитать MISO-контакт, если высокий: сделать Ba [0] = 1
          if (digitalRead(_MISOPin) == HIGH) {
            Ba |= (1 << 0); //изменяет первый бит Ba на "1"
          }
          NextBit = false;
        }
        else if (CountBits < 17) {
          Bb = Bb << 1;  //Сдвиньте Ba на один бит влево и сохраните значение MISO (0 или 1)
          //read MISO-pin, if high: make Ba[0] = 1
          if (digitalRead(_MISOPin) == HIGH) {
            Bb |= (1 << 0); //изменяет первый бит Bb на "1"
          }
          NextBit = false;
        }
        else {
          Bc = Bc << 1;  //Сдвиньте Bc на один бит влево и сохраните значение MISO (0 или 1)
          //read MISO-pin, if high: make Bc[0] = 1
          if (digitalRead(_MISOPin) == HIGH) {
            Bc |= (1 << 0); //изменяет первый бит Bc на "1"
          }
          NextBit = false;
        }
      }

    }

    //Power = (Ba*255+Bb)/2
    P = ((float)Ba * 255 + (float)Bb + (float)Bc / 255.0) / 2;

    if (U > 200 && U < 300 && P >= 0 && P < 4000) { // игнорировать ложные показания с напряжением или мощностью вне нормального диапазона
      //updateTallys(U, P);
      volts = U;
      watts = P;
      event = true;
    }
    inSync = false; //сбросить переменную синхронизации, чтобы обеспечить синхронизацию следующего чтения.
  }

  if (Bb == 0) { //Если Bb не 3 или что-то еще, кроме 0, что-то не так!
    inSync = false;
    Serial.println("Nothing connected, or out of sync!");
  }
}
