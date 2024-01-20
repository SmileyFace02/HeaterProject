#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <config.h>

// -------------------- FUNCTION DECLARATIONS --------------------
float tempFromResistance(int);
void setOutput(float);
void updateScreen();
void handleEncoder();
void editModeToggle();
void updateValues();
void tempControl();


// -------------------- MENU --------------------
struct MenuItem {
  const char* name;
  float** fvaluesPtr;
  int fvaluesc;
  void (*action)();
};
struct Menu {
  const char* title;
  MenuItem* items;
  int itemCount;
};


// -------------------- GLOBAL VARIABLES --------------------
float currentTemperature = 0;
float setTemperature = 0;

int lastEncoderState = 0;
int encoderSteps = 0;
int editModeToggleState = 0;

// -------------------- CONSTANTS --------------------
float* temps[] = {&currentTemperature, &setTemperature};
MenuItem mainMenuItems[] = {
  {"Temp", temps, 2, editModeToggle}
};
Menu mainMenu = {
  "Main Menu",
  mainMenuItems,
  1         // Menu item count
};

Menu *activeMenu = &mainMenu;
MenuItem *activeMenuItem = &activeMenu->items[0];

#if !defined(HAS_SCREEN) || !defined(SCREEN_ADDRESS)
  #error "Screen not defined"
#endif
LiquidCrystal_I2C lcd(SCREEN_ADDRESS, 16, 2);


// -------------------- SETUP AND LOOP --------------------

void setup() {
  #ifndef MOSFET_PIN
    #error "MOSFET_PIN not defined"
  #endif
    pinMode(MOSFET_PIN, OUTPUT);

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
  pinMode(MOSFET_PIN, OUTPUT);

  Serial.begin(9600);
}

long millis_last = millis();
void loop() {
  if (millis() - millis_last > 25) {
    millis_last = millis();
    updateValues();
    updateScreen();
  }
  tempControl();
  handleEncoder();
}


// -------------------- FUNCTION DEFINITIONS --------------------

float tempFromResistance(int resistance) {
  float T = 1.0 / ((1.0 / (float)REFERENCE_TEMP_KELVIN) + (1.0 / (float)NTC_BETA) * log((float)resistance / (float)REFERENCE_RESISTANCE));
  return (T - 273.15);
}

void setOutput(float percentage) {
  #ifdef MOSFET_HIGH_OFF
    percentage = 100 - percentage;
  #endif
  if (percentage < 0) {
    analogWrite(MOSFET_PIN, 0);
    return;
  }
  else if (percentage > 100) {
    analogWrite(MOSFET_PIN, 255);
    return;
  }
  analogWrite(MOSFET_PIN, percentage * 255 / 100);
}

void updateScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(activeMenuItem->name);
  lcd.print(":  ");
  lcd.setCursor(0, 1);
  float** vals = activeMenuItem->fvaluesPtr;
  lcd.print(currentTemperature);
  lcd.print("/");
  lcd.print(setTemperature);
  lcd.print("/");
  lcd.print(encoderSteps);


  Serial.print(">TEMP ");
  Serial.print("[");
  Serial.print(currentTemperature);
  Serial.print("] / [");
  Serial.print(setTemperature);
  Serial.println("]");
}

void updateValues() {
  setTemperature += encoderSteps;
  encoderSteps = 0;
  currentTemperature = tempFromResistance(analogRead(NTC_PIN));
  setOutput(setTemperature);
}

void handleEncoder() {
  int a = digitalRead(ENCODER_PIN_A);
  int b = digitalRead(ENCODER_PIN_B);
  if (a != lastEncoderState) {
    if (b != a) {
      encoderSteps++;
    } else {
      encoderSteps--;
    }
  }
  lastEncoderState = a;
}

void editModeToggle() {
  editModeToggleState = !editModeToggleState;
}

void tempControl() {
  float error = setTemperature - currentTemperature;
  float output = error * 0.8;
  setOutput(output);
}