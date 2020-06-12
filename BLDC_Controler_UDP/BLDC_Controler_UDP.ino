//#######################################
//#######################################
// GPIO mappings for Arduino Nano
//#######################################
int m1_Signal_hall = 2; // Signal - Hall sensor
int m1_VR_speed = 6;  //VR   6
int m1_ZF_Direction = 4; // ZF
int m1_EL_Start_Stop = 5; //EL

int m12_Signal_hall = 3; // Signal - Hall sensor
int m12_VR_speed = 9;  //VR
int m12_ZF_Direction = 8; // ZF
int m12_EL_Start_Stop = 10; //EL
//#######################################
//#######################################
#define COUNTERLIMIT  1

const int UPLIMIT = 100;
const int DWLIMIT = -100;

int lspeed = 0;
int rspeed = 0;

volatile int lpos = 0;
volatile int rpos = 0;

int lmaxpos = 0;
int rmaxpos = 0;

float  ldiff = 0;
float rdiff = 0;
volatile bool change = true;


bool FF = false;
//#######################################
//#######################################

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
unsigned long period = 1000;  //the value is a number of milliseconds

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {

  // put your setup code here, to run once:
  Serial.begin(9600);

  //wheel 1 - Setup pin mode
  pinMode(m1_EL_Start_Stop, OUTPUT);//stop/start - EL
  pinMode(m1_Signal_hall, INPUT);   //plus       - Signal
  pinMode(m1_ZF_Direction, OUTPUT); //direction  - ZF

  //wheel 2 - Setup pin mode
  pinMode(m12_EL_Start_Stop, OUTPUT);//stop/start - EL
  pinMode(m12_Signal_hall, INPUT);   //plus       - Signal
  pinMode(m12_ZF_Direction, OUTPUT); //direction  - ZF

  //Hall sensor detection - Count steps
  attachInterrupt(digitalPinToInterrupt(m1_Signal_hall), lplus, CHANGE);
  attachInterrupt(digitalPinToInterrupt(m12_Signal_hall), rplus, CHANGE);

  startMillis = millis();  //initial start time
}

void lplus() {
  lpos++;
  change = true;
}
void rplus() {
  rpos++;
  change = true;
}

int LSpeed = 0;
int RSPeed = 0;
int counter = 0;

void wheelStop() {
  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m12_EL_Start_Stop, LOW);
}


void LWheelMove(int LwheelSpeed) {
  LwheelSpeed = limit(LwheelSpeed);
  if (LwheelSpeed > 0) {
    analogWrite(m1_VR_speed, LwheelSpeed);
    digitalWrite(m1_ZF_Direction, LOW);
  } else if (LwheelSpeed < 0) {
    analogWrite(m1_VR_speed, -1 * LwheelSpeed);
    digitalWrite(m1_ZF_Direction, HIGH);
  } else   if (LwheelSpeed == 0) {
    LWheelStop();
    return;
  }
  digitalWrite(m1_EL_Start_Stop, HIGH);
}

void RWheelStop() {
  analogWrite(m12_VR_speed, 0);
  digitalWrite(m12_EL_Start_Stop, LOW);
}
void RWheelMove(int RwheelSpeed) {
  RwheelSpeed = limit(RwheelSpeed);
  if (RwheelSpeed > 0) {
    analogWrite(m12_VR_speed, RwheelSpeed);
    digitalWrite(m12_ZF_Direction, HIGH);
  } else if (RwheelSpeed < 0) {
    analogWrite(m12_VR_speed, -1 * RwheelSpeed);
    digitalWrite(m12_ZF_Direction, LOW);
  } else   if (RwheelSpeed == 0) {
    RWheelStop();
    return;
  }
  digitalWrite(m12_EL_Start_Stop, HIGH);
}

void LWheelStop() {
  analogWrite(m1_VR_speed, 0);
  digitalWrite(m1_EL_Start_Stop, LOW);
}

int limit(int originalvalue) {
  if (originalvalue >= UPLIMIT)
    return UPLIMIT;
  if (originalvalue <= DWLIMIT)
    return DWLIMIT;
  return originalvalue;
}


void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '{') {
      inputString = "";
      continue;
    }
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void loop() {
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (stringComplete) {
    Serial.println(inputString);
//    if (inputString.length() > 2 && !FF)
//    {
//      if (inputString[0] == 'F' && inputString[1] == 'F' && inputString[2] == '}') {
//        FF = true;
//        lpos = 0; rpos = 0; ldiff =0; rdiff = 0;
//        Serial.print("Mode FF");
//        lspeed = 35; rspeed = 35;
//        LWheelMove(50); RWheelMove(50);
//      }
//    }
    if (inputString != "" && inputString.length() > 6) {
      if ((inputString[0] >= 48 && inputString[0] <= 57) || (inputString[0] == 45 && inputString[1] >= 48 && inputString[1] <= 57))
      {
        int x = getValue(inputString, ':', 0).toInt();
        int y = getValue(inputString, ':', 1).toInt();
        //Serial.print(inputString);
        Serial.print(">");
        Serial.print(x);
        Serial.print("!");
        Serial.print(y);
        Serial.println();
        counter = 0;
        // MODE FF
        if (x == 0 & y == 0 && FF)
        {
          LWheelMove(0); RWheelMove(0);
          FF = false;
        }

        //Mode Normal
        if (!FF) {
          LWheelMove(x); RWheelMove(y);
        }

        if ((x < 0 && y > 0) || (x > 0 && y < 0)) {
          period = 250;
        } else {
          period = 750;
        }
      }
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    if (counter <= COUNTERLIMIT)
      counter++;
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  if (counter > COUNTERLIMIT ) {
    counter = 0;
    if (!FF) {
      LWheelMove(0); RWheelMove(0);
      Serial.println("Stop");
    }


  }


  //  if (change && FF) {
  //    if (lpos < 50 || rpos < 50) {
  //      return;
  //    }
  //    if (lpos > rpos) {
  //      LWheelMove(lspeed);lspeed=0;
  //      lmaxpos = lpos;
  //    } else {
  //      rmaxpos = rpos;
  //      RWheelMove(rspeed);rspeed=0;
  //    }
  //
  //    if(lspeed==0 && rspeed==0) FF= false;
  //    change = false;
  //        Serial.print(lpos);
  //        Serial.print(":");
  //        Serial.print(rpos);
  //        Serial.print(":");
  //        Serial.print(lmaxpos);
  //        Serial.print(":");
  //        Serial.print(rmaxpos);
  //        Serial.println();
  //  }
  if (change && FF) {
    if (lpos < (rpos + 10)) {
      ldiff = rpos - lpos;
      ldiff = ldiff * 2.2;
      if (rspeed > ldiff + 10) {
        rspeed = rspeed - ldiff; 
        lspeed = lspeed + ldiff;
      }
    } else if (rpos < (lpos + 10))

    {
      rdiff = lpos - rpos;
      rdiff = rdiff * 2.2;
      if (lspeed > rdiff + 10) {
        lspeed = lspeed - rdiff; 
        rspeed = rspeed + rdiff;
      }
    }

    LWheelMove(lspeed);
    RWheelMove(rspeed);
    change = false;
        Serial.print(lpos);
        Serial.print(":");
        Serial.print(rpos);
        Serial.print(":");
        Serial.print(lspeed);
        Serial.print(":");
        Serial.print(rspeed);
        Serial.println();
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//"
//"direction":"forward","steps":"500","speed":"20","speed2":"20"}
