//*****************************************************************************/
//	Program Name: Freshwater Aquarium Controler
//  Ver. 0.009
//  Created 14.June.2014
// 	By Bill Walker
//*****************************************************************************/ 
//  Notes:
//  
//  09/07/14  Added EEPROM routines for feeding times & lights.
//
//  07/07/14  Observed controller lockup when water filter turns on after 
//            feeding interval.
//  06/07/14  Having issues with the controler locking up intermitently when the
//            relays trigger. I have removed the 5v power supply from the relay
//            enclosure and have tried to isolate the control wiring from the
//            110v wiring but still having issues.
//*****************************************************************************/ 
// Revisions:
// 14.06.14  New Program
// 19.06.14  optimized screen writing code. added routine for feeding time
// 22/06/14  Feeding / water filter logic now working
// 23/06/14  Lighting control now working
// 05/07/14  Updated screen displays to show light schedule
// 09/07/14  Added EEPROM routines for lights & feeding times for future screen
//           imputs.
//
// Features:
// 1. Real time clock
// 2. Controls Water filter while feeding fish
// 3. Control Feeding of Fish
// 4. Light Control on/off
// 
// Features to be added:
// 1. PH Sensor - Need to find cheap PH sensor
// 2. Water Level - Need to find cheap water level sensor & small pump.
// 3. Control Water Temp Heater
// 4.  
// 5. 
//*****************************************************************************/ 
//
// 	The circuit:
// 	* list the components attached to each input
// A0 OPEN        D0 OPEN        D6 Water Filter Relay D12 LCD Screen
// A1 RTC Power   D1 OPEN        D7 Food Feeder Relay  D13 OPEN
// A2 RTC Power   D2 LCD Screen  D8 OPEN           
// A3 OPEN        D3 LCD Screen  D9 OPEN
// A4 RTC         D4 LCD Screen  D10 OPEN          
// A5 RTC         D5 LCD Screen  D11 LCD Screen
//*****************************************************************************/ 
//
// Items to be displayed
// Current Time  
// PH 
// Temp
// Next Feed time 
// 
// LCD Display Layout
// Main Screen Default
//      ------------------ 
//      |00:00:00  PH:0.0| 
//      |NFT 00:00   00.F|
//      ------------------
//      Main Screen Pg 2
// ------------------  ------------------
// |00:00:00  PH:0.0|  |00:00:00  PH:0.0|
// |Lites On   00:00|  |Lights Off 00:00|
// ------------------  ------------------
//
//
//****** Temperature Setting Screen ******
// ------------------ 
// |Set Temp   00.0F| 
// |Act Temp   00.0F|
// ------------------
//****** Lights Sched Screen ******
// ------------------ 
// |Lights  On 00:00| 
// |Lights Off 00:00|
// ------------------ 
//****** Feeding Sched Screen ******
// ------------------
// |Feed Time1 00:00| 
// |Feed Time2 00:00|
// ------------------
//*****************************************************************************/

//LIBRARIES
#include <LiquidCrystal.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <EEPROM.h>
#include <EEPROMAnything.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by time_t as 
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define FEEDS_PER_DAY 3
#define WATER_FILTER_OFF  6                        
#define LIGHTS  7                        
#define RELAY3  8                        
#define FEEDER_MOTOR  9
#define NUM_FEEDS 2
#define NUM_LIGHTS 2

//------------------------------------------------------------------------------
// Temperture Sensor variables
//------------------------------------------------------------------------------

float tempC; //Temp in Celcius
float tempF; //Tem in Farenhight
float vref = 5.0; //Voltage reference for temp probe
int reading = 0.0; //Temp sensor value
int tempPin = 0; //Temp sensor plugged analog pin 0
float setTempF = 0.0 ;

//------------------------------------------------------------------------------
// SET STATUS FLAGS
//------------------------------------------------------------------------------

boolean Feeding_Flag = false;
boolean feederMotorRunning = false; 
boolean waterfilterStopped = false; 
boolean Lights_On_Flag = false;
boolean screenDisplayFlag = false;
boolean LCD_Light_On = true;

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Times to feed the fish & light tank, this is a workaround until screen input
// is created.
//------------------------------------------------------------------------------

//int FEEDTime1, FEEDTime2;
int feedFish1H = 8, feedFish1M = 30, feedFish1S = 0,  
    feedFish2H = 18, feedFish2M = 30, feedFish2S = 0; 

//int LIGHTTime1, LIGHTTime2;
int lightTime1H = 7, lightTime1M = 15, lightTime1S = 0,
    lightTime2H = 21, lightTime2M = 30, lightTime2S = 0;    


/****************************EEPROM FUNCTIONS**********************************/

struct config_f {
  int feedFish1h;
  int feedFish1m;
  int feedFish1s;
  int feedFish2h;
  int feedFish2m;
  int feedFish2s;
  } FEEDERsettings;  //0 - 15

  struct config_l {
  int lightTime1h;
  int lightTime1m;
  int lightTime1s;
  int lightTime2h;
  int lightTime2m;
  int lightTime2s;
  } LIGHTsettings;  //16 - 31

  struct config_t {
   float tempFset;
    } tempSettings; //32-33

void SaveFeedTimesToEEPROM () {
  FEEDERsettings.feedFish1h = feedFish1H;
  FEEDERsettings.feedFish1m = feedFish1M;
  FEEDERsettings.feedFish1s = feedFish1S;
  FEEDERsettings.feedFish2h = feedFish2H;
  FEEDERsettings.feedFish2m = feedFish2M;
  FEEDERsettings.feedFish2s = feedFish2S;
  EEPROM_writeAnything ( 0, FEEDERsettings );
}

void SaveLightTimesToEEPROM () {
  LIGHTsettings.lightTime1h = lightTime1H;
  LIGHTsettings.lightTime1m = lightTime1M;
  LIGHTsettings.lightTime1s = lightTime1S;
  LIGHTsettings.lightTime2h = lightTime2H;
  LIGHTsettings.lightTime2m = lightTime2M;
  LIGHTsettings.lightTime2s = lightTime2S;
  EEPROM_writeAnything ( 16, LIGHTsettings );
}

void SaveTempSetpointToEEPROM() {
  tempSettings.tempFset =  setTempF;
   EEPROM_writeAnything (32, tempSettings );
}

void ReadFromEEPROM(){
  EEPROM_readAnything ( 0, FEEDERsettings );
  feedFish1H = FEEDERsettings.feedFish1h;
  feedFish1M = FEEDERsettings.feedFish1m;
  feedFish1S = FEEDERsettings.feedFish1s;
  feedFish2H = FEEDERsettings.feedFish2h;
  feedFish2M = FEEDERsettings.feedFish2m;
  feedFish2S = FEEDERsettings.feedFish2s;

  EEPROM_readAnything ( 16, LIGHTsettings );
  lightTime1H = LIGHTsettings.lightTime1h;
  lightTime1M = LIGHTsettings.lightTime1m;
  lightTime1S = LIGHTsettings.lightTime1s;
  lightTime2H = LIGHTsettings.lightTime2h;
  lightTime2M = LIGHTsettings.lightTime2m;
  lightTime2S = LIGHTsettings.lightTime2s;

  EEPROM_readAnything ( 32, tempSettings );
  setTempF = tempSettings.tempFset;
}

/*************************END OF EEPROM FUNCTIONS******************************/

int state = HIGH;

unsigned long int feedTiming =10000;//feeder motor run time
unsigned long int waterFilter =300000;//water filter off time
unsigned long int screenDisplayTime_1 = 5000;//screen display cycle timer
unsigned long int Lcd_Timer = 90000;//LCD backlight display time
unsigned long int currTemp =2000;
unsigned long previousMillisScreen = 10; 

unsigned long previousMillisWaterFilter = 0; //Water filter off timer
unsigned long previousMillisFeeder = 0; //Feeder on timer
unsigned long previousMillisLCD = 0;  //LCD on timer
unsigned long previousMillisTemp = 0; //Temp Senor read timer

const int feedtimes[NUM_FEEDS][3] = {{ feedFish1H, feedFish1M, feedFish1S}, 
                                    { feedFish2H, feedFish2M, feedFish2S}};
const int lighttimes[NUM_LIGHTS][3] = {{ lightTime1H, lightTime1M, lightTime1S}, 
                                      { lightTime2H, lightTime2M, lightTime2S}};

int next_feed_time[3] = {0,0,0};
int light_on_time[3] = {0,0,0};
int feed_num = 0;
int light_num = 0;
int displayScreen = 0;

/****************************** RTC FUNCTIONS *********************************/
void readClock(){
   if(Serial.available()){
    processSyncMessage();
  }
  if(timeStatus() == timeNotSet){ 
  lcd.print("waiting for sync message");
  }
}

void digitalClockDisplay(){
// Not in use
}
void processSyncMessage() {
// if time sync available from serial port, update time and return true
// time message consists of header & 10 ASCII digits
  while(Serial.available() >=  TIME_MSG_LEN ){  
    char c = Serial.read() ; 
    Serial.print(c);  
    if( c == TIME_HEADER ) {       
      time_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial.read();          
        if( c >= '0' && c <= '9'){   
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
        }
      }   
      setTime(pctime); //Sync clock to the time received on the serial port
    }  
  }
}

/**************************END OF RTC FUNCTIONS********************************/

//------------------------------------------------------------------------------
//Check current time and set next feeding time
//------------------------------------------------------------------------------
void initFeedTimeCounter(){
   for (size_t i = 0; i<3; i++)
   next_feed_time[i] = feedtimes[feed_num][i];
     if( hour() < next_feed_time[0] || hour() == next_feed_time[0] && minute()
      < next_feed_time [1]){
     }
     else{
       feed_num = (feed_num + 1) % NUM_FEEDS;
       for (size_t i = 0; i<3; i++)
       next_feed_time[i] = feedtimes[feed_num][i];
       if( hour() < next_feed_time[0] || hour() == next_feed_time[0] && minute()
       < next_feed_time [1]){
       }
       else{
         feed_num = (feed_num + 1) % NUM_FEEDS;
         for (size_t i = 0; i<3; i++)
         next_feed_time[i] = feedtimes[feed_num][i];
       }
     }        
 }    

 //------------------------------------------------------------------------------
//  Check current time and set tank lights
//------------------------------------------------------------------------------
void initLightTimerCounter(){   
   for (size_t j = 0; j<3; j++)
   light_on_time[j] = lighttimes[light_num][j]; 
     if(hour() < light_on_time[0] || hour() == light_on_time[0] && minute() 
      < light_on_time[1]){
     }
     else{
      light_num = (light_num + 1) % NUM_LIGHTS;
      for (size_t j = 0; j<3; j++)
      light_on_time[j] = lighttimes[light_num][j];
      digitalWrite(LIGHTS,LOW); 
      Lights_On_Flag = !Lights_On_Flag;
//      printStatus();
        if(hour() < light_on_time[0] || hour() == light_on_time[0] && minute() 
          < light_on_time[1]){
        }
        else{
          light_num = (light_num + 1) % NUM_LIGHTS;
          for (size_t j = 0; j<3; j++)
          light_on_time[j] = lighttimes[light_num][j];
          digitalWrite(LIGHTS,HIGH);
          Lights_On_Flag =!Lights_On_Flag;
//          printStatus();
        }
      }  
}

//------------------------------------------------------------------------------
// Tank Lights ON / OFF
//------------------------------------------------------------------------------

void lights(){
  if(Lights_On_Flag == 0 && hour() == light_on_time[0] && minute() 
    == light_on_time[1] && second() == light_on_time[2]){
    digitalWrite(LIGHTS,LOW);
// LCD_Light_On = !LCD_Light_On;//reset timer for LCD backlight
    previousMillisLCD = millis();
    Lights_On_Flag = !Lights_On_Flag;
//    printStatus();
    light_num = (light_num + 1) % NUM_LIGHTS;
    for (size_t j = 0; j<3; j++)
      light_on_time[j] = lighttimes[light_num][j]; 
    }
  else if(Lights_On_Flag == 1 && hour() == light_on_time[0] && minute() 
    == light_on_time[1] && second() == light_on_time[2]){
    digitalWrite(LIGHTS,HIGH);
    Lights_On_Flag = !Lights_On_Flag;
//    printStatus();
    light_num = (light_num + 1) % NUM_LIGHTS;
    for (size_t j = 0; j<3; j++)
      light_on_time[j] = lighttimes[light_num][j]; 
  }
} 

//------------------------------------------------------------------------------
// Read Temp Sensor 
//------------------------------------------------------------------------------
void tempSensorRead(){
  if(millis() - previousMillisTemp > currTemp){
    // document make & model of temp sensor here. different 
    // sensors have different factors.
    reading = analogRead(tempPin); // Read temp sensor voltage
    tempC = ( reading * vref ) / 10;  // convert to degrees Centigrade
    tempF = ( tempC * 1.8 ) + 32;  //convert to degrees Farenhight
    previousMillisTemp = millis();
    if(previousMillisTemp < millis()){
      previousMillisTemp =(previousMillisTemp);
    }
    else{
    }  
  }
   else{
  }
}

//------------------------------------------------------------------------------
// Begin Setup
//------------------------------------------------------------------------------

void setup(){
  lcd.begin(16, 2); // set up the LCD’s number of rows and columns
  Serial.begin(9600);
  Serial.println();
//   Serial.print("-------Feeder Run Time = ");
//   Serial.print(feedTiming/1000);
//   Serial.println(" sec");
//   Serial.print("-------Water Filter Off Time = ");
//   Serial.print(waterFilter/1000);
//   Serial.println(" sec");
  pinMode(A1, OUTPUT);//LCD backlight
  digitalWrite(A1, HIGH);
  pinMode(WATER_FILTER_OFF, OUTPUT);
  digitalWrite(WATER_FILTER_OFF, HIGH); 
  pinMode(LIGHTS, OUTPUT);
  digitalWrite(LIGHTS, HIGH);
  pinMode(FEEDER_MOTOR, OUTPUT);
  digitalWrite(FEEDER_MOTOR, HIGH);
  setSyncProvider(RTC.get);
  readClock();
//   printStatus();
  initFeedTimeCounter();
  initLightTimerCounter();      
  setTempF = 78.6; 
   
//------------------------------------------------------------------------------
// EEPROM memory saves need to be moved to screen updated routine when it is 
// completed.  
//------------------------------------------------------------------------------
//   SaveFeedTimesToEEPROM();
//   SaveLightTimesToEEPROM();
//   SaveTempSetpointToEEPROM();
  ReadFromEEPROM();
}
//------------------------------------------------------------------------------
//  End of Setup
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Begin Main Loop
//------------------------------------------------------------------------------

void loop(){
  digitalWrite(A1, LCD_Light_On? HIGH : LOW);
  readClock();
//  tempSensorRead();
//  phSensorRead();
  feedTime();
  lights();
//  updateScreen();
   updateScreen1();  
//   updateScreen2();
//  LCD_Light();  
}

//------------------------------------------------------------------------------
// End of Main Loop
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Update LCD Screen
//------------------------------------------------------------------------------
void updateScreen(){
  lcd.setCursor(0,0);// set the cursor to column 0, line 0 
  lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
  lcd.setCursor(0,1);// set the cursor to column 0, line 1 
  lcd.printf("NFT %02d:%02d  00.0F", next_feed_time[0], next_feed_time[1]);
}

//------------------------------------------------------------------------------
void updateScreen1(){
  if( millis() - previousMillisScreen > screenDisplayTime_1){
    previousMillisScreen = millis();
    if(screenDisplayFlag == 0){
      screenDisplayFlag = 1;
    }
    else{
      screenDisplayFlag = 0;
    }
    if(screenDisplayFlag == 1){
      lcd.setCursor(0,0);// set the cursor to column 0, line 0 
      lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
      lcd.setCursor(0,1);// set the cursor to column 0, line 1 
      lcd.printf("NFT %02d:%02d  00.0F", next_feed_time[0], next_feed_time[1]);
    } 
    else{
      lcd.setCursor(0,1);// set the cursor to column 0, line 0 
      lcd.print("Set Temp   ");
      lcd.setCursor(11,1);
      lcd.print(tempSettings.tempFset,1);
      lcd.setCursor(15,1);
      lcd.print("F");

    } 
//    else{
//      lcd.setCursor(0,0);// set the cursor to column 0, line 0 
//      lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
//      lcd.setCursor(0,1);// set the cursor to column 0, line 1 
//      if(Lights_On_Flag == 0){
//      lcd.printf("Lites On  %02d:%02d ", light_on_time[0], light_on_time[1]);
//    }
//    else{
//      lcd.printf("Lites Off %02d:%02d   ", light_on_time[0], light_on_time[1]);
//    }  
//    }
  }
  lcd.setCursor(0,0);// set the cursor to column 0, line 0 
  lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
}

void updateScreen2(){
  if( millis() - previousMillisScreen > screenDisplayTime_1){
    previousMillisScreen = millis();
    if(screenDisplayFlag == 0){
      screenDisplayFlag = 1;
    }
      switch (displayScreen) {
        case 0:  // Screen 1
          lcd.setCursor(0,1);// set the cursor to column 0, line 0 
          lcd.print("Screen 1");
          // do something
          displayScreen = 1;
        break;
        case 1: // Screen 2
         lcd.setCursor(0,1);// set the cursor to column 0, line 0 
         lcd.print("Screen 2");
          // do something
          displayScreen = 2;
        break;
        case 2: // Screen 3
          lcd.setCursor(0,1);// set the cursor to column 0, line 0 
          lcd.print("Screen 3");
          // do something
          displayScreen = 0;
        break;
//      default:
        // do something
     }   

  }
  lcd.setCursor(0,0);// set the cursor to column 0, line 0 
  lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
}
//------------------------------------------------------------------------------
// Feed Fish
//------------------------------------------------------------------------------
void feedTime(){
  if(Feeding_Flag == 0 && hour() == next_feed_time[0]  && minute() == 
    next_feed_time[1] && second() == next_feed_time[2]){

    previousMillisLCD = millis();//reset timer for LCD backlight
    LCD_Light_On = !LCD_Light_On; //Turn on LCD backlight

    digitalWrite(WATER_FILTER_OFF,LOW); // Turn off water filter
    previousMillisWaterFilter = millis(); //Reset Water pump timer
    waterfilterStopped = !waterfilterStopped;

    delay(2000);

    digitalWrite(FEEDER_MOTOR,LOW);//Turn on Feeder
    previousMillisFeeder = millis(); //Reset Feeder motor timer
    feederMotorRunning = !feederMotorRunning;//flip flag bit
    Feeding_Flag = !Feeding_Flag;
    feed_num = (feed_num + 1) % NUM_FEEDS;

    for (size_t i = 0; i<3; i++)
      next_feed_time[i] = feedtimes[feed_num][i];
  }
    else if(Feeding_Flag == 1 && waterfilterStopped == 1 && (millis() 
    - previousMillisWaterFilter > waterFilter)){

    digitalWrite(WATER_FILTER_OFF,HIGH); // Turn on water filter 
    waterfilterStopped = !waterfilterStopped;
    Feeding_Flag = !Feeding_Flag;
  }
  else if(Feeding_Flag == 1 && feederMotorRunning == 1 && (millis() 
    - previousMillisFeeder > feedTiming)){

    digitalWrite(FEEDER_MOTOR,HIGH);//Turn off Feeder
    feederMotorRunning = !feederMotorRunning;
  }				
} 


//------------------------------------------------------------------------------
// LCD Backlight ON / OFF
//------------------------------------------------------------------------------

void LCD_Light(){
  if(LCD_Light_On){
    digitalWrite(A1, HIGH);
  }
  else {
  }
  if(LCD_Light_On == 1 && millis() - previousMillisLCD > Lcd_Timer){
    digitalWrite(A1, LOW);
    LCD_Light_On = !LCD_Light_On;
  }   
}  

//------------------------------------------------------------------------------
// Print Status updates to Serial
//------------------------------------------------------------------------------
void printStatus(){
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
  Serial.print("Feeding Time Flag = ");
  Serial.println(Feeding_Flag);
  Serial.print("Water Filter Not Running Flag = ");
  Serial.println(waterfilterStopped);
  Serial.print("Feeder Motor Running Flag = ");
  Serial.println(feederMotorRunning);
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon 
  // and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
    Serial.print(digits);
  }

void LCD_Reset(){

}
//End
