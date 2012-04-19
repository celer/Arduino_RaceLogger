#include "RaceLogger.h"


#define DEBUG(msg) Serial.println(msg);

#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

#define LOG_LED_PIN 5

uint8_t * heapptr, * stackptr;
void check_mem() {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
}


RaceLogger::RaceLogger(){
  gpsStream=NULL;
  accel=NULL;
  
  running=false;
  haveSDCard=false;
  haveGPSFix=false;
  logPtr=logBuffer;
  gpsPtr=gpsBuffer;
  inputPtr=inputBuffer;
  gpsLinesCaptured=0;
  idleCount=0;
}

void RaceLogger::initGPS(){
  if(gpsStream){
    //FIXME this really should look on the SD Card for a set of GPS setup strings
    while(gpsStream->available()){ gpsStream->read(); }
    gpsStream->println(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    while(gpsStream->available()){ Serial.write(gpsStream->read());}
    gpsStream->println(PMTK_SET_NMEA_UPDATE_10HZ);
    while(gpsStream->available()){ Serial.write(gpsStream->read());}
  } else {
    DEBUG("No GPS Connected");
  }
}

void RaceLogger::initAccel(){
  if(accel){
    accel->init(ACCEL_SCALE_4,ACCEL_12_5HZ);
  } else {
    DEBUG("No Accel Connected");
  }
}

void RaceLogger::init(){
  pinMode(53,OUTPUT);
#ifdef LOG_LED_PIN
  pinMode(LOG_LED_PIN,OUTPUT);
#endif

  Serial.begin(115200);
  
  openLog(); 
  
  initGPS();
  initAccel();
  
  //Set 53 to output for SD Card
  delay(500); 
  delay(1000);
} 


void RaceLogger::openLog(){
  haveSDCard=false;
  int j=0;
  for(j=0;j<=3;j++){
    Serial.print("Using card speed: ");
    Serial.println(j);
    if(sdCard.init(j,SD_CHIP_SELECT)>0){
      if(sdVolume.init(&sdCard)>0){
        if(sdRoot.openRoot(&sdVolume)>0){
          if(logFile.open(&sdRoot,logFileName(),O_WRITE|O_CREAT)>0){
            haveSDCard=true;
            return;
          } 
        } else {
          DEBUG("Unable to open SD root file system");
        }
      } else {
        DEBUG("Unable to open SD volume");
      }
    } else {
      DEBUG("Unable to open SD card");
      haveSDCard=false;
    }
  }
  delay(1000);
}

void RaceLogger::closeLog(){
  DEBUG("Closing log");
  logFile.write(logBuffer, logPtr-logBuffer);
  logFile.sync();
  logFile.close();
}

char *gpsToken(char *where){
  char *p=where;
  while(*p!='\0' && *p!=','){
    p++;
  }
  if(*p=='\0')
    return NULL;
  return p;
}

void RaceLogger::parseGPS(char *gpsLine,uint8_t len){
  gpsLine[len]=0;
  char *line = gpsLine;
  char *start=line, *p=line; 
  char *tok;
  int i;

  if(strncmp(line,"$GPRMC",6)==0){
    for(i=0;i<=11;i++){
      tok = gpsToken(line);
      if(tok==NULL)
        break;
      if(*line!=','){
        switch(i){
          case 1:
           //printf("Timestamp %d %f\n",atoi(line),atof(line)); 
           Serial.print("Timestamp:");
           Serial.println(atof(line));
          break;
          case 2:
           Serial.print("Validity");
           Serial.println((char) *line);
           //printf("Validity %c\n",*line);
          break;
          case 3: 
            Serial.print("Latitude: ");
            Serial.println(atof(line));
           //printf("Latitude %d\n",atoi(line));
          break;
          case 4:
           //printf("Lat Dir %c\n",*line);
          break;
          case 5: 
            Serial.print("Longitude: ");
            Serial.println(atof(line));
           //printf("Longitude %d\n",atoi(line));
          break;
          case 6:
           //printf("Lon Dir %c\n",*line);
          break;
          case 7:
           //printf("Speed %d\n",atoi(line));
          break;
          case 8:
           //printf("Course %d\n",atoi(line));
          break;
          case 9:
            Serial.print("Date: ");
            Serial.println(atol(line));
           //printf("Date %d\n",atoi(line));
          break;
        }  
      }
      
      
      
      line=tok+1; 
    }
  }
  
}


void RaceLogger::loop(){

  if(Serial.available()){
    char c = Serial.read();
    if(c=='s'){
      DEBUG("Stopping logging and closing file");
      running=false;
      closeLog();
    } else if(c=='m'){
      Serial.print("Available memory:");
      check_mem();
      Serial.println(stackptr-heapptr);
    }
  }


  if(running){
    if(haveSDCard){
      //Log the accelerometer data
      if(accel && accel->available()){
        accel->read();

        //FIXME consider comparing accel readings to last set and ignoring if no change has occured
        char accelBuffer[50];
        sprintf(accelBuffer,"A %d %d %d\n",(int)(accel->accelG[0]*1000),(int)(accel->accelG[1]*1000),(int)(accel->accelG[2]*1000)); 
        logData(accelBuffer);
        accelLinesCaptured++;
      } else {
        idleCount++;
      }
      //Log GPS Data
      if(gpsStream && gpsStream->available()){
        char c = gpsStream->read();
        if(gpsPtr-gpsBuffer+1 > GPS_BUFFER_SIZE){
          gpsLinesCaptured++;
          DEBUG("GPS OVERFLOW");
          logData(gpsBuffer,gpsPtr-gpsBuffer);
          gpsPtr=gpsBuffer;       
        }
        *gpsPtr=c;
        gpsPtr++;
        if(c=='\n'){
          gpsLinesCaptured++;
          parseGPS(gpsBuffer,gpsPtr-gpsBuffer);
          logData(gpsBuffer,gpsPtr-gpsBuffer);
          gpsPtr=gpsBuffer;       
        }
      } else {
        idleCount++;
      }
    } else {
      openLog();
    }
  }
}

void RaceLogger::debug(){
  if(running){
    Serial.print("Log Buffer Used: ");
    Serial.println(logPtr-logBuffer);
    Serial.print("GPS Lines Captured: ");
    Serial.println(gpsLinesCaptured);
    Serial.print("Accel Lines Captured: ");
    Serial.println(accelLinesCaptured);
    Serial.print("Idle Count: ");
    Serial.println(idleCount);
    idleCount=0;
    gpsLinesCaptured=0;
    accelLinesCaptured=0;
  }
}


void RaceLogger::logData(char *data, int16_t len){
#ifdef LOG_LED_PIN
  digitalWrite(LOG_LED_PIN,HIGH);
#endif
  if(len==-1)
    len = strlen(data);
  if((logPtr-logBuffer)+len+1 > LOG_BUFFER_SIZE){
    DEBUG("Writing log buffer");
    logFile.write(logBuffer,logPtr-logBuffer); 
    logFile.sync(); 
    logPtr = logBuffer;
  }
  strncpy(logPtr,data,len);  
  logPtr+=len;
#ifdef LOG_LED_PIN
  digitalWrite(LOG_LED_PIN,LOW);
#endif
}
