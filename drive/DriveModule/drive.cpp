#include "drive.h"

//*********************************Global Variables***********************************//

#include <Wire.h>
#include <INA219_WE.h>
#include <math.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

 float open_loop, closed_loop; // Duty Cyc les
 float vpd,vb,iL,dutyref,current_mA; // Measurement Variables
 unsigned int sensorValue0,sensorValue1,sensorValue2,sensorValue3;  // ADC sample values declaration
 float ev=0,cv=0,ei=0,oc=0; //internal signals
 float Ts=0.0008; //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.


//*Parameters
 float kpv=2.6,kiv=27,kdv=0; // voltage pid.
//**********
 float u0v,u1v,delta_uv,e0v,e1v,e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
 float kpi=0.02512,kii=39.4,kdi=0; // current pid.
 float u0i,u1i,delta_ui,e0i,e1i,e2i; // Internal values for the current controller
 float uv_max=4, uv_min=0; //anti-windup limitation
 float ui_max=1, ui_min=0; //anti-windup limitation
 float current_limit = 1.0;
 bool Boost_mode = 0;
 bool CL_mode = 0;
 
 unsigned int com_count=0;   // a variables to count the interrupts. Used for program debugging.
                 
int DIRR = 21; //defining right direction pin
int DIRL = 20; //defining left direction pin

int opp_num = 0; //count of opperation number

//******* Output voltage 
float vref = 5; //default vref
//***********

 int pwmr = 5;                     //pin to control right wheel speed using pwm
 int pwml = 9;                     //pin to control left wheel speed using pwm
 
 float r = 160; // ANGLE CONST.
 int ang_act;

 int state;

 //Angle PID Variables//
  float err0,err1,err2=0;
  float out0,out1,out2=0;
//Distance PID Variables//
  float err0_2,err1_2,err2_2=0;
  float out0_2,out1_2,out2_2=0;


//============================= Class Functions ========================================//

DriveClass:: DriveClass(){}

void DriveClass::SETUP(){
  //************************** Motor Pins Defining **************************//
  pinMode(DIRR, OUTPUT);
  pinMode(DIRL, OUTPUT);
  pinMode(pwmr, OUTPUT);
  pinMode(pwml, OUTPUT);
  //digitalWrite(pwmr, HIGH);       //setting right motor speed at maximum
  //digitalWrite(pwml, HIGH);       //setting left motor speed at maximum
}

void DriveClass::pinSetup(){
  //************************** Basic pin setups **************************//
  noInterrupts(); //disable all interrupts
  pinMode(13, OUTPUT);  //Pin13 is used to time the loops of the controller
  pinMode(3, INPUT_PULLUP); //Pin3 is the input from the Buck/Boost switch
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC
  }

void DriveClass::timerInit(){
  // TimerA0 initialization for control-loop interrupt.
  TCA0.SINGLE.PER = 255; //
  TCA0.SINGLE.CMP1 = 255; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm; //64 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm; 

  // TimerB0 initialization for PWM output
  pinMode(6, OUTPUT);
  TCB0.CTRLA=TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz
  analogWrite(6,120); 

  interrupts();  //enable interrupts.
  Wire.begin(); // We need this for the i2c comms for the current sensor
  ina219.init(); // this initiates the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  }
  

void DriveClass::operationSetup(boolean Boost_mode, boolean CL_mode){
    if (Boost_mode){
      if (CL_mode) { //Closed Loop Boost
          pwm_modulate(1); // This disables the Boost as we are not using this mode
      }else{ // Open Loop Boost
          pwm_modulate(1); // This disables the Boost as we are not using this mode
      }
    }else{      
      if (CL_mode) { // Closed Loop Buck
          // The closed loop path has a voltage controller cascaded with a current controller. The voltage controller
          // creates a current demand based upon the voltage error. This demand is saturated to give current limiting.
          // The current loop then gives a duty cycle demand based upon the error between demanded current and measured
          // current
          current_limit = 3; // Buck has a higher current limit
          ev = vref - vb;  //voltage error at this time
          cv=pidv(ev);  //voltage pid
          cv=saturation(cv, current_limit, 0); //current demand saturation
          ei=cv-iL; //current error
          closed_loop=pidi(ei);  //current pid
          closed_loop=saturation(closed_loop,0.99,0.01);  //duty_cycle saturation
          pwm_modulate(closed_loop); //pwm modulation
      }else{ // Open Loop Buck
          current_limit = 3; // Buck has a higher current limit
          oc = iL-current_limit; // Calculate the difference between current measurement and current limit
          if ( oc > 0) {
            open_loop=open_loop-0.001; // We are above the current limit so less duty cycle
          } else {
            open_loop=open_loop+0.001; // We are below the current limit so more duty cycle
          }
          open_loop=saturation(open_loop,dutyref,0.02); // saturate the duty cycle at the reference or a min of 0.01
          pwm_modulate(open_loop); // and send it out
      }
    }
 }// closed loop control path

void DriveClass::writeState(int DIRRstate_this, int DIRLstate_this){
  digitalWrite(DIRR, DIRRstate_this);
  digitalWrite(DIRL, DIRLstate_this); 
}

void DriveClass::writePWMState(int pwmrState_t, int pwmlState_t){
  analogWrite(pwmr, pwmrState_t);
  analogWrite(pwml, pwmlState_t);
}

void DriveClass::pwm_modulate(float pwm_input){ // PWM function
  analogWrite(6,(int)(255-pwm_input*255)); 
}

int DriveClass::pwm_modulate_gen(float pwm_input){ // PWM function
   return (int)(255-pwm_input*255); 
}


float DriveClass::pidi(float pid_input){ // This is a PID controller for the current
  float e_integration;
  e0i = pid_input;
  e_integration=e0i;
  
  //anti-windup
  if(u1i >= ui_max){
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }
  
  delta_ui = kpi*(e0i-e1i) + kii*Ts*e_integration + kdi/Ts*(e0i-2*e1i+e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i,ui_max,ui_min);
  
  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}

float DriveClass::pidv( float pid_input){// This is a PID controller for the voltage
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;
 
  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if(u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv*(e0v-e1v) + kiv*Ts*e_integration + kdv/Ts*(e0v-2*e1v+e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v,uv_max,uv_min);
  
  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

void DriveClass::sampling(){
// This subroutine processes all of the analogue samples, creating the required values for the main loop
  // Make the initial sampling operations for the circuit measurements
  
  sensorValue0 = analogRead(A0); //sample Vb
  //sensorValue2 = analogRead(A2); //sample Vref  
  sensorValue3 = analogRead(A3); //sample Vpd
  current_mA = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)

  // Process the values so they are a bit more usable/readable
  // The analogRead process gives a value between 0 and 1023 
  // representing a voltage between 0 and the analogue reference which is 4.096V
  
  vb = sensorValue0 * (4.096 / 1023.0); // Convert the Vb sensor reading to volts
  //vref = sensorValue2 * (4.096 / 1023.0); // Convert the Vref sensor reading to volts
  //vref = 2;
  vpd = sensorValue3 * (4.096 / 1023.0); // Convert the Vpd sensor reading to volts

  // The inductor current is in mA from the sensor so we need to convert to amps.
  // We want to treat it as an input current in the Boost, so its also inverted
  // For open loop control the duty cycle reference is calculated from the sensor
  // differently from the Vref, this time scaled between zero and 1.
  // The boost duty cycle needs to be saturated with a 0.33 minimum to prevent high output voltages
  
  if (Boost_mode == 1){
    iL = -current_mA/1000.0;
    dutyref = saturation(sensorValue2 * (1.0 / 1023.0),0.99,0.33);
  }
  else{
    iL = current_mA/1000.0;
    dutyref = sensorValue2 * (1.0 / 1023.0);
  }
}

void DriveClass::setVref(float vref_t){
  vref= vref_t;
}

float DriveClass::saturation( float sat_input, float uplim, float lowlim){ // Saturatio function
  if (sat_input > uplim) sat_input=uplim;
  else if (sat_input < lowlim ) sat_input=lowlim;
  else;
  return sat_input;
}

void DriveClass::forward(float distance, float x_n, float y_n,  int speed_f)
{
  long dis_int = distance * 1000;
  long y_int = y_n * 1000;
  int speed_m = speed_f;
  if (speed_f == 100){speed_m = 90;}

  //pair pwm_pair = Line_PWM_PID_alt(x_n/r);

  if (y_int < dis_int)
  {
    //writePWMState(pwm_pair.x, pwm_pair.y); //right, left  //Offset added to correct for wheels 100:90
    writePWMState(255, 128);
    DIRRstate = HIGH;
    DIRLstate = LOW;
  }
  else
  {
    writePWMState(0, 0);
    operation = 1;
    opp_num = 0;
    operation_done = 1;
    Serial1.println("d?");
  }
  //Serial.println("PWM   right: "+String(pwm_pair.x) + " left: "+ String(pwm_pair.y));
}

void DriveClass::turn(float angle_des, float x_n, int speed_f)
{
  long angle_int = angle_des * 100;
  long x_int = x_n * 100;
  if ((x_int < angle_int * r) && (angle_int > 0 )){
    writePWMState(speed_f, speed_f);
    DIRRstate = HIGH;
    DIRLstate = HIGH;
  }
  else if ((x_int > angle_int * r) && (angle_int < 0 )){
    writePWMState(speed_f, speed_f);
    DIRRstate = LOW;
    DIRLstate = LOW;
  }
  else{
    writePWMState(0, 0);
    opp_num = 2;
    operation = 1;
  }
}

  void DriveClass::printvals()
  {
    Serial.println(vb);
  }

  void DriveClass::interrupt(int loopTrigger)
  {

    if (loopTrigger)
    {                                                       // This loop is triggered, it wont run unless there is an interrupt
      digitalWrite(13, HIGH);                               // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.
      drive.sampling();                                     // Sample all of the measurements and check which control mode we are in
      drive.operationSetup(digitalRead(2), digitalRead(3)); //determine operation from OL_CL swtich, and Buck_Boost switch
      digitalWrite(13, LOW);                                // reset pin13.
      loopTrigger = 0;
    }
  }

  //find arc length and angle to desired point
  Line DriveClass::mapLine(float x, float y)
  { // we are assuming we are at 0,0
    float angle = atan2(y, x);
    float length = sqrt((x*x) + (y*y));
    Line newline{angle, length};
    return newline;
  }

  pair DriveClass::getCoords(Line line){
    float r = line.length;
    float theta = line.angle;
    //Serial.println("r: " + String(r));
    //Serial.println("theta: " + String(theta));
    float x = r * cos(theta);
    float y = r* sin(theta);
    //Serial.println("x: " + String(x));
    //Serial.println("y: " + String(y));
    return pair{x, y};
  }

  float DriveClass::Angle_PID(int input)
  { // This is a PID controller for the line following algorithm. It tries to 0 the input.
    float e_integration;

    err0 = input;
    e_integration = err0;

    float kpv = 0.5;
    float kiv = 0;
    float kdv = 0;

    float out_max = PI; //anti-windup;

    //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
    if (out1 >= out_max)
    {
      e_integration = 0;
    } else if (out1 <= -out_max) {
    e_integration = 0;
  }

  float delta_out; 
  // USE SAME TS AS VOLTAGE PID
  delta_out = kpv*(err0-err1) + kiv*Ts*e_integration + kdv/Ts*(err0-2*err1+err2); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  out0 = out1 + delta_out;  //this time's control output

  //output limitation
  this->saturation(out0,out_max,-out_max);
  
  out1 = out0; //update last time's control output
  err2 = err1; //update last last time's error
  err1 = err0; // update last time's error
  return out0;
}

float DriveClass::Distance_PID(int input){// This is a PID controller for the line following algorithm. It tries to 0 the input.
  float e_integration;

  err0_2 = input;
  e_integration = err0_2;

  float kpv = 1;
  float kiv = 1;
  float kdv = 0;

  float out_max = 3.14; //anti-windup;

  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if(out1_2 >= out_max) {
    e_integration = 0;
  } else if (out1_2 <= -out_max) {
    e_integration = 0;
  }

  float delta_out; 
  // USE SAME TS AS VOLTAGE PID
  delta_out = kpv*(err0_2-err1_2) + kiv*Ts*e_integration + kdv/Ts*(err0_2-2*err1_2+err2_2); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  out0_2 = out1_2 + delta_out;  //this time's control output



  //output limitation
  this->saturation(out0_2,out_max,-out_max);
  
  out1_2 = out0_2; //update last time's control output
  err2_2 = err1_2; //update last last time's error
  err1_2 = err0_2; // update last time's error
  return out0_2;
}

pair DriveClass::Line_PWM_PID(int bearing, float dist){ //uses simple PID to generate PWM pairs for motors.

  // assume we receive angle relative to line, and dist from line.
  float PID_Change = 0; 
  if (abs(dist) < DIST_ERR ){
    state = ANGLE_STATE;
  }else if (abs(bearing) <ANGLE_ERR){
    state = DISTANCE_STATE;
  }
  if (state == DISTANCE_STATE){
    PID_Change = Distance_PID(dist);
  }else if(state == ANGLE_STATE){
    PID_Change = Angle_PID(bearing);
  }
  float new_bearing = bearing + PID_Change;
  int motora,motorb=0;
  if (new_bearing>0){ //angle is to the right of line
    motora = round(128*new_bearing/PI);
    motorb=0;
  }else{
    motorb = round(128*new_bearing/PI);
    motora=0;
  }
  pair newpair{128+motora,128+motorb};
  //pair newpair{255-motora,255-motorb};
  return newpair;

}

pair DriveClass::Line_PWM_PID_alt(float bearing){ //uses simple PID to generate PWM pairs for motors.
  float PID_Change = Angle_PID(bearing);
  state = ANGLE_STATE;

  float new_bearing = bearing + PID_Change;
  int motora,motorb=0;
  if (new_bearing>0){ //angle is to the left of line
    motora = round(128*new_bearing/PI);
    motorb=0;
  }else{
    motorb = round(128*new_bearing/PI);
    motora=0;
  }
  pair newpair{128+motora,128+motorb};
  //pair newpair{255-motora,255-motorb};
  return newpair;
}

/* void DriveClass::followLine(Line line)
{
  int start_angle = line.angle;
  float total_dist = line.length;
  switch (state)
  {
  case 0:      //hibernating
    state = 1; //intentional fall-through
  case 1:      //aligning line
    if (turn(start_angle))
    {
      state = 2;
      rebaseCoords();
      ///
    }
    break;
  case 2: // FALLTHROUGH
  case 3:
    // PID-LOOP FOLLOWING CODE. STATE GETS CHANGED BY LINE_PWM_PID!
    int curr_angle = 1;
    float curr_dist = 1;
    pair PWM_pair = Line_PWM_PID(curr_angle, curr_dist);
    writePWMState(PWM_pair.x, PWM_pair.y);
  }
} */



DriveClass drive = DriveClass();
