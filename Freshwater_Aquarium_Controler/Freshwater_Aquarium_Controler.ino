//*************************************************************************************/
//	Program Name: Freshwater Aquarium Controler
//  Ver. 0.005
//  Created 14.June.2014
// 	By Bill Walker
//*************************************************************************************/ 
// 
// 
// Revisions:
// 14.06.14  New Program
// 19.06.14  optimized screen writing code. added routine for feeding time
// 22/06/14  Feeding / water filter logic now working
// 23/06/14  Lighting control now working
// Features:
// 1. Real time clock
// 2. Controls Water filter while feeding fish
// 3. Control Feeding of Fish
// 4. Light Control on/off
// 
// Features to be added:
// 1. PH Sensor
// 2. Water Level
// 3. Control Water Temp Heater
// 4.  
// 5. 
//*************************************************************************************/ 
// 	The circuit:
// 	* list the components attached to each input
// A0 OPEN        D0 OPEN        D6 Water Filter Relay D12 LCD Screen
// A1 RTC Power   D1 OPEN        D7 Food Feeder Relay  D13 OPEN
// A2 RTC Power   D2 LCD Screen  D8 OPEN           
// A3 OPEN        D3 LCD Screen  D9 Water Heater Relay
// A4 RTC         D4 LCD Screen  D10 OPEN          
// A5 RTC         D5 LCD Screen  D11 LCD Screen
//*************************************************************************************/ 
// Items to be displayed
// Current Time  
// PH 
// Temp
// Next Feed time 
// 
// CD Display Layout
// ------------------ 
// |00:00:00  PH:0.0| 
// |NFT:00:00T:00.0F|
// ------------------
//*************************************************************************************/
//
//LIBRARIES
#include <LiquidCrystal.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // initialize the library with the numbers of the interface pins
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define FEEDS_PER_DAY 3
#define WATER_FILTER_OFF  6                        
#define LIGHTS  7                        
#define RELAY3  8                        
#define FEEDER_MOTOR  9
#define NUM_FEEDS 2
#define NUM_LIGHTS 2
//------------------------------------------------------------------------------------
// Temperture Sensor variables
float tempC;
float tempF;
float voltage;
boolean Feeding_Flag = false;
boolean feederMotorRunning = false;
boolean waterfilterStopped = false;
boolean Lights_On_Flag = false;
boolean LCD_Light_On = true;
int reading;
int tempPin = 0; //Temp sensor plugged analog pin 0
//------------------------------------------------------------------------------------
// Feeding time variables
int feedTimeHourDisplay  = 0; //Next scheduled feed time hour
int feedTimeMinDisplay   = 0; //Next scheduled feed time minute
int feedTimeHour1 = 8; // 1st feeding time hours
int feedTimeMin1 = 30; // 1st feeding time minutes
int feedTimeHour2 = 18; // 2nd feeding time hours
int feedTimeMin2 = 30;    // 2nd feeding time minutes
int state = HIGH;
unsigned long int feedTiming =100000;//feeder motor run time
unsigned long int waterFilter =300000;//water filter off time
unsigned long int Lcd_Timer = 90000;//LCD backlight display time
unsigned long int prevTime_2 =2000;
int interval_1 = 10;  
int interval_2 = 10; 
int interval_3 = 0; 
const int feedtimes[NUM_FEEDS][3] = {{ 8, 30, 0}, {18, 30, 0}};
const int lighttimes[NUM_LIGHTS][3] = {{ 7, 15, 0}, {21, 30, 0}};
int next_feed_time[3] = {0,0,0};
int light_on_time[3] = {0,0,0};
int feed_num = 0;
int light_num = 0;
//------------------------------------------------------------------------------------
void setup(){
   lcd.begin(16, 2); // set up the LCD’s number of rows and columns
   pinMode (A3, OUTPUT); //setup arduino to power RTC from analog pins (NEED TO MOVE POWER OFF OF THIS PIN)
   digitalWrite(A3, HIGH);
   pinMode(A2, OUTPUT); //setup arduino to power RTC from analog pins (NEED TO MOVE POEW OFF OF THIS PIN)
   digitalWrite(A2, LOW);
   pinMode(A1, OUTPUT);//LCD backlight
   digitalWrite(A1, HIGH);
   pinMode(WATER_FILTER_OFF, OUTPUT);
   digitalWrite(WATER_FILTER_OFF, HIGH);   
   pinMode(LIGHTS, OUTPUT);
   digitalWrite(LIGHTS, HIGH);
   pinMode(FEEDER_MOTOR, OUTPUT);
   digitalWrite(FEEDER_MOTOR, HIGH);
   Serial.begin(9600);
   setSyncProvider(RTC.get);
   readClock();
   for (size_t i = 0; i<3; i++)
   next_feed_time[i] = feedtimes[feed_num][i];
     if( hour() <= next_feed_time[0] && minute() <= next_feed_time[1]){
     }
     else{
       feed_num = (feed_num + 1) % NUM_FEEDS;
       for (size_t i = 0; i<3; i++)
       next_feed_time[i] = feedtimes[feed_num][i];
         if( hour() >= next_feed_time[0] && minute() >= next_feed_time[1]){
           feed_num = (feed_num + 1) % NUM_FEEDS;
           for (size_t i = 0; i<3; i++)
           next_feed_time[i] = feedtimes[feed_num][i];
         }
      }        
//------------------------------------------------------------------------------------
   for (size_t j = 0; j<3; j++)
   light_on_time[j] = lighttimes[light_num][j]; 
     if(hour() <=light_on_time[0] && minute() <= light_on_time[1]){
     }
     else{
       light_num = (light_num + 1) % NUM_LIGHTS;
       for (size_t j = 0; j<3; j++)
       light_on_time[j] = lighttimes[light_num][j];
       digitalWrite(LIGHTS,LOW);   
       Lights_On_Flag = !Lights_On_Flag;
//         if(hour() >=light_on_time[0] && minute() >= light_on_time[1]){
//      	   light_num = (light_num + 1) % NUM_LIGHTS;
//           for (size_t j = 0; j<3; j++)
//           light_on_time[j] = lighttimes[light_num][j];
//           digitalWrite(LIGHTS,HIGH);   
//         }
      }  
     interval_1 = millis();
     interval_2 = millis();
}
//------------------------------------------------------------------------------------
void loop(){
  digitalWrite(A1, LCD_Light_On? HIGH : LOW);
  readClock();
//  sensorRead();
  feedTime();
  lights();
  updateScreen();
//  LCD_Light();  
 }
//------------------------------------------------------------------------------------
// get time from RTC
// T1262347200  //noon Jan 1 2010 **Push this time code from Serial monitor until RTC installed
//------------------------------------------------------------------------------------
void readClock(){
   if(Serial.available()){
    processSyncMessage();
  }
  if(timeStatus() == timeNotSet){ 
  lcd.print("waiting for sync message");
  }
}

void digitalClockDisplay(){
//Not in use
}
void processSyncMessage() {
  // if time sync available from serial port, update time and return true
  while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of header & 10 ASCII digits
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
      setTime(pctime);   // Sync Arduino clock to the time received on the serial port
    }  
  }
}

//------------------------------------------------------------------------------------
// Read Temp Sensor 
void sensorRead(){
  if(millis() - prevTime_2 > interval_3){
    // document make & model of temp sensor here. different 
    // sensors have different factors.
    reading = analogRead(tempPin); // Read temp sensor voltage
    tempC = (reading *5.0)/10;  // convert to degrees Centigrade
    tempF = (tempC*1.8)+32;  //convert to degrees Farenhight
    prevTime_2 = millis();
    if(prevTime_2 < millis()){
      prevTime_2 =(prevTime_2);
    }
    else{
    }  
  }
  else{
  }
}
//------------------------------------------------------------------------------------
void updateScreen(){
  lcd.setCursor(0,0);// set the cursor to column 0, line 0 
  lcd.printf("%02d:%02d:%02d  PH:0.0", hour(), minute(), second());
  lcd.setCursor(0,1);// set the cursor to column 0, line 1 
  lcd.printf("NFT %02d:%02d  00.0F", next_feed_time[0], next_feed_time[1]);
}

//------------------------------------------------------------------------------------
void feedTime(){
  if(Feeding_Flag == 0 && hour() == next_feed_time[0]  && minute() == next_feed_time[1] && second() == next_feed_time[2]){
    interval_1 = millis();
    interval_2 = millis();
    LCD_Light_On = !LCD_Light_On;
    interval_3 = millis();//reset timer for LCD backlight
    feederMotorRunning = !feederMotorRunning;//flip flag bit
    waterfilterStopped = !waterfilterStopped;
    digitalWrite(WATER_FILTER_OFF,LOW); // Turn off water filter
    delay(2000);
    digitalWrite(FEEDER_MOTOR,LOW);//Turn on Feeder
    Feeding_Flag = !Feeding_Flag;
    feed_num = (feed_num + 1) % NUM_FEEDS;
    for (size_t i = 0; i<3; i++)
      next_feed_time[i] = feedtimes[feed_num][i];
  }
  else if(Feeding_Flag == 1 && feederMotorRunning == 1 &&(millis() - interval_2 > feedTiming)){
    digitalWrite(FEEDER_MOTOR,HIGH);//Turn off Feeder
    feederMotorRunning = !feederMotorRunning;
  }
  else if(Feeding_Flag == 1 && waterfilterStopped == 1 && (millis() - interval_1 > waterFilter)){
    digitalWrite(WATER_FILTER_OFF,HIGH); // Turn on water filter 
    waterfilterStopped = !waterfilterStopped;
  }				
} 

void lights(){
  if(Lights_On_Flag == 0 && hour() == light_on_time[0] && minute() == light_on_time[1] && second() == light_on_time[2]){
    digitalWrite(LIGHTS,LOW);
//    LCD_Light_On = !LCD_Light_On;//reset timer for LCD backlight
    interval_3 = millis();
    Lights_On_Flag = !Lights_On_Flag;
    light_num = (light_num + 1) % NUM_LIGHTS;
    for (size_t j = 0; j<3; j++)
      light_on_time[j] = lighttimes[light_num][j]; 
    }
  else if(Lights_On_Flag == 1 && hour() == light_on_time[0] && minute() == light_on_time[1] && second() == light_on_time[2]){
    digitalWrite(LIGHTS,HIGH);
    Lights_On_Flag = !Lights_On_Flag;
    light_num = (light_num + 1) % NUM_LIGHTS;
    for (size_t j = 0; j<3; j++)
      light_on_time[j] = lighttimes[light_num][j]; 
  }
} 
void LCD_Light(){
  if(LCD_Light_On){
    digitalWrite(A1, HIGH);
  }
  else {
  }
  if(LCD_Light_On == 1 && millis() - interval_3 > Lcd_Timer){
    digitalWrite(A1, LOW);
    LCD_Light_On = !LCD_Light_On;
  }   
}  
//End
