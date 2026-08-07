#include <util/atomic.h>
const byte ENCA=2;
const byte ENCB=3; 
const byte RPWM = 9; 
const byte LPWM= 10; 
volatile long pos_i=0; 
int increment =0; 
int posPrev=0;
float prevT=0; 
//int vel= (2 * 3.14159 * (12.2 / 2.0)) / 1000.0; // [m];//converted 12.2mm to m
float WheelCircmf = (2 * 3.14159 * (12.2 / 2.0)) / 1000.0;
float CountsPerRev=212.2/4; 
float TANK_LENGTH_M= 300.2/1000; 
void setup() {
pinMode(RPWM, OUTPUT);
pinMode(LPWM, OUTPUT); 
pinMode(ENCA,INPUT);
pinMode(ENCB, INPUT);
Serial.print("RESET");
Serial.begin(9600);
attachInterrupt(digitalPinToInterrupt(ENCA), readEncoder, RISING);
}
void readEncoder(){
int b = digitalRead(ENCB);
if (b>0){
  increment = 1; 
}
else {increment = -1;}
pos_i = pos_i + increment; 
}

//set motor pwm
//calculate cts/s using discrete method
//convert cts/s to velocity

void Motor(int dir, int pwmVal, int LPWM, int RPWM){
if(dir==1){
  analogWrite(LPWM, pwmVal);
  analogWrite(RPWM, 0);
}
else if (dir == -1){
  analogWrite(RPWM, pwmVal);
  analogWrite(LPWM, 0);
}
else {
  digitalWrite(RPWM, 0);
  digitalWrite(LPWM, 0);
}
}
void loop() {
int pos = 0; 
int dir = -1; 
ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
  pos = pos_i;
}
long currT = micros();
float dT = (float(currT-prevT))/ 1.0e6; 
float ctsPers = (pos-posPrev)/dT;
prevT=currT; 
posPrev=pos; 
//converted to dx/dt or cts/s
float vel=(ctsPers/CountsPerRev)*WheelCircmf;
int pwmVal=120;//PWM Calibration
float distance_traveled_m = fabs((float)(pos) / CountsPerRev) * WheelCircmf;
  if(distance_traveled_m >= TANK_LENGTH_M-0.02){
    Motor(0,0,LPWM, RPWM);
    Serial.println("Start Stopping");
    return;
    //e_integral=0;
    //return;  //-------------->Return effectively cancels all code below once this condition is met. 
    //DirTravelled =0;  //if gantry reaches end, stop the motor
    //pos_atPassStart = pos; //reset pass start pos to current pos
    //distance_traveled_m=0; //reset dist travelled to 0 for 2nd round
  }
  
//int dir=1; 
Motor(dir, pwmVal, LPWM, RPWM);
//Pass PWM value into Motor function to test pwm to velocity. 
Serial.print("PWM = ");
Serial.print(pwmVal);
Serial.print("V= ");
Serial.println(vel); 
}

