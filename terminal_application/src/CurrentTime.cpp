#include "CurrentTime.h"
#include <time.h>

int HH_Current,MM_Current,SS_Current;

void getCurrentTime(void){
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  HH_Current = timeinfo ->tm_hour;
  MM_Current = timeinfo ->tm_min;
  SS_Current = timeinfo ->tm_sec;
}

int getHours(void){
    getCurrentTime();
    return HH_Current;
}

int getMins(void){
    return MM_Current;
}

int getSecs(void){
    return SS_Current;
}
