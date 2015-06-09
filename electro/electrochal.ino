/*
 iHack 2015 electronic challenge

 Several flags can be found by understanding the circuit, abusing of it and 
 *possibly* exploiting race conditions.

 Author: Martin Dubé <martin.dube ]at[ hackfest.ca>
 License: Modified BSD License
 Organisation: Hackfest Communication

 The circuit
 -----------

 LCD: 
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Bit Shifter (74HC595):
 * Vcc pin to +5V
 * SER pin to pin 8
 * OE pin to ground
 * RCLK pin to pin 9
 * SRCLK pin to pin 10
 * SRCLR pin to +5V
 * Qb pin to LED1
 * Qd pin to LED2
 * Qf pin to LED3
 * Qh pin to LED4

 Button 1:

 Button 2: 

 Potentiometer:

 LED 1 à 4:

 LM35 (temperature sensor):

 IR Sensor: 

 Photocell sensor:

*/

// include the library code:
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <IRremote.h>

// Timer1
#include <TimerOne.h>

OneWire  ds(13);  // on pin 10 (a 4.7K resistor is necessary)

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// All devices pins
const int potPin = 0;
const int lightPin = 1; 
const int buttonPin = 7;    // the number of the pushbutton pin
const int ledPin = 13;      // the number of the LED pin
const int irPin = 6; //set D13 as input signal pin

volatile unsigned int clockCount = 0;
volatile unsigned int flagClockCount = 0;

// IR Remote
IRrecv irrecv(irPin);
decode_results signals;

//How many of the shift registers - change this
#define number_of_74hc595s 1 
#define numOfRegisterPins number_of_74hc595s * 8
boolean registers[numOfRegisterPins];

// Variables will change:
int clockLedState = HIGH;         // the current state of the output pin
int flagLedState = LOW;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

int timer1_counter;

// The interrupt will blink the LED, and keep
// track of how many times it has blinked.
//int clockLedState = LOW;
volatile unsigned long blinkCount = 0; // use volatile for shared variable

const int SER_Pin = 8;   //pin 14 on the 75HC595
const int RCLK_Pin = 9;  //pin 12 on the 75HC595
const int SRCLK_Pin = 10; //pin 11 on the 75HC595

int LED_CLOCK = 1;
int LCD_BTN = 3;
int LED_FLAG = 5;
int LED_IR = 7;

// All devices value
volatile unsigned int potValue = 0; // use volatile for shared variable
volatile unsigned int tempValue = 0; // use volatile for shared variable
volatile unsigned int lightValue = 0; // use volatile for shared variable
volatile unsigned long irValue = 0; // use volatile for shared variable
volatile char flagValue[16];

// Flags (max 11 for LCD)
const char FLAG_1[] = "d1g1t4ll0ck";  // Lock Challenge
const char FLAG_2[] = "und3rc0v3rz";  // IR Challenge
const char FLAG_3[] = "chr1stm4str";  // Turn all lights on
const char FLAG_4[] = "r4c3abcc0nd";  // Race condition Challenge
const char FLAG_5[] = "aas3r14lbbc";  // Serial Challenge
const char FLAG_6[] = "f1r3burnw00";  // Temperature sensor (bonus)

// 
char* LCD_FMT_STR[] = { "Electro Chal", 
                         "Pot: %i",
                         "Temp: %i C", 
                         "Light: %i%",
                         "IR: %lX", 
                         "Flag:%s"};
int LCD_MSG_INDEX = 0;

int LOCK_INDEX_MODE = 0;
int CHAL2_INDEX_MODE = 0;

int LOCK_CHAL_COMBIN[][2] = {{0,20}, {20,40}, {40,60}};
int LOCK_CHAL_INDEX;                // A number from 0 to 2
int LOCK_CHAL_USER_INPUT[3];        // Saved attempts by a user

// TO BE REMOVED: 0 + 6 + 2 + 5 + 9 + 6 + Menu/Info/Status (and keep pressed)
long IR_CHAL_VALUES[] = {0xFF6897, 0xFF5AA5, 0xFF18E7, 0xFFFFFFFF};
int IR_CHAL_INDEX;                 // A number from 0 to 7
long IR_CHAL_USER_INPUT[4];        // Saved attempts by a user

int RC_CHAL_USER_INPUT[][2] = {{0,0},{0,0},{0,0},{0,0},{0,0}};
int RC_CHAL_INDEX;                 // A number from 0 to 4

void setup(void) {
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.setCursor(0,0);
  //lcd.clear();
  lcd.print("iHack 2015");
  lcd.setCursor(0,1);
  lcd.print("Electro Chal");

  // IR Remote
  irrecv.enableIRIn(); // enable input from IR receiver

  // LED Shifter
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, LOW);

  //reset all register pins
  clearRegisters();
  writeRegisters();

  // Switch button
  pinMode(buttonPin, INPUT);

  Timer1.initialize(500000);
  Timer1.attachInterrupt(callback,500000); // callback every 1 sec.
}

void callback(void)
{
  if (clockCount % 4 == 0){
    Serial.print("Clock count: ");
    Serial.println(clockCount);
  }
  
  updateDevicesValue();
  
  // Blink the clock light for lock challenge
  if (clockCount % 4 == 0 and clockLedState == LOW) {
    clockLedState = HIGH;
    blinkCount = blinkCount + 1;  // increase when LED turns on
  } else {
    clockLedState = LOW;
  }
  setRegisterPin(LED_CLOCK, clockLedState);

  // Validate lock challenge
  if (clockCount % 4 == 0){
    updateLockChallenge();
    if (lockIsValid()) {
      printLockFlag();
      flagClockCount = clockCount;
    }
  }  
  
  // Validate IR challenge
  if (clockCount % 4 == 0){
    updateIRChallenge();
    if (irIsValid()) {
      printIRFlag();
      flagClockCount = clockCount;
    }
  }  
  
  // Validate Christmas Tree challengema
  if (christmasTreeIsValid()){
    printChristmasTreeFlag();
    flagClockCount = clockCount;
  }
  
  // Validate RC challenge
  if (clockCount % 4 == 0){
    updateRCChallenge();
    if (rcIsValid()) {
      printRCFlag();
      flagClockCount = clockCount;
    }
  }  
  
  // Validate temp challenge
  if (clockCount % 4 == 0){
    if (tempValue > 60){
      printTempFlag();
      flagClockCount = clockCount;
    } 
  }
  
  updateLCD(); 
  
  clockCount = clockCount + 1;

  
  // Reset flag after 15 seconds
  if (flagClockCount != 0 && flagClockCount + 30 < clockCount){ 
    Serial.println("Removing flag from LCD");
    setRegisterPin(LED_FLAG, LOW);
    flagValue[0] = '\0';
    flagClockCount = 0;
  }
  
  //Serial flag
  if (clockCount % 60 == 0){
     Serial.print("Flag: ");
     Serial.println(FLAG_5);
  }
}

void updateDevicesValue(void){
  int itmp = 0;
  unsigned long ltmp = 0;
  potValue = analogRead(potPin);
  lightValue = analogRead(lightPin);
  
  if (clockCount % 2 == 0){
    itmp = getTemp();   
    if (itmp != 0){
      tempValue = itmp; 
    }
  }   

  ltmp = getIR();
  if (ltmp != 0){
    irValue = ltmp;
  }
}

void updateLockChallenge(void){
  int i;
  int val = map(potValue, 0, 1023, 0, 100);
  
  LOCK_CHAL_USER_INPUT[LOCK_CHAL_INDEX] = val;
  LOCK_CHAL_INDEX = (LOCK_CHAL_INDEX + 1) % 3;
  Serial.print("Updating Lock Challenge. index=");
  Serial.println(LOCK_CHAL_INDEX);
  Serial.print("Lock Challenge values: ");
  for (int i = 0; i < sizeof(LOCK_CHAL_USER_INPUT) / sizeof(int); i++){
    Serial.print("a[");
    Serial.print(i);
    Serial.print("]=");
    Serial.print(LOCK_CHAL_USER_INPUT[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

boolean lockIsValid(void){
  for (int i = 0; i < 3; i++){
     if (LOCK_CHAL_USER_INPUT[0] > LOCK_CHAL_COMBIN[i][0] &&
          LOCK_CHAL_USER_INPUT[0] < LOCK_CHAL_COMBIN[i][1] &&
          LOCK_CHAL_USER_INPUT[1] > LOCK_CHAL_COMBIN[(i+1)%3][0] &&
          LOCK_CHAL_USER_INPUT[1] < LOCK_CHAL_COMBIN[(i+1)%3][1] &&
          LOCK_CHAL_USER_INPUT[2] > LOCK_CHAL_COMBIN[(i+2)%3][0] &&
          LOCK_CHAL_USER_INPUT[2] < LOCK_CHAL_COMBIN[(i+2)%3][1]) {
        return true; 
     }
  }
  
  return false; 
}

void updateIRChallenge(void){
  int i;
  
  IR_CHAL_USER_INPUT[IR_CHAL_INDEX] = irValue;
  IR_CHAL_INDEX = (IR_CHAL_INDEX + 1) % 4;
  Serial.print("Updating IR Challenge. index=");
  Serial.println(IR_CHAL_INDEX);
  Serial.print("IR Challenge values: ");
  for (int i = 0; i < sizeof(IR_CHAL_USER_INPUT) / sizeof(long); i++){
    Serial.print("a[");
    Serial.print(i);
    Serial.print("]=");
    Serial.print(IR_CHAL_USER_INPUT[i],HEX);
    Serial.print(" ");
  }
  Serial.println("");
}

boolean irIsValid(void){
  int potVal = map(potValue,0,1023,0,100);
  for (int i = 0; i < 4; i++){
     if (IR_CHAL_USER_INPUT[0] == IR_CHAL_VALUES[i] &&
          IR_CHAL_USER_INPUT[1] == IR_CHAL_VALUES[(i+1)%4] &&
          IR_CHAL_USER_INPUT[2] == IR_CHAL_VALUES[(i+2)%4] &&
          IR_CHAL_USER_INPUT[3] == IR_CHAL_VALUES[(i+3)%4]){
        if (lightValue > 300 && potVal == 50){
          return true;
        }        
        Serial.println("Almost!");
        return false;
     }
  }
  
  return false; 
}

boolean christmasTreeIsValid(void){
  if (clockCount % 4 == 0){
    Serial.print("Registers state. ");
    for (int i = 1; i < 8; i=i+2){
      Serial.print("reg[");
      Serial.print(i);
      Serial.print("]=");
      Serial.print(registers[i]);
      Serial.print(" ");
    }
    Serial.println("");
  }
  
  for (int i = 1; i < 8; i=i+2){
    if (registers[i] != HIGH){
      return false;
    }
  }
  
  return true; 
}

void updateRCChallenge(void){
  int i;
  
  RC_CHAL_USER_INPUT[RC_CHAL_INDEX][0] = potValue;
  RC_CHAL_USER_INPUT[RC_CHAL_INDEX][1] = lightValue; 
  RC_CHAL_INDEX = (RC_CHAL_INDEX + 1) % 5;
  Serial.print("Updating RC Challenge. index=");
  Serial.println(RC_CHAL_INDEX);
  Serial.print("RC Challenge values: ");
  for (int i = 0; i < sizeof(RC_CHAL_USER_INPUT) / sizeof(int) / 2; i++){
    Serial.print("a[");
    Serial.print(i);
    Serial.print("]=");
    Serial.print(RC_CHAL_USER_INPUT[i][0]);
    Serial.print("-");
    Serial.print(RC_CHAL_USER_INPUT[i][1]);
    Serial.print(" ");
  }
  Serial.println("");
}

boolean rcIsValid(void){
  int startIndex = 0;
  int endIndex = 4;
  int tolerance = 2;
  int i;
  
  // Find smallest value as startIndex
  for (i = 0; i < 5; i++){
    if (RC_CHAL_USER_INPUT[i][0] < RC_CHAL_USER_INPUT[startIndex][0]){
      startIndex = i; 
    }
  }
  endIndex = (startIndex+4)%5;
  
  // First value must be 0
  if (RC_CHAL_USER_INPUT[startIndex][0] != 0 && RC_CHAL_USER_INPUT[startIndex][1] != 0){
    return false;
  }
  
  // Last value must be < 950
  if (RC_CHAL_USER_INPUT[endIndex][0] > 950 && RC_CHAL_USER_INPUT[endIndex][1] > 950){
    return false;
  }
  
  // All values must be different
  for (i = 0; i < 4; i++){
    if (!(RC_CHAL_USER_INPUT[i%5][0] != RC_CHAL_USER_INPUT[(i+1)%5][0] && 
        RC_CHAL_USER_INPUT[i%5][1] != RC_CHAL_USER_INPUT[(i+1)%5][1])){
      return false;
    }
  }
 
  for (i = startIndex; i < (startIndex + 5); i++){
    if (!(RC_CHAL_USER_INPUT[i%5][0] <= RC_CHAL_USER_INPUT[i%5][1] + tolerance &&
        RC_CHAL_USER_INPUT[i%5][0] >= RC_CHAL_USER_INPUT[i%5][1] - tolerance)){
      return false;
    }
  }
  
  return true; 
}

void printLockFlag(void){
  setRegisterPin(LED_FLAG, HIGH);
  for (int i = 0; i < sizeof(FLAG_1) - 1; i++){
    flagValue[i] = FLAG_1[i];
  }
  Serial.println("Lock Flag was printed");
}

void printIRFlag(void){
  setRegisterPin(LED_FLAG, HIGH);
  for (int i = 0; i < sizeof(FLAG_2) - 1; i++){
    flagValue[i] = FLAG_2[i];
  }
  Serial.println("IR Flag was printed");
}

void printChristmasTreeFlag(void){
  setRegisterPin(LED_FLAG, HIGH);
  for (int i = 0; i < sizeof(FLAG_3) - 1; i++){
    flagValue[i] = FLAG_3[i];
  }
  Serial.println("Christmas tree Flag was printed");
}

void printRCFlag(void){
  setRegisterPin(LED_FLAG, HIGH);
  for (int i = 0; i < sizeof(FLAG_4) - 1; i++){
    flagValue[i] = FLAG_4[i];
  }
  Serial.println("Race condition Flag was printed");
}

void printTempFlag(void){
  setRegisterPin(LED_FLAG, HIGH);
  for (int i = 0; i < sizeof(FLAG_6) - 1; i++){
    flagValue[i] = FLAG_6[i];
  }
  Serial.println("Temperature Flag was printed");
}

int getTemp(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    ds.reset_search();
    //delay(250);
    return 0;
  }
  
  //Serial.print("ROM =");
  //for( i = 0; i < 8; i++) {
  //  Serial.write(' ');
  //  Serial.print(addr[i], HEX);
  //}

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 0;
  }
  //Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return 0;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    //  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  //Serial.print("Temperature = ");
  //Serial.print(celsius);
  //Serial.print(" Celsius, ");
  //Serial.print(fahrenheit);
  //Serial.println(" Fahrenheit");

  return celsius;
}

long getIR(void) {
  char buf[16];
  if (irrecv.decode(&signals)) {
    long ret = signals.value;
    // Serial.println(signals.value, HEX);
    translateIR(buf, signals.value);
    
    Serial.print("IR Value: ");
    Serial.print(ret, HEX);
    Serial.print(" DecodeType: ");
    Serial.print(signals.decode_type);
    Serial.print(" Str: ");
    Serial.println(buf);
    irrecv.resume(); // get the next signal

    setRegisterPin(LED_IR, HIGH);
    return ret;
  }
  setRegisterPin(LED_IR, LOW);
  return 0;
}

// takes action based on IR code received
void translateIR(char* buf, int value) {
 
  switch(value) {

  case 0xFFA25D:  
    strcpy(buf,"POWER");
    break;

  case 0xFF629D:  
    strcpy(buf,"MODE");
    break;

  case 0xFFE21D:  
    strcpy(buf,"MUTE");
    break;

  case 0xFF22DD:  
    strcpy(buf,"PLAY/PAUSE");
    break;

  case 0xFF02FD:  
    strcpy(buf,"PREV");
    break;

  case 0xFFC23D:  
    strcpy(buf,"NEXT");
    break;

  case 0xFFE01F:  
    strcpy(buf,"EQ");
    break;

  case 0xFFA857:  
    strcpy(buf,"VOL-");
    break;

  case 0xFF906F:  
    strcpy(buf,"VOL+");
    break;

  case 0xFF6897:  
    strcpy(buf,"0");
    break;

  case 0xFF9867:  
    strcpy(buf,"100+");
    break;

  case 0xFFB04F:  
    strcpy(buf,"200+");
    break;

  case 0xFF30CF:  
    strcpy(buf,"1");
    break;

  case 0xFF18E7:  
    strcpy(buf,"2");
    break;

  case 0xFF7A85:  
    strcpy(buf,"3");
    break;

  case 0xFF10EF:  
    strcpy(buf,"4");
    break;

  case 0xFF38C7:  
    strcpy(buf,"5");
    break;

  case 0xFF5AA5:  
    strcpy(buf,"6");
    break;

  case 0xFF42BD:  
    strcpy(buf,"7");
    break;

  case 0xFF4AB5:  
    strcpy(buf,"8");
    break;

  case 0xFF52AD:  
    strcpy(buf,"9");
    break;

  default: 
    strcpy(buf,"other button");
  }
}

//set all register pins to LOW
void clearRegisters(){
  for(int i = numOfRegisterPins - 1; i >=  0; i--){
     registers[i] = LOW;
  }
} 


//Set and display registers
//Only call AFTER all values are set how you would like (slow otherwise)
void writeRegisters(){

  digitalWrite(RCLK_Pin, LOW);

  for(int i = numOfRegisterPins - 1; i >=  0; i--){
    digitalWrite(SRCLK_Pin, LOW);

    int val = registers[i];

    digitalWrite(SER_Pin, val);
    digitalWrite(SRCLK_Pin, HIGH);

  }
  digitalWrite(RCLK_Pin, HIGH);
}

//set an individual pin HIGH or LOW
void setRegisterPin(int index, int value){
  registers[index] = value;
  writeRegisters();
}

boolean isButtonPressed(void) {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  } 
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        lastButtonState = reading;
        setRegisterPin(LCD_BTN, HIGH);
        return true;
      }else{
        setRegisterPin(LCD_BTN, LOW);
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  return false;
}

void printModeToLCD(char* msg){
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(msg); 
}

char* getLCDMsg(char* buf, int index){

  
  switch(index) {

  // Electro
  case 0:  
    sprintf(buf, LCD_FMT_STR[index]);
    break;  
    
  // Potentiometer
  case 1:  
    sprintf(buf, LCD_FMT_STR[index], potValue);
    break;  
    
  // Temperature
  case 2:  
    sprintf(buf, LCD_FMT_STR[index], tempValue);
    break;  
    
  // Light
  case 3:  
    sprintf(buf, LCD_FMT_STR[index], lightValue);
    break;  
    
  // IR
  case 4:  
    sprintf(buf, LCD_FMT_STR[index], irValue);
    break;
    
  // Flag
  case 5:  
    sprintf(buf, LCD_FMT_STR[index], flagValue);
    break;
  } 

  if (clockCount % 4 == 0){  
    //Serial.println("getLCDMsg()");
    Serial.print("Menu index: ");
    Serial.println(index);
    //Serial.print("Format str: ");
    //Serial.println(LCD_FMT_STR[index]);
    //Serial.print("potValue: ");
    //Serial.println(potValue);
    //Serial.print("buf: ");
    //Serial.println(buf);
    //Serial.println("");
  }
}

void changeMode(void){
  Serial.println("Changing mode");
  Serial.print("From: ");
  Serial.println(LCD_MSG_INDEX);
  LCD_MSG_INDEX = (LCD_MSG_INDEX + 1) % (sizeof(LCD_FMT_STR) / sizeof(char*));
  Serial.print("To: ");
  Serial.println(LCD_MSG_INDEX);
  updateLCD();
}

void updateLCD(void){
  char buf[32];
  getLCDMsg(buf, LCD_MSG_INDEX);
  printModeToLCD(buf);  
}

void loop(void) {
  noInterrupts();
  if (isButtonPressed()){
    changeMode();
  }
  interrupts();
}
