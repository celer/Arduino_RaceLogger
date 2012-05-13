#ifndef RACELOGGER_H
#define RACELOGGER_H

#define MEGA_SOFT_SPI 1

#include <Arduino.h>
#include <SD.h>
#include <Accel.h>

#define INPUT_BUFFER_SIZE 15
#define GPS_BUFFER_SIZE 140
#define LOG_BUFFER_SIZE 1024

#ifndef SD_CHIP_SELECT 
  #define SD_CHIP_SELECT 10
#endif


class RaceLogger {
public: 
  RaceLogger();

  void init(); 
  void initGPS();
  void initAccel();

  void setRPMInterrupt(uint8_t intNumber);
  
  void setGPSSerial(Stream *gpsStream){
    this->gpsStream = gpsStream;
  }
  void setAccel(Accel *accel){
    this->accel = accel;
  }

  void openLog();
  void closeLog();

  void debug();

  void logData(char *data,int16_t len=-1);
  
  void start(){ running = true; }
  void stop(){ running = false; }
  void sync(){ logFile.sync(); }

  void tagLapMarker();
  void tagSplitMarker();

  char *logFileName(){ return "gps.log"; }

  void parseGPS(char *gpsLine, uint8_t len);

  void loop();

  bool running;
  bool haveSDCard;
  bool haveGPSFix;

  Stream *gpsStream;
  Accel *accel;
  Sd2Card sdCard;
  SdFile sdRoot;
  SdVolume sdVolume;
  SdFile logFile;

  uint8_t ignPerRev;
  uint32_t gpsLinesCaptured;
  uint32_t accelLinesCaptured;
  uint32_t idleCount;


  char inputBuffer[INPUT_BUFFER_SIZE];
  char *inputPtr;
  char gpsBuffer[GPS_BUFFER_SIZE];
  char *gpsPtr;
  char logBuffer[LOG_BUFFER_SIZE];
  char *logPtr;

};

#endif
