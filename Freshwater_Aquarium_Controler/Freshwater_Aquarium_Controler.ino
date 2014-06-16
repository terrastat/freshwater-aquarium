/*
	Program Name: Freshwater Aquarium Controler
 Ver. 0.001
 Created 14.June.2014
 	By Bill Walker
 
 Revisions:
 14.06.14  New Program
 
 Features:
 1. Control Water Temp Heater
 2. Control Feeding of Fish
 3. Controls Water filter while feeding fish
 4. 
 
 Features to be added:
 1. Real time clock
 2. PH Sensor
 3. Water Level
 4. 
 
 	The circuit:
 	* list the components attached to each input
 A0 Temp Sensor D0 n/a         D6 Water Filter Relay D12 LCD Screen
 A1 PH Sensor   D1 n/a         D7 Food Feeder Relay  D13 
 A2 Water level D2 LCD Screen  D8 Water Heater Relay           
 A3             D3 LCD Screen  D9 
 A4             D4 LCD Screen  D10           
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

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
//
float tempC;
float tempF;
float voltage;
int reading;
int tempPin = 0; //Temp sensor plugged analog pin 0
int phPin = 0; //PH sensor plugged analog pin 1
int threshTemp = 77; //Alarm Temperature threshold (in F)
int pinLed = 9; //Digital
int state = HIGH;
//int currSwitch;
//int prevSwitch = LOW;
int pinSwitch = 6; //digital
unsigned long int stateTime=0;
unsigned long int printTime=0;
unsigned long int prevTime_1 =2000;
unsigned long int prevTime_2 =2000;
unsigned long int buzzTime=1000;
int interval_1 = 250;  //in ms (how often screen reprints
int interval_2 = 4000;  //in ms (how often screen reprints
long int debounce = 200;
int secondDisplayedOnLcd = 2;

//------------------------------------------------------------------------------------
void setup(){
  pinMode(pinLed, OUTPUT);
  pinMode(pinSwitch, INPUT);
  lcd.begin(16, 2); // set up the LCD’s number of rows and columns
  Serial.begin(9600);
}
//------------------------------------------------------------------------------------
void loop(){
  readClock();
//  sensorRead();
//  feedTime();
  updateScreen();
}
//------------------------------------------------------------------------------------
// get time from RTC
// T1262347200  //noon Jan 1 2010 **Push this time code from Serial monitor until RTC installed
void readClock(){
   if(Serial.available() ) 
  {
    processSyncMessage();
  }
  if(timeStatus() == timeNotSet){ 
    Serial.println("waiting for sync message");
 //   lcd.print("waiting for sync message");
  }
  else     
      digitalClockDisplay();  
//delay(1000);
}

void digitalClockDisplay(){
  // digital clock display of the time
  if( secondDisplayedOnLcd != second() ){
    secondDisplayedOnLcd = second();
   }
 
  lcd.setCursor(0,0);
  printDigitsHour(hour());
  printDigits(minute());
  printDigits(second());
}

void printDigits(byte digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits,DEC);
}
void printDigitsHour (byte digits){
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits,DEC);
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
  /*      LCD Display Layout
              111111     
    0123456789012345 
   |00:00:00  PH:0.0| 
   |NFT:00:00T:00.0F|
   ------------------  
   */
  lcd.setCursor(0,0);
//  lcd.print("Time:");
  lcd.setCursor(10,0);
  lcd.print("PH:0.0");

//  lcd.setCursor(10,0);
//  lcd.print(ph);
  lcd.setCursor(0,1);// set the cursor to column 0, line 1 
  lcd.print("NFT:00:00T:00.0F");
  lcd.setCursor(5,1);
  //  lcd.print(feedTime);
  lcd.setCursor(12,1);
//  lcd.print(tempF);  //Print Fahrenheit temperature to LCD 
  printTime = millis();
  delay(1000);  //wait 1 seconds
}

//------------------------------------------------------------------------------------

//End

