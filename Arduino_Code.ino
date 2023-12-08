//LCD dash firmware made by P-rth 7/12/23
//
/*

Curcuit :

A7 --> 5.6k ohms --> Groud
Reset --> 22uf capacitor --> switch -- Groud  (turn off to upload code)

LDR (+) --> Vin / 5v
LDR (-) --> A7

Backlight + (LCD A) --> D10
Backlight - (LCD K) --> Groud


LCD RS --> 12
LCD Enable --> 11
LCD D4 --> 5
LCD D5 --> 4
LCD D6 --> 3
LCD D7 --> 2

*/


#include <LiquidCrystal.h>
#include <LcdProgressBarDouble.h>
#include <Smooth.h>

//============================
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing


char appFromPC[20] = {0};
char oldappFromPC[20] = {0};

char timeFromPC[numChars] = {0};
int dayperFromPC = 0;
int cpuFromPC = 0;                    // variables to hold the parsed data
int memFromPC = 0;
boolean newData = false;



//============================

int backlight=10;
int y;                                              // y is the real time backlight brightess using ldr
int backlight_bright = 150;                        // startup bright
int inputPin = A7;
#define  SMOOTHED_SAMPLE_SIZE  200
Smooth  average(SMOOTHED_SAMPLE_SIZE);                // Smoothing average object
int sensorValue1 = 0;
int sample = 0;                                       // Simulated moving sample
int sensorMin = 0;                                    // 100% sensor value
int sensorMax = 250;                                  // 0% sensor value
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);                // creating LCD instance
float bright_steps = 0;


//=================================================================

LcdProgressBarDouble lpg_day_bright(&lcd, 1, 8, 9);  // -- creating Object lenght 8 row 2 col 9
LcdProgressBarDouble lpg_cpu_mem(&lcd, 1, 8);  // -- creating

//=================================================================

unsigned long interval_timeout = 5000;
unsigned long previous_serial_Millis = 0;


#include <SoftwareReset.hpp>
//==================================================================

bool FirstStart = true;
int aniframe_num = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
const long interval = 100;
const long interval1 = 50;
int total_frames = 17;
bool anifwd = true;
String aniframe[17] =           { "|               ", 
                                  " \x01              ",
                                  "  -             ",
                                  "   /            ",
                                  "    |           ",
                                  "     \x01          ",
                                  "      -         ",
                                  "       /        ",                                  //loading animation frames
                                  "        |       ",
                                  "         \x01      ",
                                  "          -     ",
                                  "           /    ",
                                  "            |   ",
                                  "             \x01  ",
                                  "              - ",
                                  "               /",
                                  "               |", };

byte customBackslash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};

byte d_pg_char[8] = {
			B00000,
			B00000,
			B00000,
			B00000,
			B00000,
			B11000,
			B11000,
			B00000
		};



//===============================================================

//================================================================
void setup() {
    Serial.begin(2000000);
    Serial.println("Enter data in format: <time[8],app[8],day[0-100],cpu[0-100],mem[0-100]>");           //start serial
    
    
    lcd.begin(2,16);


    lpg_day_bright.setMinValues(0);
    lpg_cpu_mem.setMinValues(0);

    lpg_day_bright.setMaxValues(100);
    lpg_cpu_mem.setMaxValues(100);
    lcd.createChar(1, customBackslash);
    analogWrite(6,0);                 //cotrast pin
}
//===============================================================
char oldtempChars[numChars];

bool lcd_sens_updates = true; 

void loop() {
    recvWithStartEndMarkers();
    
    if (FirstStart == false){
      takereading();

      if (lcd_sens_updates == true){
        lpg_day_bright.draw(dayperFromPC, sensorValue1); 
      }

      y = map(sensorValue1, 0, 100, 50, 255);
      analogWrite(backlight, y);
    }

    if (newData == true) {  
      if (oldtempChars != tempChars) {
        newdata();
        strcpy(oldtempChars , tempChars);
      }
    }

    else if (FirstStart == true && newData == false){

      loadinganimation();    

    }
    
    if (FirstStart == false && newData == false){
      unsigned long cMillis = millis();

      if (cMillis - previous_serial_Millis >= interval_timeout) {
       // Serial.println("Resetting");
        

        bright_steps = (150 - y)/16;                 //150 is the brightess target as it is the brightess o startup

        for (byte j = 0; j < 16; ++j) {
            y = y+bright_steps;
            delay(100);
            analogWrite(backlight,y);
            lcd.scrollDisplayRight();
        }

        softwareReset::simple();

     }

    }

}

//================================================================

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//================================================================



void parseData() {      // split the data into its parts
    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(timeFromPC, strtokIndx); 

    strtokIndx = strtok(NULL,",");      //secod part of the string - the app
    strcpy(appFromPC, strtokIndx); 

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    dayperFromPC = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    cpuFromPC = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    memFromPC = atoi(strtokIndx);     // convert this part to an integer
} 

//================================================================

void showParsedData() {
    Serial.print("timeFromPC : ");
    Serial.println(timeFromPC);
    Serial.print("appFromPC : ");
    Serial.println(appFromPC);
    Serial.print("day%FromPC : ");
    Serial.println(dayperFromPC);

    Serial.print("cpuFromPC : ");
    Serial.println(cpuFromPC);
    Serial.print("memFromPC : ");
    Serial.println(memFromPC);
}

//==================================================================

void takereading() {
    sample = analogRead(inputPin);
    average += sample;

    sensorValue1 = constrain((int) average(), sensorMin, sensorMax);
    sensorValue1 = map(sensorValue1, sensorMin, sensorMax, 0, 100);

    if (newData == true) {
      Serial.println(sensorValue1);
    }
}
//=================================================================
class RepeatedString : public Printable
{
public:
    RepeatedString(const char * str, unsigned int count)
    : _str(str), _count(count) {}
    size_t printTo(Print& p) const
    {
        size_t sz = 0;
        for (unsigned int i = 0; i < _count; i++)
            sz += p.print(_str);
        return sz;
    }
private:
    const char * _str;
    unsigned int _count;
};
//=================================================================

void updatelcd() {
    //lcd.clear();
    lpg_cpu_mem.draw(cpuFromPC,memFromPC);

    if (strlen(appFromPC) < 11)
    {
      lcd_sens_updates = true;
      lcd.setCursor(11,0);
      lcd.print(timeFromPC);
      lpg_day_bright.draw(dayperFromPC, sensorValue1);
    }
    else if (strlen(appFromPC) >= 11){
      if (lcd_sens_updates != false) {
        lcd_sens_updates = false;
        lcd.setCursor(11,0);
        lcd.print("     ");
        lcd.setCursor(9,1);                   //clear previous text for new layout
        lcd.print("       ");
      }
      lcd.setCursor(10,1);
      lcd.print(timeFromPC);
    }
  
    if (strcmp(oldappFromPC,appFromPC) != 0)  {

      lcd.home();
      lcd.print(RepeatedString(" ",strlen(oldappFromPC)));    //mask the old text

      strcpy(oldappFromPC,appFromPC );
      lcd.home();
      lcd.print(appFromPC);
    }

    lcd.setCursor(8,1);
    lcd.print("|");

}

//==================================================================


void loadinganimation(){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // if frame less than total_frames add frame else set to 0
    if (anifwd == true){
      if (aniframe_num < total_frames) {
        aniframe_num++;
      } else {
        anifwd = false;
      }
    }
    else{
      if (aniframe_num > 0) {
        aniframe_num--;
      } else {
        anifwd = true;
      }
    }


    lcd.home();
    lcd.print("[- Connecting -]");
    lcd.setCursor(0, 1);
    lcd.print(aniframe[aniframe_num]);

  }


  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;

    if (backlight_bright > 5){
      analogWrite(backlight,backlight_bright);
      backlight_bright = backlight_bright-1;
    }
    


  }
}


//==================================================================

void newdata(){
    previous_serial_Millis = millis();


    strcpy(tempChars, receivedChars);
        // this temporary copy is necessary to protect the original data
        //   because strtok() used in parseData() replaces the commas with \0
    parseData();
    //showParsedData();

    if (FirstStart == true){
      takereading();
      y = map(sensorValue1, 0, 100, 50, 255);
      bright_steps = (y - backlight_bright)/16;


      for (byte j = 0; j < 16; ++j) {
          backlight_bright = backlight_bright+bright_steps;
          delay(100);
          analogWrite(backlight,backlight_bright);
          lcd.scrollDisplayLeft();
      }


      lcd.createChar(1, d_pg_char);
      lcd.clear();
      analogWrite(6,30);                 //cotrast pin
      FirstStart = false;
    }

    updatelcd();
    newData = false;
}
