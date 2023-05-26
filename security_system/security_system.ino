#include <dht.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2); 

const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
char pass[4] = {'5', '5', '5', '5'};
char input[4] = {'0', '0', '0', '0'};
dht sensor;

SoftwareSerial esp32 = SoftwareSerial(A0, A1);

unsigned long prevMillis = 0;
int timer_count = 0;
bool armed, alarm;
enum Sys_States {Sys_Init, Sys_Disarmed, Sys_Armed, Sys_Alarm} Sys_State;
enum Keypad_States {Keypad_Init, Keypad_Idle, Keypad_1, Keypad_2, Keypad_3, Keypad_4} Keypad_State;
enum Alarm_States {Alarm_Init, Alarm_Off, Alarm_On_Pause, Alarm_On_Sound} Alarm_State;

void Sys_Tick() {
  switch (Sys_State) {
    case Sys_Init:
      Sys_State = Sys_Disarmed;
      Draw_Menu();
    case Sys_Disarmed:
      if (armed) {
        Sys_State = Sys_Armed;
        Draw_Menu();
        Serial.println("System is armed");
        esp32.println("System is armed");
      } 
      break;
    case Sys_Armed:
      if (alarm) {
        Sys_State = Sys_Alarm;
        Draw_Menu();
        Serial.println("Alarm Sounded");
        esp32.println("Alarm Sounded");
      } else if (!armed) {
        Sys_State = Sys_Disarmed;
        Draw_Menu();
        Serial.println("System is disarmed");
        esp32.println("System is disarmed");
      } 
      break;
    case Sys_Alarm:
      if (!armed) {
        Sys_State = Sys_Disarmed;
        Draw_Menu();
        Serial.println("System is disarmed, alarm stopped");
        esp32.println("System is disarmed, alarm stopped");
      }
      break;
  }
}

void Alarm_Tick() {
  switch (Alarm_State) {
    case Alarm_Init:
      Alarm_State = Alarm_Off;
      break;
    case Alarm_Off:
      if (alarm)
        Alarm_State = Alarm_On_Sound;
      break;
    case Alarm_On_Sound:
      if (alarm) 
        Alarm_State = Alarm_On_Pause;
      else 
        Alarm_State = Alarm_Off;
      break;
    case Alarm_On_Pause:
      if (alarm)
        Alarm_State = Alarm_On_Sound;
      else 
        Alarm_State = Alarm_Off;
      break;
    default:
      Alarm_State = Alarm_Off;
  }

  switch (Alarm_State) {
    case Alarm_Off:
      noTone(11);
      break;
    case Alarm_On_Sound:
      tone(11, 440);
      break;
    case Alarm_On_Pause:
      noTone(11);
      break;
  }
  
}
void Keypad_Tick() {
  char key = keypad.getKey();
  switch (Keypad_State) {
    case Keypad_Init:
      Keypad_State = Keypad_Idle;
    case Keypad_Idle:
      if (key != NO_KEY) {
        Keypad_State = Keypad_1;
        Draw_Menu();
        lcd.print(key);
        input[0] = key;
      }
      break;
    case Keypad_1:
      if (key != NO_KEY) {
        Keypad_State = Keypad_2;
        Draw_Menu();
        lcd.print(input[0]);
        lcd.print(key);
        input[1] = key;
      }
      break;
    case Keypad_2:
      if (key != NO_KEY) {
        Keypad_State = Keypad_3;
        Draw_Menu();
        lcd.print(input[0]);
        lcd.print(input[1]);
        lcd.print(key);
        input[2] = key;
      }
      break;
    case Keypad_3:
      if (key != NO_KEY) {
        Keypad_State = Keypad_4;
        Draw_Menu();
        lcd.print(input[0]);
        lcd.print(input[1]);
        lcd.print(input[2]);
        lcd.print(key);
        input[3] = key;
      }
      break;
    case Keypad_4:
      Keypad_State = Keypad_Idle;
      lcd.clear();
      Draw_Menu();
      if (armed) {
        if (input[0] == pass[0] && input[1] == pass[1] && input[2] == pass[2] && input[3] == pass[3]) {
          armed = false;
        } else {
          lcd.print("WRONG PASSCODE");
        }
      } else {
        pass[0] = input[0];
        pass[1] = input[1];
        pass[2] = input[2];
        pass[3] = input[3];
        lcd.print("PASS CHANGED");
      }
      break;
  }
}

void Motion_Tick() {
  int motion_detected = digitalRead(12);
  if (motion_detected && armed) {
    alarm = true;
  } else if (!armed) {
    alarm = false;
  }
}

void Draw_Menu() {
  lcd.clear();
  if (alarm)
    lcd.print("System: Alarm");
  else if (armed)
    lcd.print("System: Armed");
  else
    lcd.print("System: Disarmed");
  lcd.setCursor(0, 1);
}

void Sensor_Tick() {
  sensor.read11(10);
  if (armed)
    esp32.println("System: Armed");
  else 
    esp32.println("System: Disarmed");
  esp32.print("Humidity: ");
  esp32.print(sensor.humidity);
  esp32.println("%");
  esp32.print("Temperature: ");
  esp32.print(sensor.temperature);
  esp32.println(" deg C");
}

void ESP32_Tick() {
  if (esp32.available()) {
    char arm_command = esp32.read();
    armed = (arm_command == '1') ? true : false;
  }
}

void setup() {
  pinMode(12, INPUT);     
  Serial.begin(9600);
  esp32.begin(9600);
  lcd.init();
  lcd.clear();         
  lcd.backlight(); 
  Sys_State = Sys_Init;
  Keypad_State = Keypad_Init;
  armed = false;
  alarm = false;
}

void tick() {
  if (timer_count < 99)
    timer_count++;
  else 
    timer_count = 0;
  
  ESP32_Tick();
  Keypad_Tick();
  Motion_Tick();
  Sys_Tick();

  if (timer_count % 5 == 0) {
    Alarm_Tick();
  }

  if (timer_count % 20 == 0) {
    Sensor_Tick();
  }
}
void loop() {
  unsigned int currMillis = millis();
  if (currMillis - prevMillis >= 100) {
    prevMillis = currMillis;
    tick();
  }
}