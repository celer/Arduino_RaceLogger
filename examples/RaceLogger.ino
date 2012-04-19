#include <Accel.h>
#include <SD.h>
#include <RaceLogger.h>

RaceLogger logger;
Accel accel;



void setup(){
  Serial1.begin(9600);
  logger.setGPSSerial(&Serial1);
  logger.setAccel(&accel);
  logger.init();
  logger.start();
}

uint32_t lastDebug=millis();
void loop(){
  logger.loop();
  uint32_t now = millis();
  if(now-lastDebug > 2000){
    logger.debug();
    lastDebug=now;
  }  
  
}

