#define DIR_PIN 9
#define STEP_PIN 8
#define RESET_PIN 7
#define SLEEP_PIN 6
#define MICROSTEP_PIN 4

#define TRACKBTN_PIN 12
#define PARKBTN_PIN 11
#define LIMITBTN_PIN 10

#define RA_PLUS_PIN A0
#define RA_MINUS_PIN A1

#define LED_PIN 13

#define ARM_LENGTH 0.2
#define A_ZERO 0
#define ROD_THREAD_STEP 1.25
#define ROD_MAX_LENGTH 0.25
#define STEPS_PER_REV 800
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
float trackSpeed = 7.292e-05;
float guideRate = 0.5;
float w = trackSpeed;
float wPlus = trackSpeed+trackSpeed*guideRate;
float wMinus = trackSpeed-trackSpeed*guideRate;

int currentStep = 0;
unsigned long maxStepCount = ROD_MAX_LENGTH/ROD_THREAD_STEP*STEPS_PER_REV;

unsigned long stepDelay = 0;
unsigned long stepLastTime = 0;

unsigned long ledDelay = 500;
unsigned long ledLastTime = 0;
int ledState = HIGH;
boolean ledBlinkMode = false;

unsigned long parkDelay = 1000;
unsigned long parkLastTime = 0;

unsigned long prepareStartDelay = 1000;
unsigned long prepareStartLastTime = 0;

int guidingState = 0;

boolean inPrepareStartMode = false;
boolean inTrackMode = false;
boolean inPauseMode = false;
boolean inParkMode = false;
boolean btnsDisabled = false;

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
  pinMode(RA_PLUS_PIN, INPUT_PULLUP);     
  pinMode(RA_MINUS_PIN, INPUT_PULLUP);  
  digitalWrite(DIR_PIN, LOW); 
  digitalWrite(MICROSTEP_PIN, HIGH); 
  Serial.begin(9600);
  
  if (digitalRead(LIMITBTN_PIN) != LOW) {
    digitalWrite(RESET_PIN, HIGH);  
    digitalWrite(SLEEP_PIN, HIGH);
    inParkMode = true;
    //set led to blink every 1/10 second
    ledBlinkMode = true;
    ledDelay = 100;
  }
  
}

void loop() {

  if (!btnsDisabled) { //Если кнопки не заблокированы, т.е. не в режиме парковки

    if (inTrackMode) {
    
      //читаем гидируещие сигналы
      if (digitalRead(RA_PLUS_PIN) == LOW) {
        if (guidingState != 1) {
          t = w*t/wPlus;
          startTime = A_ZERO/wPlus;
          w = wPlus;
          guidingState = 1;
        }
      } else if (digitalRead(RA_MINUS_PIN) == LOW) {
        if (guidingState != -1) {
          t = w*t/wMinus;
          startTime = A_ZERO/wMinus;
          w = wMinus;
          guidingState = -1;
        }
      } else {
        if (guidingState != 0) {
          t = w*t/trackSpeed;
          startTime = A_ZERO/w;
          w = trackSpeed;
          guidingState = 0;
        }
      }
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

    } else if (!inPrepareStartMode) { //Если монти ни в каком режиме, т.е. стартовая позиция

      int trackBtnread = digitalRead(TRACKBTN_PIN);

      if (trackBtnread != trackBtnLastState) {
        trackBtnLastTime = millis();
      }

      if ((millis() - trackBtnLastTime) > debounceDelay) {
        if (trackBtnread == LOW) {
          Serial.print("Pause pressed in no mode\n");
          if (!trackBtnIsPressed) {
            trackBtnIsPressed = true;
            inPrepareStartMode = true;
            prepareStartLastTime = millis();
            //set led to blink every second
            ledBlinkMode = true;
            ledDelay = 1000; 
            //switch motor to active mode
            digitalWrite(SLEEP_PIN, HIGH);
            digitalWrite(RESET_PIN, HIGH);  
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
  
  if (inPrepareStartMode && millis()-prepareStartLastTime > prepareStartDelay) {
    inPrepareStartMode = false;
    inTrackMode = true;
    startTime = millis() + DEFAULT_TIME;
    stepDelay = 1000000/(0.2*w*cos(w*DEFAULT_TIME/2000))/800*0.00125;
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
      inParkMode = false;
      inPauseMode = false;
      inTrackMode = false;
      btnsDisabled = false;
      parkLastTime = 0;
      stepLastTime = 0;
      //set led to light constantly
      ledState = HIGH;				
      ledBlinkMode = false;
      //switch motor to sleep mode
      digitalWrite(SLEEP_PIN, LOW);
      digitalWrite(RESET_PIN, LOW);  
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
        stepDelay = 1000000/(ARM_LENGTH*w*cos(w*t/2000))/800*0.00125;
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
