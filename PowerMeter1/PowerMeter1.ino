class PowerMeter
{
  public:
    PowerMeter(int CLKPin = D2, int MISOPin = D5);
    void CLK_ISR();
    void tick();
  private:
    void doInSync();
    void clearTallys();
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
    
    volatile unsigned int isrTriggers; // for debugging to see if ISR routine is being called
    
    float avgVolts, minVolts, maxVolts;
    float avgWatts, minWatts, maxWatts;
    int numReadings;
};

PowerMeter::PowerMeter(int CLKPin, int MISOPin){
  _CLKPin = CLKPin;
  _MISOPin = MISOPin;
  clearTallys();
}
void PowerMeter::tick(){
  if (inSync == true) {
    doInSync();
  } 
}
void PowerMeter::clearTallys() {
  numReadings = 0;
  minVolts = 9999;
  maxVolts = -9999;
  minWatts = 9999;
  maxWatts = -9999;
}
void PowerMeter::updateTallys(float volts, float watts) {

  avgVolts = (volts + (numReadings * avgVolts)) / (numReadings + 1);
  avgWatts = (watts + (numReadings * avgWatts)) / (numReadings + 1);

  if (volts < minVolts) minVolts = volts;
  if (volts > maxVolts) maxVolts = volts;
  if (watts < minWatts) minWatts = watts;
  if (watts > maxWatts) maxWatts = watts;

  numReadings += 1;

  //Serial.print("Readings="); Serial.println(numReadings);
  Serial.print("Volts: "); Serial.print(volts); Serial.print(" ");
  //Serial.print(" avg="); Serial.print(avgVolts); Serial.print(" min="); Serial.print(minVolts); Serial.print(" max="); Serial.println(maxVolts);
  Serial.print("Watts: "); Serial.println(watts); 
  //Serial.print(" avg="); Serial.print(avgWatts); Serial.print(" min="); Serial.print(minWatts); Serial.print(" max="); Serial.println(maxWatts);
}
void PowerMeter::CLK_ISR() {
  isrTriggers += 1;
  //if we are trying to find the sync-time (CLK goes high for 1-2ms)
  if (inSync == false) {
    ClkHighCount = 0;
    //Register how long the ClkHigh is high to evaluate if we are at the part wher clk goes high for 1-2 ms
    while (digitalRead(_CLKPin) == HIGH) {
      ClkHighCount += 1;
      delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
    }
    //if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
    if (ClkHighCount >= 33 && ClkHighCount <= 67) {
      inSync = true;
    }
  }
  else { //we are in sync and logging CLK-highs
    //increment an integer to keep track of how many bits we have read.
    CountBits += 1;
    NextBit = true;
  }
}

void PowerMeter::doInSync() {
    CountBits = 0;  //CLK-interrupt increments CountBits when new bit is received
    while (CountBits < 40) {} //skip the uninteresting 5 first bytes
    CountBits = 0;
    Ba = 0;
    Bb = 0;
    while (CountBits < 24) { //Loop through the next 3 Bytes (6-8) and save byte 6 and 7 in Ba and Bb
      if (NextBit == true) { //when rising edge on CLK is detected, NextBit = true in in interrupt.
        if (CountBits < 9) { //first Byte/8 bits in Ba
          Ba = (Ba << 1);  //Shift Ba one bit to left and store MISO-value (0 or 1) (see http://arduino.cc/en/Reference/Bitshift)
          //read MISO-pin, if high: make Ba[0] = 1
          if (digitalRead(_MISOPin) == HIGH) {
            Ba |= (1 << 0); //changes first bit of Ba to "1"
          }   //doesn't need "else" because BaBb[0] is zero if not changed.
          NextBit = false; //reset NextBit in wait for next CLK-interrupt
        }
        else if (CountBits < 17) { //bit 9-16 is byte 7, stor in Bb
          Bb = Bb << 1;  //Shift Ba one bit to left and store MISO-value (0 or 1)
          //read MISO-pin, if high: make Ba[0] = 1
          if (digitalRead(_MISOPin) == HIGH) {
            Bb |= (1 << 0); //changes first bit of Bb to "1"
          }
          NextBit = false; //reset NextBit in wait for next CLK-interrupt
        }
      }
    }
    if (Bb != 3) { //if bit Bb is not 3, we have reached the important part, U is allready in Ba and Bb and next 8 Bytes will give us the Power.

      //Voltage = 2*(Ba+Bb/255)
      U = 2.0 * ((float)Ba + (float)Bb / 255.0);

      //Power:
      CountBits = 0;
      while (CountBits < 40) {} //Start reading the next 8 Bytes by skipping the first 5 uninteresting ones

      CountBits = 0;
      Ba = 0;
      Bb = 0;
      Bc = 0;
      while (CountBits < 24) { //store byte 6, 7 and 8 in Ba and Bb & Bc.
        if (NextBit == true) {
          if (CountBits < 9) {
            Ba = (Ba << 1);  //Shift Ba one bit to left and store MISO-value (0 or 1)
            //read MISO-pin, if high: make Ba[0] = 1
            if (digitalRead(_MISOPin) == HIGH) {
              Ba |= (1 << 0); //changes first bit of Ba to "1"
            }
            NextBit = false;
          }
          else if (CountBits < 17) {
            Bb = Bb << 1;  //Shift Ba one bit to left and store MISO-value (0 or 1)
            //read MISO-pin, if high: make Ba[0] = 1
            if (digitalRead(_MISOPin) == HIGH) {
              Bb |= (1 << 0); //changes first bit of Bb to "1"
            }
            NextBit = false;
          }
          else {
            Bc = Bc << 1;  //Shift Bc one bit to left and store MISO-value (0 or 1)
            //read MISO-pin, if high: make Bc[0] = 1
            if (digitalRead(_MISOPin) == HIGH) {
              Bc |= (1 << 0); //changes first bit of Bc to "1"
            }
            NextBit = false;
          }
        }

      }

      //Power = (Ba*255+Bb)/2
      P = ((float)Ba * 255 + (float)Bb + (float)Bc / 255.0) / 2;

      if (U > 200 && U < 300 && P >= 0 && P < 4000) { // ignore spurious readings with voltage or power out of normal range
         updateTallys(U, P);
      } else {
        Serial.print(".");
      }

      inSync = false; //reset sync variable to make sure next reading is in sync.
    }

    if (Bb == 0) { //If Bb is not 3 or something else than 0, something is wrong!
      inSync = false;
      Serial.println("Nothing connected, or out of sync!");
    }
}
