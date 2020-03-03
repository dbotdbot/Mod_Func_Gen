#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include "AD9833.h"
#include <SPI.h>

#define TFT_CS  5
#define TFT_DC  12
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_RST 14

#define squareWave 1
#define sineWave 0
#define triangleWave 2

//Variables relating to the content of the screen
int waveType = 0;  //0 = square, 1 = sine, 2 = triangle

enum screenMode {
  displayAll,
  displayFunc,
  displayFuncMenu
};

long outputVoltage = 0L;
long outputCurrentLimit = 0L;
int frequency = 0;
long pkVoltage = 0L;
long dcOffset = 0L;

//adc input pins
const int potX = 36;
const int potY = 39;
const int dcOffsetPin = 34;
const int pkVoltPin = 35;
const int voltPin = 32;
const int currentLimPin = 33;
//ButtonPins
const int joyButPin = 25;
const int funcButPin = 26;


//Variables for the analog inputs
int xPotVal = 0;
int yPotVal = 0;
int dcOffsetADC = 0;
int pkVoltADC = 0;
int voltADC = 0;
int currentLimADC = 0;

//Variables for Menu
int menu[3] = {};
int (*menuP)[3] = &menu;

const int subMenuLeng = 5;
int subMenu[subMenuLeng] = {};
bool activated = false;
bool funcOn = false;



long dcOffsetPot = 0L;
long pkVoltagePot = 0L;
long voltagePot = 0L;
long currentLimitPot = 0L;

const int FYSNC_Pin = 27;
const int ledPin1 = 13;

long masterClock_freq = 24000000;
unsigned char currentMode = 0;
unsigned long frequency1 = 100;
unsigned long phase = 0;

char buffer[32];

//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
AD9833 sigGen(FYSNC_Pin, 24000000); 

screenMode screen = displayAll;
//waveType = 2;



void joyButtonInterupt(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > 200){
    Serial.println("JoyStick button pressed");
    if((menu[0] == 1) && ((menu[1] == 1) || (menu[1] ==2))){
      if(activated){activated = false;}
      else if (!activated){activated = true;}           
      Serial.println("Activated var toggled");
      Serial.print("Activated = ");
      Serial.println(activated);
    } 
  }
  last_interrupt_time = interrupt_time;
}

void funcButtonInterupt(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > 200){
    Serial.println("Func button pressed");
    if((menu[0] == 1) && ((menu[1] == 1) || (menu[1] ==2))){
      if(funcOn){funcOn = false;}
      else if (!funcOn){funcOn = true;}           
      Serial.println("FuncOn var toggled");
      Serial.print("FuncOn = ");
      Serial.println(funcOn);
    } 
  }
  last_interrupt_time = interrupt_time;
}

void setup(void) {
   //setup interupt pins
  pinMode(joyButPin, INPUT_PULLUP);
  pinMode(funcButPin, INPUT_PULLUP);
  //attach interupts
  attachInterrupt(digitalPinToInterrupt(joyButPin), joyButtonInterupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(funcButPin), funcButtonInterupt, FALLING);

  Serial.begin(9600);
  Serial.print("Hello! ST77xx TFT Test");

  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  tft.setRotation(1);
  Serial.println("Initialized");

  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(500);

  // large block of text
  tft.fillScreen(ST77XX_BLACK);
  //drawtext_line1("Test Line 1", ST77XX_WHITE);
  drawtext_line2("Test Line 2", ST77XX_GREEN);
  
  pinMode(ledPin1, OUTPUT);
  pinMode(FYSNC_Pin, OUTPUT);
  digitalWrite(FYSNC_Pin, HIGH);

  Serial.print("Menu [0] = ");
  Serial.println(menu[0]);
  Serial.print("Menu [1] = ");
  Serial.println(menu[1]);
}

void loop() {
  //read all the ADCs
  
  xPotVal = analogRead(potX);
  yPotVal = analogRead(potY);
  dcOffsetADC = analogRead(dcOffsetPin);
  pkVoltADC = analogRead(pkVoltPin);
  voltADC = analogRead(voltPin);
  currentLimADC = analogRead(currentLimPin);

  calcPots(dcOffsetADC, pkVoltADC, voltADC, currentLimADC);
  
  menuNav(xPotVal, yPotVal, menu);
  
  //tft.invertDisplay(true);
  //delay(500);
  //tft.invertDisplay(false);
  // digitalWrite(ledPin1, HIGH);
  // delay(500);
  // digitalWrite(ledPin1, LOW);
  // delay(500);

  if(funcOn){
    setOutput(frequency, waveType);
  }
  
  delay(1500);

  frequency1 += 500;
  if(frequency1 > 2000){
    frequency1 = 100;
  }

  ltoa(frequency1,buffer,10);
  
  //drawtext_line3(buffer, ST77XX_RED);


  //drawScreen(screen, waveType, outputVoltage, outputCurrentLimit, frequency, pkVoltage, dcOffset);


  writeScreen(menu, waveType, outputVoltage, outputCurrentLimit, frequency, pkVoltage, dcOffset);
  
  Serial.println(buffer);
  Serial.print("X pot = ");
  Serial.print("\t");
  xPotVal = analogRead(potX);
  Serial.println(xPotVal);
  Serial.print("Y pot = ");
  Serial.print("\t");
  yPotVal = analogRead(potY);
  Serial.println(yPotVal);

  
}


void calcPots(int dcOffset, int pkVolt, int volt, int currentLim){
  outputVoltage = (long)volt;
  outputCurrentLimit = (long)currentLim;
  pkVoltage = (long)pkVolt;
  dcOffset = (long)dcOffset;
}

void menuNav(int x, int y, int *m){
  int xMax = 1;
  int yMax = 2;
  int xPlus = 2000;
  int xMinus = 500;
  int yPlus = 2000;
  int yMinus = 500;

  Serial.println("In Nav Function");
  Serial.print("M [0] = ");
  Serial.println(m[0]);
  Serial.print("M [1] = ");
  Serial.println(m[1]);

  if(!activated){
    Serial.println("Not activated");
    if(x > xPlus){
       Serial.println("X Pot High");
      //increment menu x coordinate if not at max
      m[0] += 1;
      if(m[0] >= xMax){
        m[0] = xMax;
        Serial.println("Increment menu x");
      }
    }
    else if(x < xMinus){
      Serial.println("X Pot Low");
      //decrement menu x coordinate if not at minimum
      m[0] -= 1;
      if(m[0] <= 0){
        m[0] = 0;
        Serial.println("Decrement menu x");
      }
    }
    if(y > yPlus){
      Serial.println("Y Pot High");
      //increment menu y coordinate if not at max
      m[1] += 1;
      if(m[1] >= yMax){
       m[1] = yMax;
       Serial.println("Increment menu y");
      }
    }
    else if(y < yMinus){
      Serial.println("Y Pot Low");
      //decrement menu y coordinate if not at minimum
      m[1] -= 1;
      if(menu[1] <= 0){
       m[1] = 0;
       Serial.println("Decrement meny y");
      }
    }
  }

  Serial.print("M Variable x = ");
  Serial.println(m[0]);

  if(activated){
    //Joystick button has been pressed while on freq or wavetype
      Serial.println("Sub menu activated");
      Serial.print("m[1] = ");
      Serial.println(m[1]);
      Serial.print("m[2] = ");
      Serial.println(m[2]);
      if((menu[0] == 1)&&(menu[1] == 1)){
        //Select Wavetype
        Serial.print("in waveType selection");
        if((y > yPlus)||(x > xPlus)){
          waveType +=1;
          if(waveType > 2){waveType = 0;}
        }
        if((y < yMinus)||(x < xMinus)){
          waveType -= 1;
          if(waveType < 0){waveType = 2;}
        }
      }
      if((menu[0] == 1)&&(menu[1] == 2)){
        //adjust frequency
        const int xPosMin = 0;
        const int xPosMax = 4;
        const int decMax = 9;
        const int decMin = 0;
        static int xpos = 0;
        
        //Move through submenu x axis
        if(x > xPlus){
          //decrement submenu x position - inverted due to setup of submenu
          menu[2] -= 1;
          if(menu[2] < xPosMin){menu[2] = 0;}
          
        }
        if(x < xMinus){
          //increment submenu x position - inverted due to set up of submenu
          menu[2] += 1;
          if(menu[2] > xPosMax){menu[2] = 0;}
        }

        //adjust submenu components
        if(y > yPlus){
          Serial.print("Before addition subMenu[menu[2]] = ");
          Serial.println(subMenu[menu[2]]);
          subMenu[menu[2]] += 1;
          Serial.print("After addition subMenu[menu[2]] = ");
          Serial.println(subMenu[menu[2]]);
          if(subMenu[menu[2]] > decMax){subMenu[menu[2]] = decMax;} 
          Serial.print("After max subMenu[menu[2]] = ");
          Serial.println(subMenu[menu[2]]);
        }
        if(y < yMinus){
          subMenu[menu[2]] -= 1;
          if(subMenu[menu[2]] < decMin){subMenu[menu[2]] = decMin;}
        }
        Serial.print("menu[2] = ");
        Serial.println(menu[2]);
        Serial.print("subMenu[menu[2]] = ");
        Serial.println(subMenu[menu[2]]);
      }
        //Determine Freq value based on digits displayed
    int multiplyer = 1;
    int sum = 0;
    // for(int x = subMenuLeng; x > -1; x--){
    //   Serial.print("submenu ");
    //   Serial.println(x);
    //   Serial.println(subMenu[x]);
    // }

    for(int x = subMenuLeng; x > 0; x--){
      // delay(1000);
      // Serial.print("x = ");
      // Serial.println(x);

      // Serial.print("submenu[subMenuLeng - x = ");
      // Serial.println(subMenu[subMenuLeng - x]);

      // delay(1000);
      // Serial.print("Sum ");
      // Serial.println(sum);
      // Serial.print("Mulpiplayer = ");
      // Serial.println(multiplyer);
      sum = sum + (subMenu[subMenuLeng - x] * multiplyer);
      multiplyer = multiplyer * 10;
      // Serial.print("Summmmmmnmm ");
      // Serial.println(sum);
     
    }

  frequency = sum;
  Serial.print("Freq = ");
  Serial.println(frequency);
  //set new frequency
  }  


  
}


void drawtext_line1(char *text, uint16_t color) {
  //tft.setCursor(10, 0);
  //tft.setTextColor(color);
  //tft.setTextWrap(true);
  //tft.setTextSize(1.5);
  //tft.print(text);
}

void drawtext_line2(char *text, uint16_t color) {
  tft.setCursor(10, 25);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.setTextSize(1.5);
  tft.print(text);
}

void drawtext_line3(char *text, uint16_t color) {
  tft.setCursor(10, 50);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 50);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.setTextSize(1.5);
  tft.print(text);
}

void setOutput(long frequency, int waveForm){
  //digitalWrite(FYSNC_Pin, LOW);
  sigGen.reset(1);
  sigGen.setFreq(frequency);
  sigGen.setFPRegister(1);
  sigGen.mode(waveForm);
  sigGen.reset(0);
  //digitalWrite(FYSNC_Pin, HIGH);
}

void writeScreen(int *m, int waveType, long outputVolt, long currentLimit, long freq, long pkVolt, long dcOffset){
    // Serial.println("In WriteScreen");
    // Serial.print("m[0] = ");
    // Serial.println(m[0]);
    switch (m[0]){
      case 0:
        // Serial.print("m[0] = ");
        // Serial.println(m[0]);
        // Serial.println("drawing Volt screen");
        drawVoltSourceScreen(outputVolt, currentLimit);
        break;
      case 1:
        // Serial.print("m[0] = ");
        // Serial.println(m[0]);
        // Serial.println("drawing func screen");
        drawFuncGenScreen(m, waveType, freq, pkVolt, dcOffset);
        break;
    }
}

void drawVoltSourceScreen(long outputVolt, long currentLimit){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST7735_GREEN);
    tft.setTextSize(1);
    tft.setCursor(15,15);
    tft.print("Voltage");
    tft.setCursor(15,25);
    tft.print("Source");
    tft.drawRect(10,10,75,25,ST7735_RED);  
    tft.setCursor(90,15);
    tft.print("Function");
    tft.setCursor(90, 25);
    tft.print("Source");
    tft.drawRect(85,10,75,25,ST7735_WHITE);

    
    tft.setCursor(10, 55);
    tft.setTextSize(2);
    ltoa(outputVolt,buffer,10);
    tft.print("Voltage = ");
    tft.print(buffer);
    tft.setCursor(10, 85);
    ltoa(currentLimit,buffer,10);
    tft.print("Current = ");
    tft.print(buffer);
}

void drawFuncGenScreen(int *m, int waveType, long freq, long pkVolt, long dcOffset){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST7735_GREEN);
    tft.setTextSize(1);
    tft.setCursor(15,15);
    tft.print("Voltage");
    tft.setCursor(15,25);
    tft.print("Source");
    tft.drawRect(10,10,75,25,ST7735_WHITE);  
    tft.setCursor(90,15);
    tft.print("Function");
    tft.setCursor(90, 25);
    tft.print("Source");
    tft.drawRect(85,10,75,25,ST7735_RED);

    tft.setCursor(10, 45);
    tft.setTextSize(1);
    ltoa(pkVolt,buffer,10);
    tft.print("Pk-Pk Voltage = ");
    tft.print(buffer);

    tft.setCursor(10, 60);
    ltoa(dcOffset,buffer,10);
    tft.print("DC Offset = ");
    tft.print(buffer);
   
    tft.setCursor(10, 75);
    tft.print("Wave Type = ");
    // Serial.print("Wave type int =");
    // Serial.println(waveType);
    switch(waveType){
      case 0:
        tft.print("Square");
        break;
      case 1:
        tft.print("Sine");
        break;
      case 2:
        tft.print("Triangle");
        break;
    }

    tft.setCursor(10,90);
    tft.print("Frequency = ");
    sprintf(buffer, "%0.5d", freq);
    tft.print(buffer);
    


    //Red Boxes for Freq and Wave select (Menu[2] - 0 = none selected, 1 = Wave selected, 2 = Freq selected)
    
    Serial.println("Determine Y menu pos");
    Serial.print("m[1] = ");
    Serial.println(m[1]);
    if(!activated){
      switch(m[1]){
        case 0:
          //nothing selected
          Serial.println("Do nothing");
          break;
        case 1:
         //WaveType selected
         Serial.println("Draw rec around Wave Type");
         tft.drawRect(5, 74, 62, 12, ST7735_RED);
         break;
        case 2:
          //Frequency Selected
          Serial.println("Draw Rec around Freq");
          tft.drawRect(5, 89, 62, 12, ST7735_RED);
          break;
      }
    }
    if(activated && (m[1] == 1)){
      //Changing WaveType
      tft.drawRect(75,74,62,12, ST7735_RED);
    }
    if(activated && (m[1]) == 2){
      //draw rect around indiviual digits
      int xoffset = 80 + 24 - (6*m[2]);
      tft.drawRect(xoffset, 89, 8, 11, ST7735_RED);
    }
}

void drawScreen(screenMode screenMode, int waveType, long outputVolt, long currentLimit, long freq, long pkVolt, long dcOffset){
  if (screenMode == displayAll){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST7735_GREEN);
    tft.setTextSize(1);
    tft.setCursor(15,15);
    tft.print("Voltage");
    tft.setCursor(15,25);
    tft.print("Source");
    tft.drawRect(10,10,75,25,ST7735_RED);  
    tft.setCursor(90,15);
    tft.print("Function");
    tft.setCursor(90, 25);
    tft.print("Source");
    tft.drawRect(85,10,75,25,ST7735_WHITE);

    
    // tft.setCursor(10, 55);
    // tft.setTextSize(2);
    // ltoa(outputVolt,buffer,10);
    // tft.print("Voltage = ");
    // tft.print(buffer);
    // tft.setCursor(10, 85);
    // ltoa(currentLimit,buffer,10);
    // tft.print("Current = ");
    // tft.print(buffer);

    tft.setCursor(10, 45);
    tft.setTextSize(1);
    ltoa(pkVolt,buffer,10);
    tft.print("Pk-Pk Voltage = ");
    tft.print(buffer);

    tft.setCursor(10, 60);
    ltoa(dcOffset,buffer,10);
    tft.print("DC Offset = ");
    tft.print(buffer);
   
    tft.setCursor(10, 75);
    tft.print("Wave Type = ");
    switch(waveType){
      case 0:
        tft.print("Square");
        break;
      case 1:
        tft.print("Sine");
        break;
      case 2:
        tft.print("Triangle");
        break;
    }
    tft.setCursor(10, 75);
    tft.drawRect(5, 74, 62, 12, ST7735_RED);

    tft.setCursor(10,90);
    tft.print("Frequency = ");
    //ltoa(freq,buffer,10);
    sprintf(buffer, "%0.5d", freq);
    tft.printf(buffer);
  
    tft.drawRect(5, 89, 62, 12, ST7735_RED);

  }
  else if (screenMode == displayFunc){
    
  }
  else if (screenMode == displayFuncMenu){
    
  }
}
