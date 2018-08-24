// Include the math library for exp and log
#include <math.h>

// setup pins for motor control
const int pinMotorSpeed = 3;

const int pinMotorDir = 12;

// setup pin for type of control (manual/auto)
const int pinSelectMode = 4;

// Setup pin for manual speed control
const int pinManualControl = A3;
int motorSpeed;

// Setup pin for start automatic run
const int pinStartAuto = 2;

// Setup pins for LEDs
const int ledManual = 6;
const int ledAuto = 5;
const int ledPower = 7;
const int ledRunning = 10;

// Setup booleans for running & first run
boolean flagFirstRun = true;
boolean flagRunning = false;
int counterDebounce = 0;
boolean flagMode;  //True = manual

// refresh frequency
int samplingFreq = 50;

// millisecond counters
unsigned long currentMillis;
unsigned long lastMillis = 0;
unsigned long startMillis = 0;
int stateLedRun = LOW;

//Define automatic running parameters
float x0 = 100.0;   // motor start voltage
float xf = 245.0;   // motor end voltage
float xCutoff = 20000.0;    //Transition/cutoff point
float R = 20;       // Smoothness parameter
float k1 = (xf-x0)/xCutoff;   // First slope (set below)
float k2 = 0;       // Second slope
int speedAuto;      // Output speed
int timeRunning;    // Time of run (millis - start)

void setup() {
  // put your setup code here, to run once:
  delay(1000);

  // Park and disable motor B
  analogWrite(11,0);

  // set motor control pins to output
  pinMode(pinMotorDir, OUTPUT);

  // Disable motor brake
  pinMode(9,OUTPUT);
  digitalWrite(9,LOW);

  // set mode select switch
  pinMode(5, INPUT);

  // Setup pins for LEDs
  pinMode(ledManual, OUTPUT);
  digitalWrite(ledManual, LOW); // start off
  pinMode(ledAuto, OUTPUT);
  digitalWrite(ledAuto, LOW);  // start off
  pinMode(ledPower, OUTPUT);
  digitalWrite(ledPower, HIGH);  //start on
  pinMode(ledRunning, OUTPUT);
  digitalWrite(ledRunning, LOW);

  // Initialize mode flag
  if (digitalRead(pinSelectMode) == HIGH)
  {
    flagMode = true;
  }
  else
  {
    flagMode = false;
  }

  // set motor speed to zero
  analogWrite(pinMotorSpeed, 0);
  
  // Begin serial communications
  Serial.begin(9600);

  // Convert frequency to millis
  samplingFreq = 1000/samplingFreq;
}

void loop() {
  // check mode selector state (voltage == Manual)
  // Manual execution
  if (digitalRead(pinSelectMode) == HIGH)
  {
    // Set mode LEDs
    digitalWrite(ledManual, HIGH);
    digitalWrite(ledAuto, LOW);
    
    // Handling change of modes
    if (flagMode == false)
    {
      flagMode = true;
      flagRunning = false;
      digitalWrite(ledRunning, LOW);
      flagFirstRun = true;
      // set motor speed to zero
    }
    // If changed to manual without zeroing pot, force zeroed pot
    if(analogRead(pinManualControl) > 10 && flagFirstRun == true)
    {
      Serial.println("MANUAL, RESET CONTROLLER TO ZERO");
      analogWrite(pinMotorSpeed, 0);
      
      currentMillis = millis();
      if((currentMillis - lastMillis) > 750)
      {
        if(stateLedRun == HIGH)
        {
          stateLedRun = LOW;
        }
        else
        {
          stateLedRun = HIGH;
        }
        digitalWrite(ledRunning,stateLedRun);
        lastMillis = currentMillis;
      }
      
    }
    // Set pot deadzone to be 10(/1023)
    else if(analogRead(pinManualControl) < 10)
    {
      Serial.println("MANUAL, MOTOR PARKED");
      flagFirstRun = false;
      flagRunning = true;
      digitalWrite(ledRunning, LOW);
      analogWrite(pinMotorSpeed, 0);
    }
    // convert manual reading to 255 pwm control for motor
    else if(analogRead(pinManualControl) > 10 && flagFirstRun == false)
    {
      flagFirstRun = false;
      digitalWrite(ledRunning, HIGH);
      Serial.print("MANUAL, SPEED CONTROL VALUE:  ");
      motorSpeed = 
      motorSpeed = float(analogRead(pinManualControl)-10)/1013.0*(xf-x0)+x0;
      Serial.println(motorSpeed);
      analogWrite(pinMotorSpeed, motorSpeed); 
    }
  }

  // BEGIN AUTOMATIC EXECUTION
  else 
  {
    // Change mode LED
    digitalWrite(ledManual, LOW);
    digitalWrite(ledAuto, HIGH);
    
    // Handling change of modes
    if (flagMode == true)
    {
      flagMode = false;
      flagRunning = false;
      digitalWrite(ledRunning, LOW);
      flagFirstRun = true;
      // set motor speed to zero
    }

    // Logic to handle how run is started with button
    if(digitalRead(pinStartAuto) == LOW && flagRunning == false)
    {
      Serial.println("AUTOMATIC, NOT RUNNING");
      counterDebounce = 0;
      // Ensure motor is parked
      analogWrite(pinMotorSpeed, 0);
    }
    else if(digitalRead(pinStartAuto) == LOW && flagRunning == true && (millis()-startMillis < 30000))
    {
      Serial.print("AUTOMATIC, EXECUTING,  SPEED = ");
      counterDebounce++;
      timeRunning = millis()-startMillis;
      speedAuto = int(k1*timeRunning + (k2-k1)*(xCutoff/R)*log(1+exp(R*((timeRunning/xCutoff)-1)))+x0);
      Serial.println(speedAuto);
      // set motor speed
      analogWrite(pinMotorSpeed, speedAuto);
    }
    else if(digitalRead(pinStartAuto) == HIGH && flagRunning == false && counterDebounce < 10)
    {
      Serial.println("AUTOMATIC, START START START");
      digitalWrite(ledRunning, HIGH);
      flagFirstRun = false;
      flagRunning = true;
      startMillis = millis();
    }
    else if(((millis()-startMillis >= 30000) || digitalRead(pinStartAuto) == HIGH) && flagRunning == true && counterDebounce > 10)
    {
      Serial.println("AUTOMATIC, STOP STOP STOP");
      digitalWrite(ledRunning, LOW);
      flagFirstRun = true;
      flagRunning = false;
      // Park motor
      analogWrite(pinMotorSpeed, 0);
    }
  }

  // add a delay
  delay(samplingFreq);
}
