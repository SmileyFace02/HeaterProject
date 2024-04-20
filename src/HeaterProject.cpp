#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <config.h>
#include <macros.h>

// -------------------- FUNCTION DECLARATIONS --------------------
float tempFromAnalog(int);
void setHeatPower(float);
void updateScreen();
void editModeToggle();
void inputHandler();
void millisOverflowHandler(long*);
void update();
bool softDelay(long*);

// -------------------- MENU --------------------
struct MenuItem {
  const char* name;
  volatile float** fvaluesPtr;
  int fvaluesCount;
  void (*onClickAction)();
};
struct Menu {
  const char* title;
  MenuItem* items;
  int itemCount;
};


// -------------------- GLOBAL VARIABLES --------------------
volatile float currentTemperature = 0;
volatile float setTemperature = 0;

volatile float currentSpeed = 0;
volatile float setSpeed = 0;
bool step = false;
bool dir = true;

volatile bool heaterOn = false;
volatile bool motorOn = false;
volatile bool fanOn = false;

volatile int lastEncoderState = 0;
volatile int encoderSteps = 0;

volatile int lastButton0State = 0;
volatile int lastButton1State = 0;
volatile int lastButton2State = 0;

volatile int editModeToggleState = 0;

// -------------------- CONSTANTS --------------------
volatile float* temps[] = {&currentTemperature, &setTemperature};
volatile float* speed[] = {&currentSpeed, &setSpeed};
MenuItem mainMenuItems[] = {
  {"Temp", temps, 2, editModeToggle},
  {"Speed", speed, 1, editModeToggle}
};
Menu mainMenu = {
  "Main Menu",
  mainMenuItems,
  2         // Menu item count
};

Menu *activeMenu = &mainMenu;
int activeMenuCursor = 0;

#if !defined(HAS_SCREEN) || !defined(SCREEN_ADDRESS)
  #error "Screen not defined"
#endif
LiquidCrystal_I2C lcd(SCREEN_ADDRESS, 16, 2);


// -------------------- SETUP AND LOOP --------------------

void setup() {
  #ifndef HEATER_PIN
    #error "HEATER_PIN not defined"
  #endif
    pinMode(HEATER_PIN, OUTPUT);
  
  #ifdef FAN_PIN
    pinMode(FAN_PIN, OUTPUT);
  #endif
  #ifdef BUTTON_0_PIN
    pinMode(BUTTON_0_PIN, INPUT_PULLUP);
  #endif
  #ifdef BUTTON_1_PIN
    pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  #endif
  #ifdef BUTTON_2_PIN
    pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  #endif

  #ifdef MOTOR_STEP_PIN
    pinMode(MOTOR_STEP_PIN, OUTPUT);
  #endif
  #ifdef MOTOR_DIR_PIN
    pinMode(MOTOR_DIR_PIN, OUTPUT);
  #endif

  #ifndef NTC_PIN
    #error "NTC_PIN not defined"
  #endif
    pinMode(NTC_PIN, INPUT);

  #ifdef HAS_SCREEN
    lcd.init();
    lcd.backlight();
  #endif

  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  Serial.begin(9600);
}

long millis_lastScreenRefresh = 0;
long millis_lastUpdate = 0;
long millis_lastMotorStep = 0;
void loop() {
  if (softDelay(&millis_lastScreenRefresh, SCREEN_REFRESH_MILLISECONDS)) updateScreen();

  if (softDelay(&millis_lastUpdate, (int)(1000.0/UPDATE_FREQ))) update();

  if (motorOn && softDelay(&millis_lastMotorStep, (int)(1000.0/currentSpeed))){
    step = !step;
    digitalWrite(MOTOR_DIR_PIN, dir);
    digitalWrite(MOTOR_STEP_PIN, step);
  }

  inputHandler();
}


// -------------------- FUNCTION DEFINITIONS --------------------
float tempFromAnalog (int val) {
  float sensor_resistance = (1023.0*NTC_RESISTOR)/((float)val) - NTC_RESISTOR;
  float T = log(sensor_resistance/REFERENCE_RESISTANCE);
  T /= NTC_BETA;
  T += 1.0 / REFERENCE_TEMP_KELVIN;
  T = 1.0 / T;
  return (T - 273.15);
}

long millis_lastHeaterOn = 0;
long millis_lastHeaterOff = 0;
void setHeatPower(float percentage) {
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  #ifndef HEATER_SWITCH_FREQ
    analogWrite(MOSFET_PIN, percentage * 255 / 100);
  #else

  millisOverflowHandler(&millis_lastHeaterOff);
  int onTime = millis() - millis_lastHeaterOff;
  int onTimeLimit = (int) (10.0*(float)percentage)/HEATER_SWITCH_FREQ;
  if (heaterOn && (onTime >= onTimeLimit)) {
    heaterOn = false;
    digitalWrite(HEATER_PIN, heaterOn);
    millis_lastHeaterOn = millis();
  }
  millisOverflowHandler(&millis_lastHeaterOn);
  int offTime = millis() - millis_lastHeaterOn;
  int offTimeLimit = 1000 - onTimeLimit;
  if (!heaterOn && (offTime >= offTimeLimit)){
    heaterOn = true;
    digitalWrite(HEATER_PIN, heaterOn);
    millis_lastHeaterOff = millis();
  }

  #endif
}

void updateScreen() {
  lcd.clear();
  if (editModeToggleState) lcd.print(">");
  else lcd.print("-");
  for (int i = 0; (i < activeMenu->itemCount) || (i < 2); i++){
    lcd.setCursor(1, i);
    Serial.println();
    lcd.print(activeMenu->items[activeMenuCursor+i].name);
    Serial.print(activeMenu->items[activeMenuCursor+i].name);
    lcd.print(": ");
    Serial.print(": ");
    for (int j = 0; j < activeMenu->items[activeMenuCursor+i].fvaluesCount; j++){
      float current_value = *activeMenu->items[activeMenuCursor+i].fvaluesPtr[j];
      if (j>0) {
        lcd.print(" / ");
        Serial.print (" / ");
      }
      lcd.print(current_value);
      Serial.print(current_value);
    }
  }
}

void handleEncoder() {
  int encoderState = digitalRead(ENCODER_PIN_A);
  if (encoderState != lastEncoderState) {
    if (digitalRead(ENCODER_PIN_B) != encoderState) {
      encoderSteps++;
    } else {
      encoderSteps--;
    }
  }
  lastEncoderState = encoderState;
}

void editModeToggle(){
  editModeToggleState = !editModeToggleState;
}

void inputHandler(){
  int encoderState = digitalRead(ENCODER_PIN_A);
  if (encoderState != lastEncoderState) {
    if (digitalRead(ENCODER_PIN_B) != encoderState) {
      encoderSteps++;
    } else {
      encoderSteps--;
    }
  }
  lastEncoderState = encoderState;
}

void millisOverflowHandler(long* millis_ptr){
  if(millis() < *millis_ptr) *millis_ptr = *millis_ptr - maxof(long);
  return;
}

void update(){
  currentTemperature = tempFromAnalog(analogRead(NTC_PIN));
  float tempError = currentTemperature-setTemperature;
  float heatPower = -tempError*(100.0/TEMP_ERROR_MAX);
  
  if(heaterOn) setHeatPower(heatPower);
  else setHeatPower(0);
  float speedError = currentSpeed - setSpeed;
  float accelAddition = (float)MOTOR_ACCELERATION*UPDATE_FREQ/1000.0;
  if(speedError > accelAddition) currentSpeed += accelAddition;
  else currentSpeed = setSpeed;

  int lastValIndex = activeMenu->items[activeMenuCursor].fvaluesCount - 1;
  if(editModeToggleState) activeMenu->items[activeMenuCursor].fvaluesPtr[lastValIndex] += encoderSteps;
  else activeMenuCursor += encoderSteps;
  encoderSteps = 0;

  return;
}

bool softDelay(long* lastMillis_ptr, int delay_milliseconds){
  millisOverflowHandler(lastMillis_ptr);
  if (millis() - *lastMillis_ptr >= delay_milliseconds){
    *lastMillis_ptr = millis();
    return true;
  } 
  return false;
}
