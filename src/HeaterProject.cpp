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

#if defined(HAS_SCREEN) && defined(SCREEN_ADDRESS)
LiquidCrystal_I2C lcd(SCREEN_ADDRESS, 16, 2);
#endif


// -------------------- SETUP AND LOOP --------------------

void setup() {
  #ifdef MOSFET_PIN
    pinMode(MOSFET_PIN, OUTPUT);
  #else
    #error "MOSFET_PIN not defined"
  #endif

  #ifdef NTC_PIN
    pinMode(NTC_PIN, INPUT);
  #else
    #error "NTC_PIN not defined"
  #endif

  #ifdef HAS_SCREEN
    lcd.init();
    lcd.backlight();
  #endif
  // Initialize serial communication
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON_PIN), editModeToggle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), handleEncoder, CHANGE);
}

long millis_last = millis();
void loop() {
  if (millis() - millis_last > 1000) {
    millis_last = millis();
    updateScreen();
    updateValues();
  }
  tempControl();
}


// -------------------- FUNCTION DEFINITIONS --------------------

float tempFromResistance(int resistance) {
  float T = 1.0 / ((1.0 / REFERENCE_TEMP_KELVIN) + (1.0 / NTC_BETA) * log(resistance / REFERENCE_RESISTANCE));
  return T - 273.15;
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
  lcd.print(activeMenu->title);
  lcd.setCursor(0, 1);
  lcd.print(activeMenuItem->name);
  lcd.print(":  ");
  float** vals = activeMenuItem->fvaluesPtr;
  //lcd.print(String((int)*(vals[0])).substring(0, 2));
  lcd.print((int)*(vals[0]));
  lcd.print("/");
  //lcd.print(String((int)*(vals[1])).substring(0, 2));
  lcd.print((int)*(vals[1]));

  Serial.print(">TEMP ");
  Serial.print("[");
  Serial.print(currentTemperature);
  Serial.print("] / [");
  Serial.print(setTemperature);
  Serial.println("]");

  // Other code to update the display goes here
}

void updateValues() {
  if (editModeToggleState) {
    setTemperature += encoderSteps;
  }
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
  updateValues();
  updateScreen();
}

void editModeToggle() {
  editModeToggleState = !editModeToggleState;
}

void tempControl() {
  float error = setTemperature - currentTemperature;
  float output = error * 0.8;
  setOutput(output);
}