#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <config.h>
#include <macros.h>

// -------------------- FUNCTION DECLARATIONS --------------------
void update();
void updateScreen();
void inputHandler();
void editmodeToggle();
float tempFromAnalog(int);
void setHeatPower(float);

void millisOverflowHandler(unsigned long*);
bool softDelay(unsigned long*, unsigned int);


// -------------------- GLOBAL VARIABLES --------------------
volatile float temperature[] = {0.0, 0.0};  //current, set
volatile float speed[] = {0.0, 0.0};        //current, set

volatile int step = false;
volatile int dir = true;

volatile int heaterOn = false;
volatile int motorOn = false;
volatile int fanOn = false;

volatile int lastEncoderState = 0;
volatile int encoderSteps = 0;

volatile int editMode = false;

char* screenData = nullptr;

// -------------------- MENU --------------------
struct MenuItem {
  const char* name;
  int fvalueCount;
  volatile float* fvaluePtr;
  int actionsCount;
  void (*onClickAction[])();    // void* func_name is a function returning void* , void (*func_name) points to a function returning void
};
struct Menu {
  const char* title;
  const int itemCount;
  MenuItem* items;
};

MenuItem mainMenuItems[] = {
  {"Temp", 2, temperature, 1, {editmodeToggle}},
  {"Speed", 2, speed, 1, {editmodeToggle}}
};
Menu mainMenu = {
  "Main Menu",
  2,                // Menu item count
  mainMenuItems
};

Menu* activeMenu = &mainMenu;
int activeMenuCursor = 0;

#if defined(HAS_SCREEN) && !defined(SCREEN_ADDRESS)
  #error "Screen adress not defined"
#endif
LiquidCrystal_I2C lcd(SCREEN_ADDRESS, SCREEN_WIDTH, SCREEN_HEIGHT);


// --------------------------------------------------------------
void setup() {
  #ifndef HEATER_PIN
    #error "HEATER_PIN not defined"
  #endif
  pinMode(HEATER_PIN, OUTPUT);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUTTON_0_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);

  pinMode(MOTOR_STEP_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);

  #ifndef NTC_PIN
    #error "NTC_PIN not defined"
  #endif
  pinMode(NTC_PIN, INPUT);

  lcd.init();
  lcd.backlight();

  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  Serial.begin(9600);

  Serial.println("Trying malloc for screenData");
  while(screenData == nullptr) screenData = (char*) malloc(sizeof(char)*SCREEN_WIDTH*SCREEN_HEIGHT);
  Serial.println("ScreenData memory allocated");
}

unsigned long lastScreenRefresh_ms = 0;
unsigned long lastUpdate_ms = 0;
unsigned long lastMotorStep_ms = 0;
void loop() {
  #ifdef HAS_SCREEN
  if (softDelay(&lastScreenRefresh_ms, SCREEN_REFRESH_MILLISECONDS)) updateScreen();
  #endif

  if (softDelay(&lastUpdate_ms, (int)(1000.0/UPDATE_FREQ))) update();

  if (motorOn && softDelay(&lastMotorStep_ms, (int)(1000.0/abs(speed[1])))){
    step = !step;
    if (speed[0] >= 0) digitalWrite(MOTOR_DIR_PIN, !INVERT_MOTOR_DIRECTION);
    else digitalWrite(MOTOR_DIR_PIN, INVERT_MOTOR_DIRECTION);
    digitalWrite(MOTOR_STEP_PIN, step);
  }

  inputHandler();
}


// -------------------- FUNCTION DEFINITIONS --------------------
void update(){
  temperature[0] = tempFromAnalog(analogRead(NTC_PIN));
  float tempError = temperature[0]-temperature[1];
  float heatPower = -tempError*(100.0/TEMP_ERROR_MAX);
  
  if(heaterOn) setHeatPower(heatPower);
  else setHeatPower(0);
  float speedError = speed[0] - speed[1];
  float accelAddition = (float)MOTOR_ACCELERATION*UPDATE_FREQ/1000.0;
  if(speedError > accelAddition) speed[0] += accelAddition;
  else speed[0] = speed[1];

  int lastValIndex = activeMenu->items[activeMenuCursor].fvalueCount - 1;
  if(editMode) activeMenu->items[activeMenuCursor].fvaluePtr[lastValIndex] = activeMenu->items[activeMenuCursor].fvaluePtr[lastValIndex] + encoderSteps;
  else activeMenuCursor = (activeMenuCursor+encoderSteps)%activeMenu->itemCount;
  encoderSteps = 0;

  digitalWrite(FAN_PIN, fanOn);

  return;
}

void updateScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (editMode) lcd.print(">");
  else lcd.print("-");

  for (int i = 0; i < SCREEN_HEIGHT; i++){
    MenuItem* activeItem = &activeMenu->items[(activeMenuCursor+i)%(activeMenu->itemCount)];

    lcd.setCursor(1, i);
    Serial.println();

    lcd.print(activeItem->name);
    Serial.print(activeItem->name);

    lcd.print(" ");
    Serial.print(": ");
    for (int j = 0; j < activeItem->fvalueCount; j++){
      float current_value = activeItem->fvaluePtr[j];
      if (j>0) {
        lcd.print("/");
        Serial.print (" / ");
      }
      lcd.print((int)current_value);
      Serial.print(current_value);
    }
  }
}

int lastButtonState[4] = {0,0,0,0};
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

  int buttonEState = digitalRead(ENCODER_BUTTON_PIN);
  if (buttonEState){
    if(!lastButtonState[0]){
      activeMenu->items[activeMenuCursor].onClickAction[0]();
      lastButtonState[0] = buttonEState;
    }
  }
  else lastButtonState[0] = buttonEState;

  int button0State = digitalRead(TOGGLE_HEAT_BUTTON);
  if (button0State){
    if(!lastButtonState[1]){
      heaterOn = !heaterOn;
      lastButtonState[1] = button0State;
    }
  }
  else lastButtonState[1] = button0State;

  int button1State = digitalRead(TOGGLE_MOTOR_BUTTON);
  if (button1State){
    if(!lastButtonState[2]){
      motorOn = !motorOn;
      lastButtonState[2] = button1State;
    }
  }
  else lastButtonState[2] = button1State;

  int button2State = digitalRead(TOGGLE_FAN_BUTTON);
  if (button2State){
    if(!lastButtonState[3]){
      fanOn = !fanOn;
      lastButtonState[3] = button2State;
    }
  }
  else lastButtonState[3] = button2State;

  return;
}

void editmodeToggle(){
  editMode = !editMode;
}

float tempFromAnalog (int val) {
  float sensor_resistance = (1023.0*NTC_RESISTOR)/((float)val) - NTC_RESISTOR;
  float T = log(sensor_resistance/REFERENCE_RESISTANCE);
  T /= NTC_BETA;
  T += 1.0 / REFERENCE_TEMP_KELVIN;
  T = 1.0 / T;
  return (T - 273.15);
}

unsigned long lastHeaterOn_ms = 0;
unsigned long lastHeaterOff_ms = 0;
void setHeatPower(float percentage) {
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  #ifndef HEATER_SWITCH_FREQ
    analogWrite(MOSFET_PIN, percentage * 255 / 100);
  #else

  millisOverflowHandler(&lastHeaterOff_ms);
  int onTime = millis() - lastHeaterOff_ms;
  int onTimeLimit = (int) (10.0*(float)percentage)/HEATER_SWITCH_FREQ;
  if (heaterOn && (onTime >= onTimeLimit)) {
    heaterOn = false;
    digitalWrite(HEATER_PIN, heaterOn);
    lastHeaterOn_ms = millis();
  }
  millisOverflowHandler(&lastHeaterOn_ms);
  int offTime = millis() - lastHeaterOn_ms;
  int offTimeLimit = 1000 - onTimeLimit;
  if (!heaterOn && (offTime >= offTimeLimit)){
    heaterOn = true;
    digitalWrite(HEATER_PIN, heaterOn);
    lastHeaterOff_ms = millis();
  }

  #endif
}

void millisOverflowHandler(unsigned long* millis_ptr){
  if(millis() < *millis_ptr) *millis_ptr = 0; //*millis_ptr - maxof(unsigned long);
  return;
}


bool softDelay(unsigned long* lastMillis_ptr, unsigned int delay_milliseconds){
  millisOverflowHandler(lastMillis_ptr);
  if (millis() - *lastMillis_ptr >= delay_milliseconds){
    *lastMillis_ptr = millis();
    return true;
  } 
  return false;
}

