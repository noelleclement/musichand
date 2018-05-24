/*
 * 
 * This code has been written by Noelle Clement, 
 * student Computer Science at University of Applied Sciences Rotterdam 2018
 * 
 * Parts of this code have based on the program "Accelerometer_States" from my SerIOGrapher repository
 * That repository has been forked from the original one, created by 'juancgarcia'
 * 
 * Also the I2Cdev library has been used, below the license:
 * 
 */

 /* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/





//TODO: ranges ipv alleen erboven of eronder



/*
 * uno: 0x68
 * 
 * 
 * 
 */


//==================================== globals ==================================

//////////// MPU6050 config //////////////

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define USE_ACCEL 3
// #define USE_GYRO 3

#if defined(USE_ACCEL) && defined(USE_GYRO)
const int numAxis = USE_ACCEL + USE_GYRO;
const int AX = 0;
const int AY = 1;
const int AZ = 2;
const int GX = 3;
const int GY = 4;
const int GZ = 5;
#elif defined(USE_ACCEL)
const int numAxis = USE_ACCEL;
const int AX = 0;
const int AY = 1;
const int AZ = 2;
#elif defined(USE_GYRO)
const int numAxis = USE_GYRO;
const int GX = 0;
const int GY = 1;
const int GZ = 2;
#endif




////////////// DEBUG /////////////

// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
//#define OUTPUT_READABLE_ACCELGYRO

//#define DEBUG

//TODO: check whether blinkstate etc wont give errors if debug is uncommented
#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define LED_PIN 13
 boolean blinkState = false;
 boolean glowing = false;
 uint32_t glowEnd = -1;
 const uint32_t glowDuration = 2500;
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
 #define LED_PIN
 #define glowing
#endif



/////////// READINGS ///////////

uint16_t lastReport; 
const int numReadings = 25;   

int32_t readings[numAxis][numReadings];  // the reading history
int32_t readIndex[numAxis];              // the index of the current reading
int32_t total[numAxis];                  // the running total
int32_t average[numAxis];                // the average
    


/////////// FUNCTIONS /////////
//TODO: see if possible to make this more general, so I don't have to write this over and over for every function

boolean flat = false;
uint32_t flatStarted = 0;
uint32_t flatDuration = 0;
uint32_t flatLastEnded = 0;

boolean vertical = false;
uint32_t verticalStarted = 0;
uint32_t verticalDuration = 0;
uint32_t verticalLastEnded = 0;

boolean forward = false;
uint32_t forwardStarted = 0;
uint32_t forwardDuration = 0;
uint32_t forwardLastEnded = 0;
boolean wasBack = false;

boolean wasItLifted = false;




//==================================== setup ==================================

void setup() {
  
    lastReport = millis();        //so there's a 'starting value' to calculate with

    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(38400);

    // initialize device
    DEBUG_PRINTLN("Initializing I2C devices...");
    accelgyro.initialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    //TODO: check whether offsets are good for own mpu
    accelgyro.setXGyroOffset(-1100);
    accelgyro.setYGyroOffset(271);
    accelgyro.setZGyroOffset(-60);
    accelgyro.setXAccelOffset(-2509);
    accelgyro.setYAccelOffset(-101);
    accelgyro.setZAccelOffset(925); // 1688 factory default for my test chip

    // verify connection
    DEBUG_PRINTLN("Testing device connections...");
    DEBUG_PRINTLN(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    //TODO check whether can be done differently?
    #ifdef DEBUG
      // configure Arduino LED for
      pinMode(LED_PIN, OUTPUT);
    #endif

    // zero-fill all the arrays:
    for (int axis = 0; axis < numAxis; axis++) {
        readIndex[axis] = 0;
        total[axis] = 0;
        average[axis] = 0;
        for (int i = 0; i<numReadings; i++){
            readings[axis][i] = 0;
        }
    }

    
}

//==================================== loop ==================================
void loop() {
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    #ifdef USE_ACCEL
        smooth(AX, ax);
        smooth(AY, ay);
        smooth(AZ, az);
    #endif

    #ifdef USE_GYRO
        smooth(GX, gx);
        smooth(GY, gy);
        smooth(GZ, gz);
    #endif

    //Serial.println("I'm here!");

    #ifdef OUTPUT_READABLE_ACCELGYRO
        reportAccelGyro();
    #endif

    checkStates();
    
    reportStates();
    handleLights();
}



//==================================== state functions ==================================

void checkStates(){
    //TODO: add handlers for states that go against each other
    //TODO: define lifted functions/states for debugging, and forward as standard > make standard for other movements too

    /////////DEBUGGING/////// 
    
    #ifdef DEBUG
      checkFlat();
      checkVertical();
    
      wasItLifted = wasLifted();      //if put directly in the if statement, it doesnt work
      DEBUG_PRINT(wasItLifted);
      if(wasItLifted){
         glowing = true;
      }
      else{
        glowing = false;
      }
    #endif


    ///////// ACTUAL READINGS /////// 
    
    checkForward();

    //TODO: works instantly now, but add handler somewhere for how often it will be communicated to the app
    if(forward &&
        wasBack
        ){
      Serial.println("forward");
      #ifdef DEBUG
        glowing = true;               
      #endif
    }


}

void checkFlat(){
    #ifdef USE_ACCEL
      if(abs(average[AY]) < 8100 && average[AZ] > 1000 ){ //abs(average[AZ]) > 1000 && abs(average[AX]) < -8001 ){
          if(!flat){
              flatStarted = millis();
          }
          flatLastEnded = millis();
              
          flatDuration = millis() - flatStarted;
          
          flat = true;
      } else {
          flat = false;
      }
    #endif
}


void checkVertical(){
    #ifdef USE_ACCEL
    boolean AX_in_range = average[AX] > -8000;      //if done with abs the negative isnt recognized, so above 0 will be the same as beneath 0
    boolean AY_in_range = abs(average[AY]) > 1500;
    boolean AZ_in_range = average[AZ] < 1001;       //same as with x
    
    if( AZ_in_range && AX_in_range && AY_in_range ){
        if(!vertical){
            verticalStarted = millis();
        }
        verticalLastEnded = millis();
            
        verticalDuration = millis() - verticalStarted;
        
        vertical = true;
    } else {
        vertical = false;
    }
    #endif
}


void checkForward(){         
   /*
    should we want vertical placement:
    boolean AX_in_range = average[AX] > -9000;
    boolean AY_in_range = average[AY] > -1000;
    boolean AZ_in_range = average[AZ] > -5000;
    */

    
   #ifdef USE_ACCEL
    boolean AX_in_range = average[AX] < -3000;
    boolean AY_in_range = average[AY] > 12000;       
    //boolean AZ_in_range = average[AZ] < 1001;     //TODO: check whether z changes when put in unwanted position, so can be used to handle wrong motions
    
    if( AX_in_range && AY_in_range){
        if(!forward){
            forwardStarted = millis();
            wasBack = true;
        }
        else{
           wasBack = false;    
        }
        forwardLastEnded = millis();
            
        forwardDuration = millis() - forwardStarted;
        
        forward = true;
       
    } else {
        forward = false;
    }
    
    #endif

}



boolean wasLifted(){
  
    if(vertical && 
        verticalDuration > 50 && 
        //millis() - flatLastEnded < 1000 &&          //if you want the led to glow up for max 1 sec uncomment this
        flatDuration > 250  && 
        flatDuration < 2500 
        ){
        return true;
    }

    return false;
}


void smooth(int axis, int32_t val) {
    // pop and subtract the last reading:
    total[axis] -= readings[axis][readIndex[axis]];
    total[axis] += val;

    // add value to running total
    readings[axis][readIndex[axis]] = val;
    readIndex[axis]++;

    if(readIndex[axis] >= numReadings)
        readIndex[axis] = 0;

    // calculate the average:
    average[axis] = total[axis] / numReadings;
}



//==================================== debug functions ==================================


void reportStates(){

    DEBUG_PRINT("flat: ");
    DEBUG_PRINT(flat);
    DEBUG_PRINT(" duration: ");
    DEBUG_PRINT(flatDuration);
    DEBUG_PRINT(" since last: ");
    DEBUG_PRINT(millis() - flatLastEnded);

    DEBUG_PRINT(" | vertical: ");
    DEBUG_PRINT(vertical);
    DEBUG_PRINT(" duration: ");
    DEBUG_PRINT(verticalDuration);
    DEBUG_PRINT(" since last: ");
    DEBUG_PRINT(millis() - verticalLastEnded);

    DEBUG_PRINT(" Glowing?: ");
    DEBUG_PRINTLN(glowing);

}


// display tab-separated accel/gyro x/y/z values
void reportAccelGyro(){

    #ifdef USE_ACCEL
        DEBUG_PRINT(average[AX]);
        DEBUG_PRINT("\t");

        DEBUG_PRINT(average[AY]);
        DEBUG_PRINT("\t");

        DEBUG_PRINT(average[AZ]);
        DEBUG_PRINT("\t");
    #endif

    #ifdef USE_GYRO
        DEBUG_PRINT(average[GX]);
        DEBUG_PRINT("\t");

        DEBUG_PRINT(average[GY]);
        DEBUG_PRINT("\t");

        DEBUG_PRINT(average[GZ]);
        DEBUG_PRINT("\t");
    #endif

        DEBUG_PRINT(millis());
        DEBUG_PRINT("\t");

        DEBUG_PRINTLN(millis() - lastReport);
        lastReport = millis();
}

void handleLights(){
    #ifdef DEBUG
      if(glowing){
          
          DEBUG_PRINTLN("GLOWING!");
          if(glowEnd == -1)
              glowEnd = millis() + glowDuration;
  
          if(millis() > glowEnd)
              glowing = false;
          
          // run glow function here
          blinkState =  true;   //was at first !blinkState, but that doesnt work properly;
      }
      if(!glowing){
          // do some variable cleanup
          glowEnd = -1;
          blinkState = false;
      }
      digitalWrite(LED_PIN, blinkState);
     #endif
}


