#ifndef PINS_H
#define PINS_H

// -------------- Edit these values to match your setup --------------
#define HEATER_PIN 12
#define HEATER_SWITCH_FREQ 1 //if HEATER_PIN is not PWM capable -> 'software PWM'

#define FAN_PIN 13

#define MOTOR_STEP_PIN 9
#define MOTOR_DIR_PIN 10
#define MOTOR_ACCELERATION 250
#define INVERT_MOTOR_DIRECTION 0

#define BUTTON_0_PIN 2
#define BUTTON_1_PIN 3
#define BUTTON_2_PIN 4
                                    //button functions
#define TOGGLE_HEAT_BUTTON BUTTON_0_PIN
#define TOGGLE_MOTOR_BUTTON BUTTON_1_PIN
#define TOGGLE_FAN_BUTTON BUTTON_2_PIN

#define HAS_SCREEN                  //Only a 16x2 i2c display is supported currently
#define SCREEN_ADDRESS 0x27         //I2C address
#define SCREEN_HEIGHT 2
#define SCREEN_REFRESH_MILLISECONDS 100

#define ENCODER_PIN_A 5
#define ENCODER_PIN_B 6
#define ENCODER_BUTTON_PIN 7

#define NTC_PIN A0                  //Analog pin
#define NTC_VALUE 100000.0          //100k NTC
#define NTC_RESISTOR 100000.0       //100k resistor
#define NTC_BETA 3950.0             //3950 beta

#define UPDATE_FREQ 10              //(Hz) check and recalculate everything at this frequency
#define TEMP_ERROR_MAX 10           //(C) sets at which point the power starts going down when nearing the target 

// -------------- System defines, do not change --------------
#define REFERENCE_TEMP_CELSIUS 25.0         //25 degrees celsius
#define REFERENCE_RESISTANCE 100000.0       //100k NTC
#define REFERENCE_TEMP_KELVIN (273.15 + REFERENCE_TEMP_CELSIUS)

#endif