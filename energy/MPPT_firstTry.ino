#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;
unsigned int rest_timer;
unsigned int loop_trigger;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float ui_max = 1, ui_min = 0; //anti-windup limitation
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0, error_amps; // Current Control
float pwm_out = 0.01;
float duty_step = 0.005;
float V_PD;
float V_B;
float power = 2;
float previous_power = 1;
boolean input_switch = true;
boolean slowLoop_enabled = false;
int state_num=0,next_state;
String dataString;
float instantaneous_power;

int n = 40;              //number of previous values to average for moving average algorithm
int index = 0;                  //the index used for the moving average algorithm
float powerValues[40];     //array that holds n previous values for moving average algorithm


void setup() {
  //Some General Setup Stuff

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications


  //Check for the SD Card
  //Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    //Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("Panel.csv")) { // Wipe the datalog when starting
    SD.remove("Panel.csv");
  }

  
  noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  //SMPS Pins
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

  //LEDs on pin 7 and 8
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  //Analogue input, port A voltage
  pinMode(A0, INPUT);

  //Analogue input, port B voltage
  pinMode(A1, INPUT);

  

  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with

  for (int i = 0; i < n; i++){
    powerValues[i] = 0;
  }
}

void loop() {
  input_switch = digitalRead(2); //get the OL/CL switch status
  if (loop_trigger == 1){ //FAST LOOP (1kHZ)
    
      //Serial.println("fastLoop"); //otherwise print an error
    

      //state_num = next_state; //state transition
      V_PD = analogRead(A0)*10.808; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)

      V_B = analogRead(A1)*4.096/1.03;
     /* if ((V_Bat > 3700 || V_Bat < 2400)) { //Checking for Error states (just battery voltage for now)
          state_num = 5; //go directly to jail
          next_state = 5; // stay in jail
          digitalWrite(7,true); //turn on the red LED
          current_ref = 0; // no current
      }
      */
      
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)

      if (input_switch){ //the input switch enables MPPT functionality
          //calculate power at port B
            
            instantaneous_power = (V_B/1000) * (current_measure/1000) * 1000;      //update the current value of power in mW
            //Serial.println(current_measure);
            power = movingAverage(instantaneous_power);

            if (index == 0){  //the below correction for the MPPT tracking occurs when n values have been averaged

              
              //workout the size of the step as a linear function of the difference in performance provided by the previous duty cycle change

              Serial.println(abs(power-previous_power));
              float absolute_duty_step = abs(power-previous_power)/float(1000);
              Serial.println(String(absolute_duty_step,5));
              absolute_duty_step = saturation(absolute_duty_step, 0.1, 0.001);
              
              //duty_step = float(sign(duty_step))*absolute_duty_step; //uncomment this to enable smart mppt
              
              
              //check if the previous step led to an improvement in the power
                if (power<previous_power) {         //If this is true, the previous step led to a decrease of power
                  duty_step = -duty_step;                     //the step is inverted
                }
        
              //The change to the duty cycle is made according to the set step
              pwm_out = pwm_out + duty_step; 
               pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
              analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)

              previous_power = power;             //store the current power as previous power, to be used during the next execution of this loop
            }
      }
      
        //print the current power output
       dataString = String(power) + "," + String(pwm_out*100) + "," + String(instantaneous_power) +"," + String(duty_step, 3); //build a datastring for the CSV file
       Serial.println(dataString); // send it to serial as well in case a computer is connected
      /*error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      pwm_out = pidi(error_amps); //Perform the PID controller calculation
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      */
      int_count++; //count how many interrupts since this was last reset to zero
      
      loop_trigger = 0; //reset the trigger and move on with life

      
  }
  
  if ((int_count == 1000) && (slowLoop_enabled)) { // SLOW LOOP (0.1Hz)
    
 dataString = String(V_PD) + "," + String(V_B) + "," + String(current_measure) + "," + String(pwm_out); //build a datastring for the CSV file
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("Panel.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file

    pwm_out = pwm_out + 0.01;
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)


      
    /*switch (state_num) { // STATE MACHINE (see diagram)
      case 0:{ // Start state (no current, no LEDs)
        current_ref = 0;
        if (input_switch == 1) { // if switch, move to charge
          next_state = 1;
          digitalWrite(8,true);
        } else { // otherwise stay put
          next_state = 0;
          digitalWrite(8,false);
        }
        break;
      }
      case 1:{ // Charge state (250mA and a green LED)
        current_ref = 250;
        if (V_Bat < 3600) { // if not charged, stay put
          next_state = 1;
          digitalWrite(8,true);          
        } else { // otherwise go to charge rest
          next_state = 2;
          digitalWrite(8,false);
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;
      }
      case 2:{ // Charge Rest, green LED is off and no current
        current_ref = 0;
        if (rest_timer < 30) { // Stay here if timer < 30
          next_state = 2;
          digitalWrite(8,false);
          rest_timer++;
        } else { // Or move to discharge (and reset the timer)
          next_state = 3;
          digitalWrite(8,false);
          rest_timer = 0;
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;        
      }
      case 3:{ //Discharge state (-250mA and no LEDs)
         current_ref = -250;
         if (V_Bat > 2500) { // While not at minimum volts, stay here
           next_state = 3;
           digitalWrite(8,false);
         } else { // If we reach full discharged, move to rest
           next_state = 4;
           digitalWrite(8,false);
         }
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;
      }
      case 4:{ // Discharge rest, no LEDs no current
        current_ref = 0;
        if (rest_timer < 30) { // Rest here for 30s like before
          next_state = 4;
          digitalWrite(8,false);
          rest_timer++;
        } else { // When thats done, move back to charging (and light the green LED)
          next_state = 1;
          digitalWrite(8,true);
          rest_timer = 0;
        }
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
        }
        break;
      }
      case 5: { // ERROR state RED led and no current
        current_ref = 0;
        next_state = 5; // Always stay here
        digitalWrite(7,true);
        digitalWrite(8,false);
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(7,false);
        }
        break;
      }

      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 5; // So if we are here, we go to error
        digitalWrite(7,true);
      }
      
    }
    
    dataString = String(state_num) + "," + String(V_Bat) + "," + String(current_ref) + "," + String(current_measure); //build a datastring for the CSV file
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
  */
  int_count = 0; // reset the interrupt count so we dont come back here for 10000ms
  }
}

// Timer A CMP1 interrupt. Every 1s the program enters this interrupt. This is the fast 1Hz loop
ISR(TCA0_CMP1_vect) {
  
    loop_trigger = 1; //trigger the loop when we are back in normal flow
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
   
}

float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}

float pidi(float pid_input) { // discrete PID function
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max) {
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}


float sum = 0;                    //the sum of n previous values for the moving average algorithm 
float movingAverageValue;

float movingAverage(float currentPower){
  

  sum = sum - powerValues[index] + currentPower;  //subtracting the previous value at the current index and adding the new one in its place
  powerValues[index] = currentPower;
  if (index < (n-1)) {
    
    index = index + 1;
    
  } else {
  
    index = 0;
  
  }

  movingAverageValue = float(sum/float(n));
  
  return movingAverageValue;
   
}

int sign(float number) {

  int signOfNumber;
  
  if (number >= 0) {
    signOfNumber = 1;
  } else if (number < 0) {
    signOfNumber = -1;
  }

  return signOfNumber;
  
}
