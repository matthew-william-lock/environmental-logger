/*
 * Logger.c
 * Remote environmental logging using raspberry pi
 * 
 * <LCKMAT002><	MLTIVI001>
 * 28 September 2019
*/

/**
 * @file       main.cpp
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 */
 

//#define BLYNK_DEBUG
#define BLYNK_PRINT stdout
#ifdef RASPBERRY
  #include <BlynkApiWiringPi.h>
#else
  #include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);

static const char *auth, *serv;
static uint16_t port;

#include <BlynkWidgets.h>
#include <iostream>
#include <fstream>
using namespace std;

#include "main.h"
#include "CurrentTime.h"

WidgetTerminal terminal(V0);
WidgetLED led1(V3);

//Global variables
int hours, mins, secs;
int RTC; //Holds the RTC instance
unsigned int sampleIntervalIndex = 0;
int HH,MM,SS;

float humidity;
int light, temperature;

long lastInterruptTime = 0; //Used for button debounce

int sysHours,sysMin,sysSec;
int alarmHours,alarmMin,alarmSec = 0; //last alarm trigger

bool alarmActive = false;
bool alarmTriggered=false; // Initial state
int sampleInterval[3]={1,2,5};
int start=0;

/*
 * startStop
 * Start or stop montering
 */
void startStop(void){
    //Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("\nStart button pressed!\n");
        if(start==0){
            start=1;
            printf("RTC Time\tSys Time\tHumidity\tTemp\tLight\tDac out\tAlarm\n");
            char buffer [80];
            int n=sprintf (buffer,"RTC Time\tSys Time\tHumidity\tTemp\tLight\tDac out\tAlarm");
            Blynk.virtualWrite(0,buffer);

        }
        else start=0;

	}
	lastInterruptTime = interruptTime;
}

/*
 * stopAlarm
 * Stop the alarm if active
 * Software Debouncing used
 */
void stopAlarm(void){
    //Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("\nAlarm button pressed!\n");
        if (alarmActive) {
            alarmActive=!alarmActive;
            led1.off();
        }

	}
	lastInterruptTime = interruptTime;
}


/*
 * changeInterval
 * Change the interval for which measurements are made
 * Software Debouncing used
 */
void changeInterval(void){
    //Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("\nInerval Changed!\n");
        sampleIntervalIndex++;
        if (sampleIntervalIndex>(sizeof(sampleInterval)/sizeof(sampleInterval[0])-1)) {
            sampleIntervalIndex=0;
        }

	}
	lastInterruptTime = interruptTime;
}

//Buzzer Test
void buzz(int pin){
    printf("Buzzer test\n");
    softToneWrite (pin,2200) ;
    usleep(400*100);
    softToneWrite (pin,0) ;
    usleep(400*100);
    printf("---BUZZER TEST COMPLETE---\n");
}

/*
* Thread to handle alarm activation
*/
void *sound_alarm(void *threadargs){
    printf("Starting alarm thread\n");
    fflush(stdout); // Make sure printf works
    for (;;){
        //Check for alarm activation
        fflush(stdout); // Make sure printf works
        while (alarmActive){
            softToneWrite (BUZZER,2200) ;
            usleep(400*100);
            softToneWrite (BUZZER,0) ;
            usleep(400*100);
        } 
    }
    pthread_exit(NULL);
}

/*
* Thread to sensor sampling
*/
void *sample_sensors(void *threadargs){
    printf("Starting sensor thread\n");
    fflush(stdout); // Make sure printf works
    for (;;){
        while(start!=1){
            sleep(1);
            //wait
        }
        
        humidity = sampleHumidity()/(float)1023 *3.3;
        temperature=sampleTemperature();
        light=sampleLight();


        //  Get time
        hours= getHoursRTC() ;
        mins= getMinsRCTC() ;
        secs= getSecsRTC();

        //Conversions
        float dac_output = light/(float)1023 * humidity;
        float environmental_temperature= ((temperature*3.3/1024)-V_0)/Tc;
        //float environmental_temperature = temperature;

        //printf("Min since last activation %d Hours since last activation %d\n",getAlarmMinutes(),getAlarmHours()); //Test
        
        //Calculates dac output and triggers alarm if within thresh hold
        if ((dac_output<0.65 || dac_output>2.65)&& (getAlarmMinutes()>2 || getAlarmHours()>0 || alarmTriggered==false)){
            alarmActive=true;
            led1.on();
            alarmTriggered=true;
            alarmHours=getSystemRunHours();
            alarmMin=getSystemRunMin();
            alarmSec=getSystemRunSec();
        }
        char alarm;
        if (alarmActive){
            alarm = '*';
        } 
        else alarm = ' ';

        char buffer [80];
        int n=sprintf (buffer,"%02d:%02d:%02d\t%02d:%02d:%02d\t%1.2f V\t\t%2.2f C\t%4d\t%1.2fV\t%c",hours, mins, secs,getSystemRunHours(), getSystemRunMin(), getSystemRunSec(),humidity,environmental_temperature,light,dac_output,alarm);

        //printf("%02d:%02d:%02d\t%02d:%02d:%02d\t%1.2f V\t%1.2f C\t%4d\t%1.2fV\t%c",hours, mins, secs,getSystemRunHours(), getSystemRunMin(), getSystemRunSec(),humidity,environmental_temperature,light,dac_output,alarm);
        //Blynk.virtualWrite(V0, hours+":"+ mins +";"+secs+"\t"+getSystemRunHours()+":"+getSystemRunMin()+":"+getSystemRunSec+"\t"+humidity+"\t"+environmental_temperature+" C\t"+light+"\t"+dac_output+"\t"+dac_output+"\t"+alarm+"\n");
        printf("%s",buffer);
        printf("\n"); 
        Blynk.virtualWrite(0,buffer);
        Blynk.virtualWrite(1,environmental_temperature);
        Blynk.virtualWrite(2,humidity);
        Blynk.virtualWrite(4,light);
        

        fflush(stdout); // Make sure printf works
        sleep(sampleInterval[sampleIntervalIndex]);
        
        
    }    
    pthread_exit(NULL);    
}    

/*
* Sample humidity using ADC
*/
int sampleHumidity(void){
    int x = analogRead (BASE + humidityPin) ;
    return x;
}

/*
* Sample light using ADC
*/
int sampleLight(void){
    int x = analogRead (BASE + lightPin) ;
    return x;
}

/*
* Sample temp
*/

int sampleTemperature(void){
    int x = analogRead (BASE + tempPin) ;
    return x;
}

/*
* Sample voltage output
*/

int sampleVoltage(void){
    int x = analogRead (BASE + voltageOutputPin) ;
    return x;
}

int setVoltage(int voltage){
    //Set control and data bits
    unsigned char dacBuffer[2];
    dacBuffer[0]=48 | (voltage>>6);
    dacBuffer[1]=252 & (voltage<<2);
    //Send buffer
    return wiringPiSPIDataRW (SPI_CHAN_DAC, dacBuffer, 2);
}

/*
 * Performs bitwise operations to extract BCD
 */

int bcdConverter(int BCD){
	int ones = BCD & 0b00001111;
	int tens = ((BCD & (0b01110000))>>4);
	return ones + tens*10;
}

int getHoursRTC(){
	int hours= bcdConverter(wiringPiI2CReadReg8 (RTC, HOUR)) ;
	return hours;
}

int getMinsRCTC(){
	int mins= bcdConverter(wiringPiI2CReadReg8 (RTC, MIN)) ;
	return mins;

}

int getSecsRTC(){
	int secs= bcdConverter(wiringPiI2CReadReg8 (RTC, SEC)) ;
	return secs;
}

/*
 * Reset the system time
 */
void resetTime(void){
        //Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("\nTime reset!\n");
        sysHours=getHoursRTC();
        sysMin=getMinsRCTC();
        sysSec=getSecsRTC();
        alarmTriggered=false;
        //clear console
        system("clear");
        terminal.clear();

	}
	lastInterruptTime = interruptTime;
}

/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}
//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	HH = getHours()+TIMEZONE; //Timezone compensation
	MM = getMins();
	SS = getSecs();

    sysHours = HH;
    sysMin=MM;
    sysSec=SS;

	//HH = hFormat(HH);
	HH = decCompensation(HH);
	wiringPiI2CWriteReg8(RTC, HOUR, HH);

	MM = decCompensation(MM);
	wiringPiI2CWriteReg8(RTC, MIN, MM);

	SS = decCompensation(SS);
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

    if(sysHours==24) sysHours=0;
}

/*
* getSystemRunHours
* Determines the amount of hours the system has been running
*/

int getSystemRunHours(void){
    //Get time
    hours= getHoursRTC();
    mins= getMinsRCTC() ;
    secs= getSecsRTC() ;

    int RTCtimeSeconds=hours*60*60+mins*60+secs;
    int sysTimeSeconds = sysHours*60*60 + sysMin*60 + sysSec;
    return (RTCtimeSeconds-sysTimeSeconds)/(60*60);
}

/*
* getSystemRunMin
* Determines the amount of minutes the system has been running
*/

int getSystemRunMin(void){
   //Get time
    hours= getHoursRTC();
    mins= getMinsRCTC() ;
    secs= getSecsRTC() ;

    int RTCtimeSeconds=hours*60*60+mins*60+secs;
    int sysTimeSeconds = sysHours*60*60 + sysMin*60 + sysSec;
    return ((RTCtimeSeconds-sysTimeSeconds-getSystemRunHours()*60*60)/(60));
}

/*
* getSystemRunSec
* Determines the amount of seconds the system has been running
*/

int getSystemRunSec(void){
    //Get time
    hours= getHoursRTC();
    mins= getMinsRCTC() ;
    secs= getSecsRTC() ;

    int RTCtimeSeconds=hours*60*60+mins*60+secs;
    int sysTimeSeconds = sysHours*60*60 + sysMin*60 + sysSec;
    return ((RTCtimeSeconds-sysTimeSeconds-getSystemRunHours()*60*60)%(60));
}

/*
* getAlarmHours
* Determines the hours since last alarm activation
*/

int getAlarmHours(void){    
    int sysTimeSeconds=getSystemRunHours()*60*60+getSystemRunMin()*60+getSystemRunSec();
    int alarmTimeSecods = alarmHours*60*60 + alarmMin*60 + alarmSec;
    return (sysTimeSeconds-alarmTimeSecods)/(60*60);
}

/*
* getAlarmMinutes
* Determines the minutes since last alarm activation
*/

int getAlarmMinutes(void){    
    int sysTimeSeconds=getSystemRunHours()*60*60+getSystemRunMin()*60+getSystemRunSec();
    int alarmTimeSecods = alarmHours*60*60 + alarmMin*60 + alarmSec;
    return ((sysTimeSeconds-alarmTimeSecods-((sysTimeSeconds-alarmTimeSecods)/(60*60))*60*60)/(60));
}

void setup()
{
    Blynk.begin(auth, serv, port);

    mcp3004Setup (BASE, SPI_CHAN_ADC) ; // Init ADC
    RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
    softToneCreate(BUZZER); //Init BUZZER pin

    wiringPiI2CWriteReg8(RTC, SEC, 0b10000001);
    
    toggleTime(); // Set RTC Time
    printf("\nSystem start time %d:%d:%d\n",sysHours,sysMin,sysSec);
    printf("\n");

    //Set up the Buttons
	for(unsigned int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		//Changed to pull down using BCM pins
		pullUpDnControl(BTNS[j], PUD_DOWN);
	}

    //Attach interrupts to Buttons
    wiringPiISR (BTNS[0], INT_EDGE_RISING,  &startStop );
    wiringPiISR (BTNS[1], INT_EDGE_RISING,  &stopAlarm );
    wiringPiISR (BTNS[2], INT_EDGE_RISING,  &changeInterval );
    wiringPiISR (BTNS[3], INT_EDGE_RISING,  &resetTime );

    // Create sensor thread
    pthread_t sensorThreead ;
    int sensorThreadErr = pthread_create(&sensorThreead,NULL,sample_sensors,NULL);
    if (sensorThreadErr){
        printf("Error occured starting sensor thread");
    }

        // Create alarm thread
    pthread_t alarmThread ;
    int alarmThreadErr = pthread_create(&alarmThread,NULL,sound_alarm,NULL);
    if (alarmThreadErr){
        printf("Error occured setting up alarm thread");
    }

    buzz(BUZZER); //Test Buzzer

}

void loop()
{
    Blynk.run();
}


int main(int argc, char* argv[])
{
    parse_options(argc, argv, auth, serv, port);

    setup();
    while(true) {
        loop();
    }

    return 0;
}

