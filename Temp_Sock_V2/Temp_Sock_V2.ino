/*
 * Temperature Sock Data Logger
 * This code reads the temperature from a sock mounted thermistor in 30 second intervals and records that data on an SD card
  
  Written July 2017 by Jonathan Melton using code writen by others
  Creative commons licence, feel free to use, modify etc. if you find this code helpful
  
  
  SD card datalogger

 This example shows how to log data from three analog sensors
 to an SD card using the SD library.

 SD card reader circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe

 */
// Define included libraries for the SD card
#include <SPI.h>
#include <SD.h>

// Define Global Variables
const int chipSelect = 4; //SD card test pin
float last_read = 0; //last clock time in millis
float time_interval = 5; //time interval between reads in seconds
const int switchpin = 2; //Pin to which the data record on/off switch is attached


// Setup Global variables for the thermistor
// which analog pin to connect
const int THERMISTORPIN = A0  ;
// resistance at 25 degrees C
#define THERMISTORNOMINAL 9880 //Nominal is 10000, I checked mine with a multimeter and got 988*      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 10
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    
 
uint16_t samples[NUMSAMPLES]; //Array for storing the samples to be averaged

void setup() {
  // Open serial communications with the computer and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  // Check to see if an SD card present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    sd_error_blink(); //Blink LED continuously in the event of a SD card failure
    return;
  }
  Serial.println("card initialized.");

  //Blink LED on successful SD card intalization
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(6, OUTPUT); // Set pin 6 as output
  sd_good_blink();

  
}

void loop() {
    //check to see if the switch is in the "on" (high) position. If it's not spin program until switch is activated
    //Serial.println(digitalRead(switchpin));
    while(digitalRead(switchpin)==0){
        //do nothing
        switch_off_blink(); //Error information from LED
        Serial.println("Stuck");
    }
    
  // make a string for assembling the data to log:
  String dataString = "";

    //Sensor read code from the original SD example, I am only using one sensor in my application
  /*// read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2) {
      dataString += ",";
    }
  }*/
  Serial.println(millis()- last_read);
  //Serial.println(time_interval*1000 < millis()- last_read);
 
  // If 30 seconds have passed, write data to the SD card
  if (time_interval*1000 < millis()- last_read ){
     //Update timer
     last_read = millis();
     
     //Construct datastring
     dataString += String(last_read/1000); // Record system time
     dataString += ",";
     dataString += String(thermistor_read()); //Read thermistor

     // open the file. note that only one file can be open at a time,
     // so you have to close this one before opening another.
     File dataFile = SD.open("datalog.txt", FILE_WRITE);
 
    // if the file is available on the SD card, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
      sd_error_blink;
    }
     
  //Blink LED to confirm Data write
  sd_write_blink();                  // Waits for roughly 30 seconds before logging next data set
  }

  
}


void sd_good_blink(){
  //This function turns the led on for a second, then blinks for .25 seconds to indicate the sd card is in
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(2000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
}

void sd_error_blink(){
  //This function blinks the LED continuously to indicate an error with SD card intalization
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                        // wait for half a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                        // wait for half a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);
}

void sd_write_blink(){
  //This function blinks the LED once to indicate a successful write
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                        // wait for half a second
  digitalWrite(LED_BUILTIN,LOW);    // Turn off LED
}

void switch_off_blink(){
    //This function informs the user that the switch is off and the device is not recording
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                        // wait for half a second
    digitalWrite(LED_BUILTIN,LOW);    // Turn off LED
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                        // wait for half a second
    digitalWrite(LED_BUILTIN,LOW);    // Turn off LED
    delay(1000);
}

float thermistor_read() {
  //Read the thermistor
  //Code written by someone on the Aidafruit website
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  digitalWrite(6,HIGH); //Turn on divider pin
  delay(10);
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
  digitalWrite(6,LOW); //Turn off divider pin
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  Serial.print("Average analog reading "); 
  Serial.println(average);
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance "); 
  Serial.println(average);
 
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
 
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" *C");
  
  return steinhart;
}





