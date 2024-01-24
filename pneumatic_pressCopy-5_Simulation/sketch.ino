
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>


LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

const uint8_t limit_sw_pin = 2;
const uint8_t main_sw1_pin = 3;
const uint8_t main_sw2_pin = 4;
const uint8_t up_pin = 5;
const uint8_t down_pin = 6;
const uint8_t mode_pin = 7;
const uint8_t relay_pin = 8;
const uint8_t led = 9;

Button limitSw(limit_sw_pin);
Button mainSw1(main_sw1_pin);
Button mainSw2(main_sw2_pin);
Button upBtn(up_pin);
Button downBtn(down_pin);
Button modeBtn(mode_pin);
// Button mode_btn(mode_btn_pin);


//Menu Part

int state;                            //saves the current state of the menu
const unsigned long LONG_PRESS(1000); //duration time for long press to trigger
byte arrow[] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00000,
  B00000
};

byte m[] = {
  B00000,
  B10001,
  B11111,
  B11111,
  B10101,
  B10001,
  B10001,
  B00000
};

byte s[] = {
  B00000,
  B00110,
  B01000,
  B01100,
  B00010,
  B00010,
  B01100,
  B00000
};

byte tenth_s[] = {
  B00000,
  B00000,
  B10010,
  B10101,
  B10101,
  B10010,
  B00000,
  B00000
};


struct Timer {
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t tenth = 0;
};

struct Register {
  uint8_t firstBoot = 14;
  uint8_t minute = 16;
  uint8_t second = 20;
  uint8_t tenth = 24;
};

#define YES 68
const struct Register r;

enum section {
  MINUTE,
  SECOND,
  TENTH,
};

struct Timer t;
struct Timer tempT;
uint8_t section = MINUTE;
bool sectionSelected = false;
uint8_t prevSection = section;
bool running = false;
unsigned long int previousMillis = 0;
unsigned long int prevMillis2= 0;

unsigned long int blinkDelay = 500;
uint8_t screenRefreshDelay = 50; 
uint8_t timeoutDelay = 60; //1000ms * 10 = 10seconds
bool blinkState = false;
uint8_t count = 0;
bool commit = false;
//when commit is true it will save the values to eeprom to refer to it later on.

void setup() {

  //get values from eeprom as soon as it starts.
  upBtn.begin();
  downBtn.begin();
  modeBtn.begin();
  //can be any random value. but if not 68 then consider first boot
  //if so clear the eeprom make it zero.
  uint8_t val = EEPROM.read(r.firstBoot);
  Serial.println(val);
  if (val != YES)
  {
    Serial.println("considering first boot");
    for (int i = 0; i < 512; i++)
    {
      EEPROM.write(i, 0);
    }
    EEPROM.put(r.firstBoot, YES);
  }
  EEPROM.get(r.minute, t.minute);
  EEPROM.get(r.second, t.second);
  EEPROM.get(r.tenth,  t.tenth );

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pneumatic Press");
  lcd.setCursor(0, 1);
  lcd.print("System...         ");
  lcd.createChar(0, arrow);
  lcd.createChar(1, m); 
  lcd.createChar(2, s); 
  lcd.createChar(3, tenth_s); 
  delay(1000);
  lcd.clear();


  pinMode(led, OUTPUT);
  pinMode(relay_pin, OUTPUT);



  Serial.begin(9600);
}

void loop() {
  //updates the states of the button so that fresh values are taken.
  updateBtns();
  //show Timer and current values
  updateTimer();
  //handles how the menu behaves and changing of selection
  menuButtonsHandler();
  //handles how the arrow moves and blinks.
  updateArrow();

}



void updateBtns() {
  upBtn.read();
  downBtn.read();
  modeBtn.read();
}




void commitToEeprom()
{
  if (commit == true)
  {
    commit = false;

    switch (section)
    {
      case MINUTE:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Minute's Set");
        delay(1000);
        lcd.clear();
        EEPROM.put(r.minute, t.minute);
        break;

      case SECOND:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Seconds Set");
        delay(1000);
        lcd.clear();
        EEPROM.put(r.second, t.second);
        break;

      case TENTH:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Deci Second Set");
        delay(1000);
        lcd.clear();
        EEPROM.put(r.tenth, t.tenth);
        break;
    }
  }
}

void updateArrow() {

  bool prevBlinkState;
  bool changed = false;
  if (sectionSelected == true)
  {

    //tick tock timer to know when blink time has reached.
    updateBtns();
    if (millis() - previousMillis >= blinkDelay) {
      prevBlinkState = blinkState;//in order to remember when the state changed
      blinkState = !blinkState;   //Toggling the state variable
      changed = true;             //in order to change whenever data changed.
      previousMillis = millis();
      Serial.print("Flipped state to: ");
      Serial.println(blinkState);

      count++;
      //if user is inactive for more than two minutes. changes will not commit and exit out of selection mode
      if (count >= timeoutDelay)
      {
        count = 0;
        sectionSelected = false;
        previousMillis = 0;
        commit = false;
        lcd.clear();
      }
    }


    //making sure to only blink once and not be busy in writing to lcd forever.
    if ((prevBlinkState != blinkState) && changed == true) {
      changed = false;
      prevBlinkState = blinkState;
      if (blinkState == false) {
        switch (section) {
          case MINUTE:
            lcd.setCursor(2, 1);
            lcd.write(0);
            lcd.setCursor(3, 1);
            lcd.write(0);
            break;

          case SECOND:
            lcd.setCursor(6, 1);
            lcd.write(0);
            lcd.setCursor(7, 1);
            lcd.write(0);
            break;

          case TENTH:
            lcd.setCursor(10, 1);
            lcd.write(0);
            lcd.setCursor(11, 1);
            lcd.write(0);
            lcd.setCursor(12, 1);
            lcd.write(0);
            break;
        }
      }
      //when time difference is less than blink delay
      else {
        changed = false;
        prevBlinkState = blinkState;
        switch (section) {
          case MINUTE:
            lcd.setCursor(2, 1);
            lcd.print(" ");
            lcd.setCursor(3, 1);
            lcd.print(" ");
            break;

          case SECOND:
            lcd.setCursor(6, 1);
            lcd.print(" ");
            lcd.setCursor(7, 1);
            lcd.print(" ");
            break;

          case TENTH:
            lcd.setCursor(10, 1);
            lcd.print(" ");
            lcd.setCursor(11, 1);
            lcd.print(" ");
            lcd.setCursor(12, 1);
            lcd.print(" ");
            break;
        }
      }
    }
  }


  //whenever user changes selection to minute - seconds etc. arrow updates
  if (prevSection != section) {
    lcd.clear();
    updateTimer();
    switch (section) {
      case MINUTE:
        lcd.setCursor(2, 1);
        lcd.write(0);
        lcd.setCursor(3, 1);
        lcd.write(0);
        break;

      case SECOND:
        lcd.setCursor(6, 1);
        lcd.write(0);
        lcd.setCursor(7, 1);
        lcd.write(0);
        break;

      case TENTH:
        lcd.setCursor(10, 1);
        lcd.write(0);
        lcd.setCursor(11, 1);
        lcd.write(0);
        lcd.setCursor(12, 1);
        lcd.write(0);
        break;
    }
  }
  prevSection = section;
}

//makes sure to add zero before 1 digit numbers.


void updateTimer() {
  if (prevMillis2 - millis() >= screenRefreshDelay)
  {
    prevMillis2 = millis();
    lcd.setCursor(2, 0);
    lcd.print(t.minute);
    lcd.write(1);  //1st character is of minute as declared in setup 
    lcd.setCursor(5, 0);
    lcd.print(':');
    lcd.setCursor(6, 0);
    lcd.print(t.second);
    lcd.write(2);
    lcd.setCursor(9,0);
    lcd.print(':');
    lcd.setCursor(10, 0);
    lcd.print(t.tenth);
    lcd.write(3);
  }
}

void menuButtonsHandler() {
  if (running == false)
  {
    //menu button up and down logic
    if (sectionSelected == false) {
      updateBtns();
      if (upBtn.wasPressed())
      {
        if (section != TENTH)
        {
          section++;
        }
        else {
          section = MINUTE;
        }
      }
      if (downBtn.wasPressed())
      {
        if (section != MINUTE)
        {
          section--;
        }
        else {
          section = TENTH;
        }
      }
      if (modeBtn.wasPressed())
      {
        sectionSelected = true;
      }

      //some change happened
      //update the arrow
      //updateArrow();

    }


    //once selection is made
    if (sectionSelected == true)
    {
      updateBtns();
      switch (section)
      {
        case MINUTE:
          if (upBtn.wasPressed())
          {
            if (t.minute == 10) lcd.clear();
            if (t.minute == 60)
            {
              t.minute = 00;
              lcd.clear();
            }
            else {
              t.minute++;
            }


          }
          if (downBtn.wasPressed())
          {
            if (t.minute == 10) lcd.clear();
            if (t.minute == 0)
            {
              t.minute = 60;
              lcd.clear();
            }
            else {
              t.minute--;
            }
          }
          if (modeBtn.wasPressed())
          {

            //Save to EEPROM
            commit = true;
            commitToEeprom();
            sectionSelected = false;

          }
          break;


        case SECOND:
          if (upBtn.wasPressed())
          {
            if (t.second == 60)
            {
              t.second = 00;
              lcd.clear();
            }
            else {
              t.second++;
            }
          }
          if (downBtn.wasPressed())
          {
            if (t.second == 9) lcd.clear();
            if (t.second == 0)
            {
              t.second = 60;
              lcd.clear();
            }
            else {
              if(t.second == 10) lcd.clear();
              t.second--;
            }
          }
          if (modeBtn.wasPressed())
          {

            //Save to EEPROM
            commit = true;
            commitToEeprom();
            sectionSelected = false;
          }
          break;

        case TENTH:
          if (upBtn.wasPressed())
          {
            if (t.tenth == 9) lcd.clear();
            if (t.tenth == 100)
            {
              t.tenth = 00;
              lcd.clear();
            }
            else {
              t.tenth++;
            }
          }
          if (downBtn.wasPressed())
          {
            if (t.tenth == 10) lcd.clear();
            if (t.tenth == 0)
            {
              t.tenth = 100;
              lcd.clear();
            }
            else {
              if (t.tenth == 100) lcd.clear();
              t.tenth--;
            }
          }
          if (modeBtn.wasPressed())
          {

            //Save to EEPROM
            commit = true;
            commitToEeprom();
            sectionSelected = false;

          }
          break;

      }
    }

  }
}



