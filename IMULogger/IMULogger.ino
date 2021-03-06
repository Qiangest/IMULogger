////////////////////////////////////////////////////////////////////////////
//
//  This file is part of RTIMULib-Arduino
//
//  Copyright (c) 2014-2015, richards-tech
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <Wire.h>
#include "I2Cdev.h"
#include "RTIMUSettings.h"
#include "RTIMU.h"
#include "RTFusionRTQF.h" 
#include "CalLib.h"
#include <EEPROM.h>
#include <TimeLib.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

RTIMU *imu;                                           // the IMU object
RTFusionRTQF fusion;                                  // the fusion object
RTIMUSettings settings;                               // the settings object
int command;
//  DISPLAY_INTERVAL sets the rate at which results are displayed

#define DISPLAY_INTERVAL  1000                       // interval between pose displays

//  SERIAL_PORT_SPEED defines the speed to use for the debug serial port

#define  SERIAL_PORT_SPEED  115200

unsigned long lastDisplay;
unsigned long lastRate;
int sampleCount;
RTQuaternion gravity;

void setup()
{
  int errcode;
  
  Serial.begin(SERIAL_PORT_SPEED);
  Wire.begin();
  imu = RTIMU::createIMU(&settings);                        // create the imu object
  
  Serial.print("ArduinoIMU starting using device "); Serial.println(imu->IMUName());
  if ((errcode = imu->IMUInit()) < 0) {
    Serial.print("Failed to init IMU: "); Serial.println(errcode);
  }
  
  if (imu->getCalibrationValid())
    Serial.println("Using compass calibration");
  else
    Serial.println("No valid compass calibration data");

 // lastDisplay = lastRate = millis();
  sampleCount = 0;
  
  gravity.setScalar(0);
  gravity.setX(0);
  gravity.setY(0);
  gravity.setZ(1);

  command = '1';
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message....");
}

void loop()
{  
  if (command=='1')  {
  unsigned long now = millis();
  unsigned long delta;
  RTVector3 realAccel;
  RTQuaternion rotatedGravity;
  RTQuaternion fusedConjugate;
  RTQuaternion qTemp;
  int loopCount = 0;
  
 
  while (imu->IMURead()) {                              // get the latest data if ready yet
    // this flushes remaining data in case we are falling behind
     if (++loopCount >= 10)
      continue;
    fusion.newIMUData(imu->getGyro(), imu->getAccel(), imu->getCompass(), imu->getTimestamp());
    
    //  do gravity rotation and subtraction
    
    // create the conjugate of the pose
    
    fusedConjugate = fusion.getFusionQPose().conjugate();
    
    // now do the rotation - takes two steps with qTemp as the intermediate variable
    
    qTemp = gravity * fusion.getFusionQPose();
    rotatedGravity = fusedConjugate * qTemp;
    
    // now adjust the measured accel and change the signs to make sense
    
    realAccel.setX(-(imu->getAccel().x() - rotatedGravity.x()));
    realAccel.setY(-(imu->getAccel().y() - rotatedGravity.y()));
    realAccel.setZ(-(imu->getAccel().z() - rotatedGravity.z()));
    
    sampleCount++;
    if ((delta = now - lastRate) >= 1000) {
      Serial.print("Sample rate: "); Serial.print(sampleCount);
      if (!imu->IMUGyroBiasValid())
        Serial.println(", calculating gyro bias");
      else
        Serial.println();
        
      sampleCount = 0;
      lastRate = now;
    }
    if ((now - lastDisplay) >= DISPLAY_INTERVAL) {
      lastDisplay = now;
       Serial.println();
       Serial.println("START");
       if (timeStatus()!= timeNotSet) {
        digitalClockDisplay();  
       }else
       {Serial.println("timeUnvalid");}
       
      RTMath::display("Accel:", realAccel);
      Serial.println();
      RTMath::display("Gyro:", (RTVector3&)imu->getGyro());                // gyro data
      Serial.println();
      RTMath::display("Mag:", (RTVector3&)imu->getCompass());              // compass data
      Serial.println();
      RTMath::display("Gravity:",rotatedGravity);
      Serial.println();
      RTMath::displayRollPitchYaw("Pose:", (RTVector3&)fusion.getFusionPose()); // fused output
      Serial.println();
      Serial.println("END");
      Serial.println();
    }
  }
}
}


void serialEvent(){
   if (Serial.available()) 
  // enter '1' to start sensing, or enter '0 ' to stop.
  // you can also use this interface to control arduino using PC/phone.
//  {command = Serial.read();
//  if (command == '1')
//  {lastDisplay = lastRate = millis();}
//  }
   processSyncMessage();
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // 2018.05.14

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
         setTime(pctime); // Sync Arduino clock to the time received on the serial port 
     }
     setup();
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(year()); 
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(day());
   Serial.print(" ");
  Serial.print((hour()+8)%24);
  printDigits(minute());
  printDigits(second());
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

