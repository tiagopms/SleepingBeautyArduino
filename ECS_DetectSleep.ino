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
   MAX_HEART_MEASURE_TIME >= 1000
   MIN_HEART_MEASURE_TIME < 500
   MAX_HEART_RATE > 80
   NUMBER_OF_MEASURES > 5
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
   04/29/2013, Tiago Pimentel
     Add heart rate device reading functions;
   04/30/2013, Tiago Pimentel
     Adjusted heart rate device reading function not to work inside interrupt;
     Adjusted current heart rate to be dynamic and not calculated on new measures every time;
*/
#include <Time.h>  

//constants definitions
//reading heart rate device data constants
#define MAX_HEART_MEASURE_TIME 2000
#define MIN_HEART_MEASURE_TIME 200
#define MAX_HEART_RATE 120
#define NUMBER_OF_MEASURES 25
//applying heart rate measurements constants
#define LEARNING_MEASURES 5
#define APPLYING_MEASURES 10
//cellphone accelerometer constants
#define STILL_TIME 100
#define ROUGH_MOVE_TIMEOUT 60

//variables related to serial incoming data declaration
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

//give a name to relay pin and sets a variable with its state
#define RELAY 4
int isOff = false;

///reading heart rate device constants and variables
//gives a name to the heart beat led, the heart measurement error led, and the bounce measure led
#define HEART_LED 8
#define HEART_ERROR_LED 6
#define HEART_BOUNCED_ERROR_LED 7
//array with times from last heart beats
unsigned long timeArray[NUMBER_OF_MEASURES + 1];
//variable that saves if current measurement state has no error
bool noDataErrors = true;
//counter that saves current number of heart beats
unsigned char counter;
//catches heart beat interrupt to be ran as normal code
volatile boolean isGetHeartBeat = false;

//applying heart rate's data variables
//constant related to percentage of chance to be asleep, changes velocity of heart rate lowering response
float heartRatePercentConstant = 0.85;
//array with current heart rate data
unsigned long currentHeartRateArray[APPLYING_MEASURES + 1];
//counters of number of measurements
float timesCalled = 0;
int currentCounter = 0;
bool isReadyToUseData = false;

//arduino setup
void setup(){
  // initialize the relay, heart rate led and heart rate measurement errors pins as outputs
  pinMode(RELAY, OUTPUT);
  pinMode(HEART_LED, OUTPUT);
  pinMode(HEART_ERROR_LED, OUTPUT);
  pinMode(HEART_BOUNCED_ERROR_LED, OUTPUT);
  //start serial reading and writing
  Serial.begin(9600);
  
  //sends system starting message and waits for 5 seconds for chest strap adjusting
  Serial.println("Starting system");
  delay(5000);
  Serial.println("System started");
  
  //inits time array used to get heart rate in bpm
  initTimeArray();
  
  //set interrupt 0, in digital port 2, to handle heart rate beat data income
  attachInterrupt(0, allowGetHeartBeat, RISING);
}

///analyses heart beat data from sensor
// interrupt function that allows getting one heart beat time by main loop
void allowGetHeartBeat() {
  isGetHeartBeat = true;
}

//initialize array used for heart beats times storing
void initTimeArray() {
  for(unsigned char i=0; i < NUMBER_OF_MEASURES; i++) {
    timeArray[i]=0;
  }
  timeArray[NUMBER_OF_MEASURES]=millis();
}

//message that gives error message, restarts heart beat data aquirement and sets error led to HIGH 
void errorHeartBeat() {
  noDataErrors = false;
  counter=0;
  Serial.println("Heart rate measure error, test will restart!" );
  initTimeArray();
  digitalWrite(HEART_ERROR_LED, HIGH);
}

//calculates heart beat rate based on last NUMBER_OF_MEASURES heart beats times
void calculateHeartRate() {
  //variable with heart rate in bpm
  unsigned int heartRate;
  
  //if no errors in the data , calculate heart rate
  if(noDataErrors) {
    //gets heart rate in bpm (beats per minute 60*20*1000/20_total_time)
    heartRate = 1200000/(timeArray[NUMBER_OF_MEASURES]-timeArray[0]);
    
    //prints data in serial so laptop sends to internet
    Serial.print("DATA 2 ");
    Serial.println(heartRate);
    
    //if heart rate not over a maximum threshold lowers state of error led and use this data
    if (heartRate > MAX_HEART_RATE) {
      errorHeartBeat();
    } else {
      digitalWrite(HEART_ERROR_LED, LOW);
      heartBeatData(heartRate);
    }
  }
  //if there are errors, reinitializes them
  noDataErrors = true;
}

//receives one heart rate sensor beat and saves its time to calculate heart rate in bpm
void getHeartBeat() {
  //variable with time between current beat and last
  unsigned long measureTime;
  //variable with heart beat led state
  static boolean heartLed = false;
  //saves bounce error, so if two concecutive errors reset measure
  static boolean bouncedSignal = false;
  
  //gets current time to time array
  timeArray[counter] = millis();
  
  //prints in serial heart beats sensor counter
  Serial.print(F("Heart counter:\t"));
  Serial.println(counter,DEC);
  
  //sets heart led state and changes it for next run so light changes in heart frequency
  digitalWrite(HEART_LED, heartLed);
  if(heartLed) {
    heartLed = false;
  } else {
    heartLed = true;
  }
  
  //if first measure, compare time to last measure in array, else with last before this one
  switch(counter) {
    case 0:
      measureTime = timeArray[counter]-timeArray[NUMBER_OF_MEASURES];
      break;
    default:
      measureTime = timeArray[counter]-timeArray[counter-1];
      break;
  }
  
  //sets MAX_HEART_MEASURE_TIME/1000 seconds as max time for single heart beat and 1.5*MIN_HEART_MEASURE_TIME/1000 as min time for 3 consecutive heart beats or resets data aquiring
  if(measureTime > MAX_HEART_MEASURE_TIME || (measureTime < 1.5*MIN_HEART_MEASURE_TIME && bouncedSignal)) {
    //sign error and reset current measures
    errorHeartBeat();
  }
  
  //sets MIN_HEART_MEASURE_TIME/1000 seconds as min time for 2 consecutive heart beats or sets bounce measure as on
  if (measureTime < MIN_HEART_MEASURE_TIME && !bouncedSignal) {
    Serial.println("Heart rate bounced measure error!" );
    digitalWrite(HEART_BOUNCED_ERROR_LED, HIGH);
    bouncedSignal = true;
  } else {
    //sets bounce data as false and lowers bounce error led state
    digitalWrite(HEART_BOUNCED_ERROR_LED, LOW);
    bouncedSignal = false;
    
    //if last measure and no errors calculates heart beating rate
    if (counter == NUMBER_OF_MEASURES && noDataErrors) {
      counter = 0;
      calculateHeartRate();
    //if not last measure and no errors raise counter
    } else if(counter != NUMBER_OF_MEASURES && noDataErrors) {
      counter++;
    //if error resets counter and erases error
    } else {
      counter = 0;
      noDataErrors = true;
    }
  }
}

//uses heart rate data aquired
//resets learned measures when light is turned on again
float resetLearnedHeartRate() {
  timesCalled = 0;
  currentCounter = 0;
  isReadyToUseData = false;
}

//calculates current heart beat rate using a mean from all measures in currentHeartRateArray
float calculateCurrentHeartRate() {
  float localCurrentHeartRate = 0;
  
  for(unsigned char i=0; i <= APPLYING_MEASURES; i++) {
    localCurrentHeartRate += currentHeartRateArray[i]/(APPLYING_MEASURES*1.0 + 1.0);
  }
  
  return localCurrentHeartRate;
}

//learns heart rate mean from person and them uses current mean to if smaller than 90% of mean, turn off light
void heartBeatData (unsigned long heartRate) {
  //if light off, no need to run code
  if(isOff) {
    return;
  }
  
  //current and learned heart rates
  static float heartRatesMean = 0;
  float currentHeartRate = 0;
  //update number of times data received
  timesCalled++;
  
  //learning heart rate mean
  if (timesCalled <= LEARNING_MEASURES) {
    if(timesCalled == 1) {
      heartRatesMean = 1;
    }
    heartRatesMean = heartRatesMean*(timesCalled - 1)/timesCalled + heartRate/timesCalled;
    Serial.print("Heart Rates: ");
    Serial.println(heartRatesMean);
  }
  //applying heart rate mean measured
  else {
    //increase current counter
    currentCounter++;
    //getting current heart rates
    currentHeartRateArray[currentCounter] = heartRate;
    
    //if last measure, go back to start and sets heart rate as ready
    if (currentCounter == APPLYING_MEASURES) {
      isReadyToUseData = true;
      currentCounter = 0;
    }
    
    //check if new mean is smaller than 90% of original
    if(isReadyToUseData) {
      //getting current heart rates mean
      currentHeartRate = calculateCurrentHeartRate();
      Serial.print("Current Heart Rate: ");
      Serial.println(currentHeartRate);
      //if smaller than heartRatePercentConstant% of original heart rate, turn off light 
      if (currentHeartRate < heartRatesMean * heartRatePercentConstant) {
        digitalWrite(RELAY, HIGH);
        isOff = true;
        Serial.println("DATA 0");
        turnOffTime = startingTime + (unsigned long int) (millis()/1000.0);
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
    Serial.println("Large time without movement!");
    //since no heart rate changed to just turn off light
    heartRatePercentConstant = 0.93;
  }
}

//turns on or off the light
//receives a int indicating if it should turn the light on or off
int switchLight (int turnOn) {
  Serial.print("Switch light received: ");
  Serial.println(turnOn);
  if(turnOn) {
    digitalWrite(RELAY, LOW); //low to allow light on
    isOff = false;
    resetLearnedHeartRate();
  } else {
    digitalWrite(RELAY, HIGH); //high to turn off light
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
    digitalWrite(RELAY, LOW); //low to allow light on
    Serial.println("DATA 1");
    isOff = false;
    resetLearnedHeartRate();
  }
}

//main loop running in arduino
void loop() {
  //Serial.println("Loop");
  delay(10);
  
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
          default:
            Serial.println("Error: Invalid data, not a valid type");
            typeOfData = 1;
            break;
        }
      }
    }
  }
  
  //if interrupt with heart beat, gets heart beat time and uses it
  if(isGetHeartBeat) {
    getHeartBeat();
    isGetHeartBeat = false;
  }
}

//really rough movement returns DATA 1 even when not necessary (already on again)
