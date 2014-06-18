/*
	Program Name: Freshwater Aquarium Controler
 Ver. 0.001
 Created 14.June.2014
 	By Bill Walker
 
 
 
 Revisions:
 14.06.14  New Program
 
 Features:
 1. Real time clock
 2.
 3. 
 4. 
 
 Features to be added:
 1. PH Sensor
 2. Water Level
 3. Control Water Temp Heater
 4.  Control Feeding of Fish
 5. Controls Water filter while feeding fish
 
 	The circuit:
 	* list the components attached to each input
 A0 Temp Sensor D0 n/a         D6 Water Filter Relay D12 LCD Screen
 A1 PH Sensor   D1 n/a         D7 Food Feeder Relay  D13 
 A2 RTC         D2 LCD Screen  D8 Water Heater Relay           
 A3 RTC         D3 LCD Screen  D9 
 A4 Water level D4 LCD Screen  D10           
 A5             D5 LCD Screen  D11 LCD Screen
 
 	* list the components attached to each output
 
 Items to be displayed
 Current Time  
 PH 
 Temp
 Next Feed time 
 
 CD Display Layout
 ------------------ 
 |00:00:00  PH:0.0| 
 |NFT:00:00T:00.0F|
 ------------------
*/

#include <LiquidCrystal.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);// initialize the library with the numbers of the interface pins
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define FEEDS_PER_DAY 3
// Temperture Sensor variables
float tempC;
float tempF;
float voltage;
int reading;
int tempPin = 0; //Temp sensor plugged analog pin 0
int threshTemp = 77; //Alarm Temperature threshold (in F)

int phPin = 0; //PH sensor plugged analog pin 1
// Feeding time variables
int feedTimeHourDisplay  = 0; //Next scheduled feed time hour
int feedTimeMinDisplay   = 0; //Next scheduled feed time minute
int feedTimeHour1 = 8; // 1st feeding time hours
int feedTimeMin1 = 30; // 1st feeding time minutes
int feedTimeHour2 = 18; // 2nd feeding time hours
int feedTimeMin2 = 30;    // 2nd feeding time minutes

int state = HIGH;
unsigned long int stateTime=0;
unsigned long int printTime=0;
unsigned long int prevTime_1 =2000;
unsigned long int prevTime_2 =2000;
int interval_1 = 250;  //in ms (how often screen reprints
int interval_2 = 4000;  //in ms (how often screen reprints
const int HOURS_IN_DAY = 24;
const int MINUTES_IN_HOUR = 60;
const int SECONDS_IN_MINUTE = 60;
const int SECONDS_IN_DAY = 86400;
const int SECONDS_IN_EIGHT_HOURS = 28800;

//------------------------------------------------------------------------------------
void setup(){
   lcd.begin(16, 2); // set up the LCD’s number of rows and columns
   pinMode (A3, OUTPUT); //setup arduino to power RTC from analog pins
   digitalWrite(A3, HIGH);
   pinMode(A2, OUTPUT);
   digitalWrite(A2, LOW);
   Serial.begin(9600);
   setSyncProvider(RTC.get);
   readClock();
   feedTimeHourDisplay = feedTimeHour1;
   feedTimeMinDisplay = feedTimeMin1;
   if(hour() > feedTimeHourDisplay /*&& minute() >= feedTimeMinDisplay*/){
         feedTimeHourDisplay = feedTimeHour2;
         feedTimeMinDisplay = feedTimeMin2;
   }
}
//------------------------------------------------------------------------------------
void loop(){
  readClock();
//  sensorRead();
  feedTime();
  updateScreen();
 }
//------------------------------------------------------------------------------------
// get time from RTC
// T1262347200  //noon Jan 1 2010 **Push this time code from Serial monitor until RTC installed
void readClock(){
   if(Serial.available()){
    processSyncMessage();
  }
  if(timeStatus() == timeNotSet){ 
  lcd.print("waiting for sync message");
  }
//  else     
     // digitalClockDisplay();  
//delay(1000);
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
  if(millis() - prevTime_2 > interval_2){
    // document make & model of temp sensor here. different 
    //    sensors have different factors.
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
  lcd.printf("NFT %02d:%02d  00.0F", feedTimeHourDisplay, feedTimeMinDisplay);
}

//------------------------------------------------------------------------------------
void feedTime(){
     if(hour() == feedTimeHour1 && minute() == feedTimeMin1){
         feedTimeHourDisplay = feedTimeHour2;
         feedTimeMinDisplay = feedTimeMin2;
         
     }    
     if(hour() == feedTimeHour2 && minute() == feedTimeMin2){    
         feedTimeHourDisplay = feedTimeHour1;
         feedTimeMinDisplay = feedTimeMin1;
     }      
} 

//End

