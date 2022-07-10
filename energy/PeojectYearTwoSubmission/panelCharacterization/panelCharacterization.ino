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
unsigned int loop_trigger;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
float Ts = 0.001; //1 kHz control frequency.
float current_measure;
float pwm_out = 0.01;
float V_PD;
float V_B;
boolean input_switch = true;
String dataString;

int n = 1000;              //number of previous values to average for moving average algorithm
int index = 0;                  //the index used for the moving average algorithm
float powerValues[1000];     //array that holds n previous values for moving average algorithm
float sum = 0;                    //the sum of n previous values for the moving average algorithm 

float instantaneous_power;
float secondAveragedPower;



void setup() {
  //Some General Setup Stuff

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications


  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
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

}

void loop() {
  input_switch = digitalRead(2); //get the OL/CL switch status
  if ((loop_trigger == 1) && (input_switch == true)){ //FAST LOOP (1kHZ)
    
      //Serial.println("fastLoop"); //otherwise print an error
    

      //state_num = next_state; //state transition
      V_PD = analogRead(A0)*10.808; 
      V_B = analogRead(A1)*4.096/1.03;
     /* if ((V_Bat > 3700 || V_Bat < 2400)) { //Checking for Error states (just battery voltage for now)
          state_num = 5; //go directly to jail
          next_state = 5; // stay in jail
          digitalWrite(7,true); //turn on the red LED
          current_ref = 0; // no current
      }
      */
      
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)

      
      //secondAveraged_V_B = movingAverage(instantaneous_power);

      instantaneous_power = (V_B/1000) * (current_measure/1000) * 1000;      //update the current value of power in mW
      secondAveragedPower = movingAverage(instantaneous_power);
      
      
      
      /*error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      pwm_out = pidi(error_amps); //Perform the PID controller calculation
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      */
      int_count++; //count how many interrupts since this was last reset to zero
      
      loop_trigger = 0; //reset the trigger and move on with life

      
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)
    
 dataString = String(V_PD) + "," + String(V_B) + "," + String(current_measure) + "," + String(pwm_out) + "," + String(instantaneous_power*100); //build a datastring for the CSV file
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


      
   
  int_count = 0; // reset the interrupt count so we dont come back here for 10000ms
  }
}

// Timer A CMP1 interrupt. Every 1s the program enters this interrupt. This is the fast 1Hz loop
ISR(TCA0_CMP1_vect) {
  if (pwm_out < 0.99) { //check to see if we have already cycled through all possible values for the duty-cycle
    loop_trigger = 1; //trigger the loop when we are back in normal flow
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
   }
}

float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}


float movingAverage(float currentPower){
  
  float movingAverageValue;
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
