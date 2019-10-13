#ifndef LOGGER_H
#define LOGGER_H

//Includes
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h> // RTC
#include <mcp3004.h> // ADC
#include <softTone.h> // Buzzer
#include <stdbool.h> 
#include <stdio.h> // For printf functions
#include <stdlib.h> // For system functions
#include <unistd.h> // Sleep function
#include <pthread.h>


// Function definitions
int setup_gpio(void);
void buzz(int pin);
void *sound_alarm(void *threadargs);
void *sample_sensors(void *threadargs);
int sampleHumidity(void);
int sampleLight(void);
int sampleTemperature(void);
int sampleVoltage(void);
int setVoltage(int voltage);
//Buttons
void changeInterval(void);
void stopAlarm(void);
int getHoursRTC();
int getMinsRCTC();
int getSecsRTC();
int bcdConverter(int BCD);
void resetTime(void);
void toggleTime(void);
int decCompensation(int units);
int getSystemRunHours(void); //Determine time system has been running
int getSystemRunMin(void); //Determine time system has been running
int getSystemRunSec(void); //Determine time system has been running
int getAlarmMinutes(void); //Minutes since last alarm activation
int getAlarmHours(void); //Hours since last alarm activation

//SPI ADC Settings
#define SPI_CHAN_ADC 0
#define SPI_SPEED 100000
#define BASE 100
const int humidityPin=7;
const int lightPin=0;
const int tempPin=1;
const int voltageOutputPin=2;

//SPI DAC Settings
#define SPI_CHAN_DAC 1// Write your value here

//Temp Sensor Settings
const float Tc= 0.010; //mV per decgree C
const float V0 = 0.500; // mV offset

// define constants
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 1; // +02H00 (RSA)

// define pins
const int BUZZER = 29 ;
const int BTNS[] = {23,24,0,2}; // B0, B1


#endif
