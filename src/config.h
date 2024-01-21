#ifndef PINS_H
#define PINS_H

// -------------- Edit these values to match your setup --------------
#define MOSFET_PIN 5                //PWM pin
#define MOSFET_HIGH_OFF             //Reverse mosfet logic - comment to disable

#define NTC_PIN A0                  //Analog pin
#define NTC_VALUE 100000            //100k NTC
#define NTC_RESISTOR 100000         //100k resistor
#define NTC_BETA 3950               //3950 beta

#define REFERENCE_TEMP_CELSIUS 25           //25 degrees celsius
#define REFERENCE_RESISTANCE 100000         //100k NTC

#define HAS_SCREEN                  //Only a 16x2 i2c display is supported currently
#define SCREEN_ADDRESS 0x27         //I2C address
//#define SCREEN_SDA_PIN 4
//#define SCREEN_SCL_PIN 5
//#define SCREEN_RESET_PIN 6

#define HAS_ENCODER
#define ENCODER_PIN_A 2            //Override default pins !INTERRUPT CAPABLE ONLY!
#define ENCODER_PIN_B 4            //Override default pins
#define ENCODER_BUTTON_PIN 3       //Override default pins !INTERRUPT CAPABLE ONLY!

// -------------- System defines, do not change --------------
#define REFERENCE_TEMP_KELVIN 273.15 + REFERENCE_TEMP_CELSIUS

#endif