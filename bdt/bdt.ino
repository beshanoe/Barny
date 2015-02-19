#define DIR_PIN 9
#define STEP_PIN 8
#define RESET_PIN 7
#define SLEEP_PIN 6
#define MICROSTEP_PIN 4

#define TRACKBTN_PIN 12
#define PARKBTN_PIN 11
#define LIMITBTN_PIN 10

#define LED_PIN 13

#define tL1 0.2
#define tL2 0.2
#define tS0 0.02
#define tA0 0
#define ROD_MAX_LENGTH 0.2
#define PERIOD_VA 600.0
#define DEFAULT_TIME 0

boolean trackBtnIsPressed = false;
boolean parkBtnIsPressed = false;
boolean btn3IsPressed = false;

boolean trackBtnLastState;
boolean parkBtnLastState;
boolean btn3LastState;

unsigned long trackBtnLastTime = 0;
unsigned long parkBtnLastTime = 0;
unsigned long btn3LastTime = 0;

boolean lastStepPin = HIGH;

long debounceDelay = 50; 

unsigned long t = 0;
unsigned long startTime = 0;
float w = 7.292e-05;
float currentAngle = 0;//acos((tL1*tL1 + tL2*tL2 - tS0*tS0)/2*tL1*tL2);
int currentStep = 0;
unsigned long maxStepCount = ROD_MAX_LENGTH/1.25*1600;
float currentLength = 0;
float newLength = 0;

unsigned long stepDelay = 0;
unsigned long stepLastTime = 0;

unsigned long outerLoopDelay = PERIOD_VA*1000;
unsigned long outerLastTime = -outerLoopDelay;

unsigned long ledDelay = 500;
unsigned long ledLastTime = 0;
int ledState = HIGH;
boolean ledBlinkMode = false;

unsigned long parkDelay = 1000;
unsigned long parkLastTime = 0;

boolean inTrackMode = false;
boolean inPauseMode = false;
boolean inParkMode = false;
boolean btnsDisabled = false;
boolean isMountParked = false;

float mycos(float x) {
  return - 2.21941782786353727022e-07*pow(x,10) + 2.42532401381033027481e-05*pow(x,8)
    - 1.38627507062573673756e-03*pow(x,6) + 4.16610337354021107429e-02*pow(x,4)
      - 4.99995582499065048420e-01*pow(x,2) + 9.99999443739537210853e-01;
}

void setup() {                
  pinMode(DIR_PIN, OUTPUT);       
  pinMode(STEP_PIN, OUTPUT);   
  pinMode(RESET_PIN, OUTPUT);       
  pinMode(SLEEP_PIN, OUTPUT);       
  pinMode(MICROSTEP_PIN, OUTPUT);  
  pinMode(LED_PIN, OUTPUT);     
  pinMode(TRACKBTN_PIN, INPUT_PULLUP);     
  pinMode(PARKBTN_PIN, INPUT_PULLUP);      
  pinMode(LIMITBTN_PIN, INPUT_PULLUP);  
  digitalWrite(DIR_PIN, LOW); 
  digitalWrite(RESET_PIN, HIGH); 
  digitalWrite(SLEEP_PIN, HIGH); 
  digitalWrite(MICROSTEP_PIN, HIGH); 
  Serial.begin(9600);
}

void loop() {

  if (!btnsDisabled) { //Если кнопки не заблокированы, т.е. не в режиме парковки

    if (inTrackMode) {
      //В режиме ведения воспользоваться можно только кнопкой "Пауза"
      int trackBtnread = digitalRead(TRACKBTN_PIN);

      if (trackBtnread != trackBtnLastState) {
        trackBtnLastTime = millis();
      }

      if ((millis() - trackBtnLastTime) > debounceDelay) {
        if (trackBtnread == LOW) {
          if (!trackBtnIsPressed) { //Если была нажата кнопка "Пауза"
            Serial.print("Pause pressed in track\n");
            trackBtnIsPressed = true;

            inPauseMode = true; //Перевести
            //btnsDisabled = true;
            //set led to light constantly
            ledState = HIGH;				
            ledBlinkMode = false;
          } 
        } 
        else {
          trackBtnIsPressed = false;
        }
      }
      trackBtnLastState = trackBtnread;

    } else if (inPauseMode) {
      //В режиме паузы воспользоваться можно кнопками "Продолжить ведение" и "Припарковать"

      int trackBtnread = digitalRead(TRACKBTN_PIN);

      if (trackBtnread != trackBtnLastState) {
        trackBtnLastTime = millis();
      }

      if ((millis() - trackBtnLastTime) > debounceDelay) {
        if (trackBtnread == LOW) {
          if (!trackBtnIsPressed) { //Если была нажата кнопка "продолжить ведение"
            Serial.print("Pause pressed in pause\n");
            trackBtnIsPressed = true;
            inPauseMode = false;
            inTrackMode = true;
            startTime = millis() - t;
            //set led to blink every second
            ledBlinkMode = true;
            ledDelay = 1000;
            //set 1/8 microstepping mode
            digitalWrite(MICROSTEP_PIN, HIGH);
            //set motor direction
            digitalWrite(DIR_PIN, HIGH);
          } 
        } else {
          trackBtnIsPressed = false;
        }
      }

      int parkBtnread = digitalRead(PARKBTN_PIN);

      if (parkBtnread != parkBtnLastState) {
        parkBtnLastTime = millis();
      }

      if ((millis() - parkBtnLastTime) > debounceDelay) {
        if (parkBtnread == LOW) {
          if (!parkBtnIsPressed) {
            Serial.print("Park pressed in pause\n");
            parkBtnIsPressed = true;
            inPauseMode = false;
            inParkMode = true;
            btnsDisabled = true;
            //set led to blink every 1/10 second
            ledBlinkMode = true;
            ledDelay = 100;
            //set 1/2 microstepping mode
            //digitalWrite(MICROSTEP_PIN, LOW);
            //set motor direction
            digitalWrite(DIR_PIN, LOW);
          } 
        } 
        else {
          parkBtnIsPressed = false;
        }
      }

      trackBtnLastState = trackBtnread;
      parkBtnLastState = parkBtnread;

    } else { //Если монти ни в каком режиме, т.е. стартовая позиция

      int trackBtnread = digitalRead(TRACKBTN_PIN);

      if (trackBtnread != trackBtnLastState) {
        trackBtnLastTime = millis();
      }

      if ((millis() - trackBtnLastTime) > debounceDelay) {
        if (trackBtnread == LOW) {
          Serial.print("Pause pressed in no mode\n");
          if (!trackBtnIsPressed) {
            trackBtnIsPressed = true;
            inTrackMode = true;
            //set led to blink every second
            ledBlinkMode = true;
            ledDelay = 1000;
            startTime = millis() + DEFAULT_TIME;
            stepDelay = 1000000/(0.2*w*cos(w*DEFAULT_TIME/2000))/800*0.00125;
            //set 1/8 microstepping mode
            digitalWrite(MICROSTEP_PIN, HIGH);
            //set motor direction
            digitalWrite(DIR_PIN, HIGH);
          } 
        } else {
          trackBtnIsPressed = false;
        }
      }

      trackBtnLastState = trackBtnread;

    }

  }

  if (inParkMode) {
    if (digitalRead(LIMITBTN_PIN) != LOW) {
      if ((micros() - parkLastTime) > parkDelay) {
        digitalWrite(STEP_PIN, HIGH);
        digitalWrite(STEP_PIN, LOW);
        parkLastTime = micros();
      }
    } else {
      Serial.print("Limit btn high\n");
      currentStep = 0;
      currentLength = tS0;
      currentAngle = 0;
      inParkMode = false;
      inPauseMode = false;
      inTrackMode = false;
      btnsDisabled = false;
      parkLastTime = 0;
      stepLastTime = 0;
      //set led to light constantly
      ledState = HIGH;				
      ledBlinkMode = false;
    }
  }
  
  if (inTrackMode) {
    t = millis() - startTime;
    if ((micros() - stepLastTime) > stepDelay) {
      if (inPauseMode) { //Если ведение прервано паузой, запомнить время внешнего цикла, чтобы потом вернуться к нему
        inTrackMode = false;
        stepLastTime = 0;
      } else {
        digitalWrite(STEP_PIN, HIGH);
        digitalWrite(STEP_PIN, LOW);
        stepLastTime = micros(); 
        currentStep++;
        stepDelay = 1000000/(0.2*w*cos(w*t/2000))/800*0.00125;
      }
    }
  }
  if (ledBlinkMode) {
    if ((millis() - ledLastTime) > ledDelay) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      ledLastTime = millis();
    }
  } else {
    digitalWrite(LED_PIN, ledState);		
  }

}

