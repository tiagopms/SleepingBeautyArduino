/*   ECS_DetectSleep.ino
 Purpose: Mid term demo
 Authot: Tiago Pimentel
        t.pimentelms@gmail.com
 CSE 467S -- Embedded Computing Systems
 WUSTL, Spring 2013
 Date: Mar., 22, 2013
 
 Invariants
   LEARNING_MEASURES >= 1
   APPLYING_MEASURES >= 1
   STILL_TIME >= 10
   ROUGH_MOVE_TIMEOUT >= 5
 Description:
   Use a relay to turn lights on or off
   Turn off the lights if :
     detect STILL_TIME seconds without movements (will be removed and replaced by rasing heart rate accepted percentage)
     || switchLight says to turn off the light
     || heart rate of APPLYING_MEASURES drops to bellow of 90% of LEARNING_MEASURES
   Turn on the lights if:
     there is a really rough movement in less than ROUGH_MOVE_TIMEOUT after the light turned off
     || switch_light says to turn on the light
 Version Log:
   03/22/2013, Tiago Pimentel
     Created file;
     Created relay activating and deactivating code, and physical project;
     Created Heart Rate code (without Heart Rate module);
   03/23/2013, Tiago Pimentel
     Created serial reading and writing functions (without purpose so far);
   03/24/2013, Tiago Pimentel
     Created switchLight function;
     Created "atoulli" (string to unsigned long long int) implementation for serial comunication;
   03/26/2013, Tiago Pimentel
     Created timeWithoutMovement function;
     Created reallyRoughMovement function;
     Decided the serial comunication protocol;
     Corrected minor bugs, as light not turning on with reallyRoughMovement;

*/
#include <Time.h>  

//constants definitions
#define LEARNING_MEASURES 100
#define APPLYING_MEASURES 100
#define STILL_TIME 100
#define ROUGH_MOVE_TIMEOUT 60

//variables declaration
//incoming data from serial
float incomingByte = 0;
//setting time in arduino
int isGettingTime = true;
unsigned long int startingTime = 0;
unsigned long int turnOffTime = 0;
//identifying type or reading real data
int isDecodingType = true;
//type of data from serial
int typeOfData = 1;
//decodes string to int variable
unsigned long int timeData = 0;
uint64_t timeDataLong = 0;
//constant related to percentage of chance to be asleep, changes velocity of heart rate lowering response
float heartRatePercentConstant = 0;

//give a name to relay pin and sets a variable with its state
int relay = 4;
int isOff = false;

//arduino setup
void setup(){
  // initialize the relay pin as an output.
  pinMode(relay, OUTPUT);
  //start serial reading and writing
  Serial.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
}

//analyses heart beat data from sensor
//receives the heartRate
void heartBeatData (int heartRate) {
  if(isOff) {
    return;
  }
  
  //current and learned heart rates
  static float heartRatesMean = 0;
  static float currentHeartRate = 0;
  //counters of number of measurements
  static float timesCalled = 0;
  static float currentCounter = 0;
  //update number of times data received
  timesCalled++;
  
  //learning heart rate mean
  if (timesCalled <= LEARNING_MEASURES) {
    heartRatesMean = heartRatesMean*(timesCalled - 1)/timesCalled + incomingByte/timesCalled;
    Serial.print("Heart Rates: ");
    Serial.println(heartRatesMean);
  }
  //applying heart rate mean measured
  else {
    currentCounter++;
    //getting current heart rates mean
    currentHeartRate = currentHeartRate*(currentCounter - 1)/currentCounter + incomingByte/currentCounter;
    Serial.print("Current Heart Rate: ");
    Serial.println(currentHeartRate);
    //check if new mean is smaller than 90% of original
    if (currentCounter == APPLYING_MEASURES) {
      currentCounter = 0;
      Serial.println("Final!");
      if (currentHeartRate < heartRatesMean*0.9) {
        Serial.println("Lower");
        digitalWrite(relay, HIGH);   // turn the LED on (HIGH is the voltage level)
        isOff = true;
      }
    }
  }
}

//handles time without movement data
//receives time without movement 
void timeWithoutMovement (unsigned long int time) {
  Serial.print("Time data without movement: ");
  Serial.println(time);
  //if no movement in an amount of time change needed heart rate lowering response to 5% instead of 10%
  if (time > STILL_TIME && !isOff) {
    //since no heart rate changed to just turn off light
    //heartRatePercentConstant = 0.95/0.9;
    digitalWrite(relay, HIGH);
    Serial.println("DATA 0");
    turnOffTime = startingTime + (unsigned long int) (millis()/1000.0);
    isOff = true;
  }
}

//turns on or off the light
//receives a int indicating if it should turn the light on or off
int switchLight (int turnOn) {
  if(turnOn) {
    digitalWrite(relay, LOW); //low to allow light on
    isOff = false;
  } else {
    digitalWrite(relay, HIGH); //high to turn off light
    if(!isOff) {
      turnOffTime = 0;
    }
    isOff = true;
  }
}

//if off turns on the light, called when really rough movement received
//receives really rough movement time
int reallyRoughMovement (unsigned long int time) {
  //if off, turn on
  if (  isOff && (startingTime + (unsigned long int) (millis()/1000.0) < turnOffTime + (unsigned long int) ROUGH_MOVE_TIMEOUT) && (time > turnOffTime) ) {
    digitalWrite(relay, LOW); //low to allow light on
    Serial.println("DATA 1");
    isOff = false;
  }
}

//main loop running in arduino
void loop() {
  //Serial.print("Loop");
  // analyse data only when you receive data:
  if (Serial.available() > 0) {
    //Serial.println("R");
    if (isGettingTime) {
      // read the incoming byte with time information
      incomingByte = Serial.read();
      //char oi = incomingByte;
      //Serial.println(oi);
      
      //decodes if data is time
      if (isDecodingType) {
        switch (typeOfData) {
          case 1:
            if(incomingByte == 'T')
              typeOfData++;
            break;
          case 2:
            if(incomingByte == 'i') {
              typeOfData++;
            } else {
              typeOfData = 1;
            }
            break;
          case 3:
            if(incomingByte == 'm') {
              typeOfData++;
            } else {
              typeOfData = 1;
            }
            break;
          case 4:
            typeOfData = 1;
            if(incomingByte == 'e') {
              isDecodingType = false;
              Serial.println("Receiving time");
            }
            break;
          default:
            Serial.println("Error: Invalid data type for starting time argument");
        }
      } else {
        if (incomingByte >= '0' && incomingByte <= '9') {
          timeData = timeData*10 + (unsigned long int) (incomingByte - '0');
          //Serial.println(timeData);
        //end of time number
        } else {
          startingTime = (timeData) - (unsigned long int) (millis()/1000.0);
          Serial.print("Start: ");
          Serial.println(startingTime);
          isGettingTime = false;
          timeData = 0;
          isDecodingType = true;
        }
      }
    } else {
      // read the incoming byte information
      incomingByte = Serial.read();
      //Serial.println(incomingByte);
      
      //decodes if valid data and type of data
      if (isDecodingType) {
        switch (typeOfData) {
          case 1:
            if(incomingByte == 'D')
              typeOfData++;
            break;
          case 2:
            if(incomingByte == 'a') {
              typeOfData++;
            } else {
              typeOfData = 1;
            }
            break;
          case 3:
            if(incomingByte == 't') {
              typeOfData++;
            } else {
              typeOfData = 1;
            }
            break;
          case 4:
            if(incomingByte == 'a') {
              typeOfData++;
            } else {
              typeOfData = 1;
            }
            break;
          case 5:
            isDecodingType = false;
            typeOfData = incomingByte - '0';
            Serial.println("Receiving data");
            break;
            
          default:
            Serial.println("Error: Invalid data type for data argument");
            break;
        }
      //uses data received
      } else {
        isDecodingType = true;
        switch (typeOfData) {
          //case heart beat sensor data
          case 0:
            heartBeatData(incomingByte);
            typeOfData = 1;
            break;
          //case time without movement data
          case 1:
            //compose time number (comes as string in more than one byte)
            if (incomingByte >= '0' && incomingByte <= '9') {
              timeDataLong = timeDataLong*10 + (uint64_t)(incomingByte - '0');
              isDecodingType = false;
            //end of time number
            } else {
              //convert from ms to seconds
              timeData = (unsigned long int) (timeDataLong/1000);
              timeDataLong = 0;
              //time might be unsincronized, consider "future" as 0
              if (startingTime + (unsigned long int) (millis()/1000.0) < timeData) {
                timeWithoutMovement(0);
              } else {
                timeWithoutMovement(startingTime + (unsigned long int) (millis()/1000.0) - timeData); //now - movementTime in seconds
              }
              timeData = 0;
              typeOfData = 1;
            }
            break;
          //case switch light data
          case 2:
            switchLight(incomingByte - '0');
            typeOfData = 1;
            break;
          //case really rough movement data
          case 3:
            //compose time number (comes as string in more than one byte)
            if (incomingByte >= '0' && incomingByte <= '9') {
              timeDataLong = timeDataLong*10 + (uint64_t)(incomingByte - '0');
              isDecodingType = false;
            //end of time number
            } else {
              //convert from ms to seconds
              timeData = (unsigned long int) (timeDataLong/1000);
              timeDataLong = 0;
              //time might be unsincronized, consider "future" as 0
              //if (startingTime > timeData) {
              //  timeWithoutMovement(0);
              //} else {
              //  timeWithoutMovement(startingTime + (unsigned long int) (millis()/1000.0) - timeData); //now - movementTime in seconds
              //}
              reallyRoughMovement(timeData);
              timeData = 0;
              typeOfData = 1;
            }
            break;
        }
      }
    }
  }
}

//really rough movement returns DATA 1 even when not necessary (already on again)
