/*Adapted from:
 Drone Bot workshop & Curio Res  
 tutorials. 
 By Jaine Ratush. 
 */

//VELOCITY MEASUREMENT--ENCODER READINGS METHOD 1 W/LOW PASS FILTER 25Hz
//trapezoidal (accel/cruise/decel) profile driven by encoder distance
//HC-SR04 used as a threshold e-stop only, not for the ramp itself

#include <util/atomic.h> //temporarily disables interrupts so new/old data isnt mixed 
const byte ENCA = 2; //Encoders connected to interrupt pins for future. 
const byte ENCB = 3; 
const byte FPWM = 9; //Forward PWM channel
const byte RPWM =10;  //Reverse PWM Channel 
int Trig = 6; //Proximity sensor trig pin 
int Echo = 5; //Proximity sensor return echo pin 
int Button=4; //digitalpin

//Globals for vel measurement:
long prevT = 0; 
int posPrev = 0; 
volatile int pos_i = 0; 
float e_integral = 0; 
float vFil = 0; 
float vPrev = 0; 

//---- Trapezoidal profile parameters and vars ----
const float WHEEL_CIRC_M   = (2 * 3.14159 * (12.2 / 2.0)) / 1000.0; // [m]
const float COUNTS_PER_REV = 48; 
const float V_CRUISE       = 0.25;   // m/s,      //Change for a higher speed
//const float D_RAMP         = 63.5/1000;  // m, accel dist.
//const float D_DECEL        = 63.5/1000;  // m, decel dist.
const float TANK_LENGTH_M  = 350.2/1000;  // m   //FIX--> 450
int DirTravelled = 1; //Direction travelled by tank--Input independent of what PID controller reacts
long pos_atPassStart = 0; // Resets encoder position to 0 after the forward pass
//---- E-stop / proximity parameters ----
const float STOP_THRESHOLD_MM  = 430;    // trigger distance from wall
const unsigned long PING_INTERVAL_MS = 60;   // ~16Hz, safe for HC-SR04
const unsigned long ECHO_TIMEOUT_US  = 25000; // caps worst-case pulseIn block at 25ms
unsigned long lastPingTime = 0;
bool emergencyStop = false;

void setup() {
  Serial.begin(9600); //Terminal

  pinMode(ENCA, INPUT); //Encoders are inputs to microcontroller
  pinMode(ENCB, INPUT);
  pinMode(FPWM, OUTPUT); //Microcontroller outputs signals thru PWM channels
  pinMode(RPWM, OUTPUT);
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(Button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCA), readEncoder, RISING);
  Serial.println("BOOT");
  /*the first argument attaches
  ENCA--pin2, to an interrupt number either 0 or 1 for arduino uno.
  readEncoder is a function I defined that is activated for some
  trigger<RISING>. If readEncoder was readEncoder() instead,
  the encoder would always be getting read instead of just when
  square wave A switches from low to high
  */
}

//----------------------------------------------------------------------//
//PROXIMITY SENSOR -- used only as a threshold e-stop, not for ramp shaping
//Returns distance in mm, or -1 if the ping timed out (no echo)
/*long Prox() {
  digitalWrite(Trig, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig, LOW); //generates 10us pulse

  long pulse_duration = pulseIn(Echo, HIGH, ECHO_TIMEOUT_US); //timeout-protected
  if (pulse_duration == 0) return -1; //no echo received, treat as "no valid reading"

  long distance_mm = pulse_duration * 0.343 / 2.0; //dist in mm from prox
  return distance_mm;
}*/

//----------------------------------------------------------------------//
//READING THE ENCODER
void readEncoder() { //square wave conditions
  int b = digitalRead(ENCB);
  int increment = 0;
  if (b > 0) {
    increment = 1; //if b>0 and A rises CCW==Forward bc B is leading A
  } else { //otherwise A leads B, CW==Reverse
    increment = -1;
  }
  pos_i = pos_i + increment;
}

//----------------------------------------------------------------------//
//TRAPEZOIDAL PROFILE
//Given how far the carriage has traveled (in meters, always positive,
//measured from the start of THIS pass), returns the velocity setpoint.
//Handles the case where the tank is too short to reach cruise speed
//(triangle profile) automatically.
/*float computeVTarget(float distance_traveled_m) {
  /*Serial.print("distance traveled = ");
  Serial.println(distance_traveled_m);*/
 /*
  float d_ramp = D_RAMP;
  float d_decel = D_DECEL;

  // Triangle case: not enough room to ramp up AND down at these distances.
  // Scale both ramp distances down proportionally so they still meet
  // exactly in the middle of the tank instead of overlapping/overshooting.
  if (TANK_LENGTH_M < (d_ramp + d_decel)) {
    float scale = TANK_LENGTH_M / (d_ramp + d_decel);
    d_ramp *= scale;
    d_decel *= scale;
  }

  float distance_remaining = TANK_LENGTH_M - distance_traveled_m;
  const float V_MIN=0.2; //ms^-1 PID starts at 0 no bueno
  if (distance_traveled_m < d_ramp) {
    // accel phase
    float v= V_CRUISE * (distance_traveled_m / d_ramp);
    return v < V_MIN ? V_MIN:v ;
  } else if (distance_remaining < d_decel) {
    // decel phase
    float v = V_CRUISE * (distance_remaining / d_decel);
    return v < 0 ? 0 : v;
  } else {
    // cruise phase
    return V_CRUISE;
  }*/ 
//}

//----------------------------------------------------------------------//
//Speed<PWM value>, direction of motor function 
//MOTOR SETTING
void setMotor(int dir, int pwmVal, int FPWM, int RPWM) {
  /*analogWrite(pin#,value from 0-255);
  Unlike tutorial, IN1 & IN2 are pwm capable channels.
  no digitalWrite becuase its not just 0 or 1 off or on.
  Instead CCW, FPWM is at some pwm value and RPWM==0*/

  if (dir == 1) {
    analogWrite(FPWM, pwmVal);
    analogWrite(RPWM, 0);
  } else if (dir == -1) {
    analogWrite(RPWM, pwmVal);
    analogWrite(FPWM, 0);
  }
  /*int dir and int pwmVal are function-specific parameters,
  given a value when the function is called later on.*/
  else {
    digitalWrite(RPWM, LOW);
    digitalWrite(FPWM, LOW);
  }
  /*Forces a stop here instead of relying on
  a perfect 0% duty cycle.*/
}

/*------------------------------------------------------------------*/
void loop() {

  //---- E-stop check, gated to ~60ms so it doesn't hammer the sensor ----
  /*if (millis() - lastPingTime > PING_INTERVAL_MS) {
    lastPingTime = millis();
    long d = Prox();
    if (d >= STOP_THRESHOLD_MM) {
      emergencyStop = true;
    }
  }
  if (emergencyStop) {
    setMotor(0, 0, FPWM, RPWM); // coast/stop; swap for dynamic braking if your driver supports it
    return; // skip the rest of loop() -- motor stays off until reset (e.g. power cycle or reset logic)
  } */

  int pos = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    pos = pos_i;
  }
/*Serial.println(pos);
delay(100);*/
  //---- Velocity measurement 
  long currT = micros(); //micros returns # microsecs since the loop began
  float dT = ((float)(currT - prevT)) / 1.0e6;
  float vel = (pos - posPrev) / dT; //dx/dt

  posPrev = pos;
  prevT = currT;
  float v = (vel / COUNTS_PER_REV) * WHEEL_CIRC_M; //cts/s -> m/s
  vFil = 0.854 * vFil + 0.0728 * v + 0.0728 * vPrev; //low-pass, cuts noise above 25Hz
  vPrev = v;

  //---- Distance traveled this pass, for the ramp ----
  float distance_traveled_m = fabs((float)(pos - pos_atPassStart) / COUNTS_PER_REV) * WHEEL_CIRC_M;
  if(distance_traveled_m >= TANK_LENGTH_M-0.02){
    setMotor(0,0,FPWM, RPWM);
    e_integral=0;
    return;  //-------------->Return effectively cancels all code below once this condition is met. 
    //DirTravelled =0;  //if gantry reaches end, stop the motor
    //pos_atPassStart = pos; //reset pass start pos to current pos
    distance_traveled_m=0; //reset dist travelled to 0 for 2nd round
  }

  //Trapezoidal ramp sets the PID target
  float vTarget = DirTravelled * V_CRUISE;
 // Kept vTarget as vars so signs would be correct 
  //PID Control!
  //Feedforward Calc: PWM/vTarget==?/0.5
  int Kff = 260;//70/0.25; //Change for a higher speed using calibration code
  
  float Kp = 1;
  float Ki = 0.2;
  float e = vTarget - vFil;
  e_integral += e * dT;
  if(e_integral > 50) e_integral =50; //integral term cap
  if(e_integral< -50) e_integral =-50; 
  //float u =  Kp * e + Ki * e_integral;
  float u = Kff*vTarget + Kp * e + Ki * e_integral; //->FeedForward
//TEST w/out PI Controller first..
  int dir = 1;
  if (u < 0) {
    dir = -1;
  }

  int pwr = (int) fabs(u);
  if (pwr > 255) {
    pwr = 255;
  }
 //Serial.print("u=");
 // Serial.print(u);
 // Serial.print("pwr= ");
 // Serial.println(pwr);
  setMotor(dir, pwr, FPWM, RPWM);
  //Serial.print(pos);
  //Serial.print(",");
 // Serial.print(",");
  //Serial.print(pwr);
 // Serial.print(v);
 //Serial.print(",");
  //Serial.print(vFil);
  //Serial.print(",");
  //Serial.print(e);
  //Serial.println(vTarget);
  //Serial.print(" stop=");
//Serial.print(emergencyStop);
Serial.print("pos=");
Serial.print(pos);
Serial.print(" dist=");
Serial.print(distance_traveled_m, 3);
Serial.print(" stopDist=");
Serial.println(distance_traveled_m >= TANK_LENGTH_M - 0.02);
  
/*Serial.print(pos);
Serial.print(",");
Serial.print(distance_traveled_m);
Serial.print(",");
Serial.print(TANK_LENGTH_M);
Serial.print(",");
Serial.println(distance_traveled_m >= TANK_LENGTH_M - 0.02); */
} 
