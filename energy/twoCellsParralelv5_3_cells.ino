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
float pwm_out;
float V_Bat;
boolean input_switch;
int state_num=0,next_state;
String dataString;
float V_Cell[] = {0, 0};           //variable used to store the voltage of a measured cell
float chargingCurrentPerCell;   //variable used to store the current of each individual cell when they are connected in parralel and charging
float dischargingCurrentPerCell[] = {0, 0}; //Current of each cell when the discharge circuit is being used
int dischargingTime[] = {0, 0};
int measurePeriod = 10;
int measurePeriodTimer = 0;
float capacity[] = {400, 400}; //to be updated
bool startedBalancing = false;
const float socCharacteristic[] = {0.29397, 0.31847, 0.34297, 0.35522, 0.36747, 0.37972, 0.39196, 0.39896, 0.40596, 0.41296, 0.41996, 0.42696, 0.43396, 0.44096, 0.45933, 0.47771, 0.49608, 0.51445, 0.53283, 0.5512, 0.56957, 0.58795, 0.68594, 0.71044, 0.73493, 0.75943, 0.78393, 0.80843, 0.83293, 0.85742, 0.88192, 0.97991, 1.2249, 1.4699, 1.7148, 2.0088, 2.3028, 2.6458, 3.0377, 3.4297, 3.8707, 4.3116, 4.7036, 5.1935, 5.6835, 6.2714, 6.9084, 7.5453, 8.0843, 8.7212, 9.4072, 10.0931, 10.975, 11.8569, 12.6409, 13.6208, 14.6007, 15.6296, 16.7075, 17.6384, 18.6673, 20.1372, 21.705, 23.1749, 24.7918, 28.3195, 38.6085, 51.7883, 61.8324, 70.3087, 78.3439, 86.2812, 95.6394, 108.2803, 127.5845, 162.9593, 194.6595, 219.5002, 246.9868, 271.1416, 295.9334, 317.0995, 334.2969, 348.6526, 359.4317, 367.8099, 374.6203, 379.6668, 384.1254, 387.9471, 390.8378, 392.9446, 395.2964, 397.3542};
int cellMeasuredID = 0;
int relayWaitTime = 1;        //the number of seconds waited for the relay to settle
int relayWaitTimer = 0;       //timer used to count the number of seconds waited for the relay to settle

float SOC[] = {0, 0};   //State of Charge (SOC) for each of the cells (in mAh)
float totalCapacity;   //The total capacity of all cells combined
float cycles[] = {7, 7}; //note TODO
float temperature[] = {77, 77}; //note TODO



void ledBinary(int number) {

  bool led[] = {false, false};
  switch (number) { //choosing which cell to measure
        case 0:{  //binary 0 is represented
                  led[0] = false;
                  led[1] = false;   
                  break;   
        }
  
        case 1:{  //binary 1 is represented
                  led[0] = true;
                  led[1] = false;
                  break;      
        }

        case 2:{  //binary 2 is represented
                  led[0] = false;
                  led[1] = true; 
                  break;     
        }

        case 3:{  //binary 3 is represented
                  led[0] = true;
                  led[1] = true;
                  break;      
        }

        
        
      }

        //display the result on the leds
        digitalWrite(7, led[0]);
        digitalWrite(8, led[1]);
  
}
void measure(int cellIdentificationNumber, int timer) { //note
  
    Serial.println("Measurement of cell " + String(cellIdentificationNumber) + " with timer of "+ String(timer)+"/"+String(relayWaitTime));
  if (timer == 0) { //at the start of the timer the signal is sent to the relay
      
      switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  digitalWrite(5,true); //select cell 0 for measurement by turning on the relay of this cell
                  break;
        }
  
        case 1:{  //cell 1 selected
                  digitalWrite(4,true); //select cell 1 for measurement by turning on the relay of this cell
                  break;
        }
      }
  } else if (timer >= relayWaitTime) { //at the end of the wait time, the relay has settled so the value is measured
    
      V_Cell[cellIdentificationNumber] = analogRead(A2)*4.096*0.9747; //measure the cell voltage 
      Serial.println("Voltage of cell "+String(cellIdentificationNumber)+" is "+String(V_Cell[cellIdentificationNumber]));
      digitalWrite(4,false); //stop measuring
      digitalWrite(5,false); //stop measuring
      socLUT(cellIdentificationNumber);                             //the SOC of the battery is updated using the look up table method for the measured voltage
  }
  

      
      
      
  }

  
  


void socLUT(int cellIdentificationNumber) {

  
  
    //SOC[cellIdentificationNumber] = socCharcteristic[cellIdentificationNumber][V_Cell[cellIdentificationNumber]*100]; //retrieving the state of charge from the look up table array for the specific cell
    
    //rounding to the nearest 10 //note check
    int cellVoltageRounded = 10*((round(V_Cell[cellIdentificationNumber])+5)/10);

    //accounting for the offset in the SOC characteristic array whose first element correponds to 2650
    int cellVoltageIndex = (cellVoltageRounded-2650)/10;
  SOC[cellIdentificationNumber] = socCharacteristic[cellVoltageIndex]; //retrieving the state of charge from the look up table array for the specific cell
  Serial.println("SOC of cell " + String(cellIdentificationNumber) + " is " + String(SOC[cellIdentificationNumber]) + "(index "+String(cellVoltageIndex)+")");
   
}

void startDischarge(int cellIdentificationNumber, float dischargeAmount) {                        //note 2

  dischargingCurrentPerCell[cellIdentificationNumber] = V_Cell[cellIdentificationNumber]/150; //current through the cell when discharging is calculated from ohms law 
                                                                                              //considering the voltage across the two parralel 300Ω resistors
                                                                                             //note: 1

  dischargingTime[cellIdentificationNumber] = round(dischargeAmount/dischargingCurrentPerCell[cellIdentificationNumber]*3600); //The discharging time required expressed in seconds
  switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  digitalWrite(9,true); //select cell 0 for measurement by turning on the relay of this cell
                  digitalWrite(5,true); //disconnect parralel connection to cell 1
                  break;
                  
        }
  
        case 1:{  //cell 1 selected
                  digitalWrite(10,true); //select cell 1 for measurement by turning on the relay of this cell
                  digitalWrite(4,true); //disconnect parralel connection to cell 0
                  break;
        }
  
    }

  Serial.println("Starting Discharge of cell " + String(cellIdentificationNumber) + ". Discharge Amount = "+ String(dischargeAmount)+". Discharge Time (s) = "+ String(dischargingTime[cellIdentificationNumber]));
}

void stopDischarge(int cellIdentificationNumber) {                        

  switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  digitalWrite(9,false); //select cell 0 for measurement by turning off the relay of this cell
                  digitalWrite(5,false); //reconnect parralel connection to cell 1
        }
  
        case 1:{  //cell 1 selected
                  digitalWrite(10,false); //select cell 1 for measurement by turning off the relay of this cell
                  digitalWrite(4,false); //reconnect parralel connection to cell 0
        }
  
    }

  Serial.println("Stopped Discharge of cell " + cellIdentificationNumber);
}

void socAccumulateParralel(float accumulationTime, float currentMeasured) {    

  chargingCurrentPerCell = currentMeasured/( sizeof(SOC)/sizeof(float) );
  
  for (int cellId = 0; cellId < ( sizeof(SOC)/sizeof(float) ); cellId++) {
    SOC[cellId] += chargingCurrentPerCell*accumulationTime;
  }

  
}

int minimumIndex (float SOC[], int arraySize){ //Function that returns the index of the minimum value in an array

  int minIndex = 0;
  float minValue = SOC[0];
  
  for (int index = 0; index < arraySize; index++){
    Serial.println("SOC: "+String(SOC[0])+String(SOC[1]));
    if (SOC[index]<minValue){
      minValue = SOC[index];
      minIndex = index;
    }
  }
  Serial.println("minIndex = "+String(minIndex));
  return minIndex;
}
void setup() {
  //Some General Setup Stuff

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications
  Serial1.begin(9600); //communications with the ESP32


  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("BatCycle.csv")) { // Wipe the datalog when starting
    SD.remove("BatCycle.csv");
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

  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A1, INPUT);


  pinMode(5, OUTPUT);  // This is the output used to control the relay pin of cell 0
  pinMode(4, OUTPUT); // This is the output used to control the relay pin of cell 1


  
  pinMode(9, OUTPUT);   // This is the output used to control the discharging of cell 0
  pinMode(10, OUTPUT); // This is the output used to control the relay pin of cell 1

  pinMode(A2, INPUT); //This is the pin used to measure the voltage of individual cells


  
  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with

  //The total capacity of the arrangement is calculated
  totalCapacity = 0;
  for (int index = 0; index < ( sizeof(capacity)/sizeof(float) ); index++){
    totalCapacity = totalCapacity + capacity[index];
  }
  
}

void serialEvent() {
  if(Serial.available()){ //if or while                 //Serial1
    String command = Serial.readStringUntil('?');       //Serial1
    char command_type = command[0];
    Serial.println("Request Recieved "+ String(command_type));
    switch(command_type){
      case 'a' : {//SOC requested
         
         float totalCharge = 0;

         String transmittedString = "a";
         
         for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
          transmittedString = transmittedString +","+String(SOC[index]/capacity[index]);    //add to the string, the percentage charge of the cell given by the index
          totalCharge = totalCharge + SOC[index];
         }

         transmittedString = transmittedString+","+String(totalCharge/totalCapacity)+"?";


        //The structure of the transmitted string is: (percentage charge of cell 0), (percentage charge of cell 1), (total percentage charge)
        Serial.println(transmittedString);           //Serial1
        break;
      }
      
      case 'b': { //number of cycles per cell 

             String transmittedString = "b";
         
             for (int index = 0; index < ( sizeof(cycles)/sizeof(float) ); index++){
              transmittedString = transmittedString +","+String(cycles[index]);    //add to the string, the percentage charge of the cell given by the index
             }
    
             transmittedString = transmittedString+"?";
    
    
            //The structure of the transmitted string is: (number of cycles of cell 0), (percentage charge of cell 1), (total percentage charge)
            Serial.println(transmittedString);          //Serial1
            break;
          }
  
      
      case 'c': { //temperature
       
            String transmittedString = "c";
         
             for (int index = 0; index < ( sizeof(temperature)/sizeof(float) ); index++){
              transmittedString = transmittedString +","+String(temperature[index]);    //add to the string, the percentage charge of the cell given by the index
             }
    
             transmittedString = transmittedString+"?";
    
    
            //The structure of the transmitted string is: (percentage charge of cell 0), (percentage charge of cell 1), (total percentage charge)
            Serial.println(transmittedString);          //Serial1
            break;
      }
      
       
     }
  }


}

void loop() {


  
  
  if (loop_trigger == 1){ // FAST LOOP (1kHZ)

      state_num = next_state; //state transition
      V_Bat = analogRead(A1)*4.096/1.037; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)
      if ((V_Bat > 3700 || V_Bat < 2400) && (state_num != 3) && (state_num != 4)) { //Checking for Error states (just battery voltage for now) (does not include voltage errors in states of charge_rest and measurement as pin A1 will be floating due to the measurement taking place) note TBI
          
          state_num = 8; //go directly to jail
          next_state = 8; // stay in jail
          ledBinary(3); //turn on the red LED
          current_ref = 0; // no current
      }
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      pwm_out = pidi(error_amps); //Perform the PID controller calculation
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      int_count++; //count how many interrupts since this was last reset to zero


      if(Serial.available()){ //note s: kala edw?                //Serial1
           serialEvent();
        }

      
      loop_trigger = 0; //reset the trigger and move on with life
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)
    input_switch = digitalRead(2); //get the OL/CL switch status
   
   //state machine


 
 
 switch (state_num) { // STATE MACHINE 
      case 0:{ // Start state (no current, no LEDs)
        current_ref = 0;
        if (input_switch == 1) { // if switch, move to charge
          next_state = 1;
          ledBinary(1);
        } else { // otherwise stay put
          next_state = 0;
          
        }
        break;
      }
      case 1:{ // Charge state (500mA and a green LED) //Changed from 250mA as there are now two batteries in parralel
        socAccumulateParralel(1, current_measure);
        Serial.println("Charge State (measurePeriodTimer " + String(measurePeriodTimer) + "/" + String(measurePeriod) + ")");
        current_ref = 500;

        if (measurePeriodTimer < measurePeriod) {

            next_state = 1;
            ledBinary(1);
            measurePeriodTimer++;
     
        } else {  //it is now time to check the voltage of each cell

          next_state = 2;
          measurePeriodTimer = 0;
        }

        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          
        }
        break;  
        
      }

      
      case 2:{ // Charge Rest, green LED is off and no current
         Serial.println("Charge Rest (rest_timer " + String(rest_timer) + "/30)");
        current_ref = 0;
        
        
        if (rest_timer < 30) { // Stay here if timer < 30
          next_state = 2;
          
          rest_timer++;
        } else { // Or move to measuring the SOC (and reset the timer)
          next_state = 3;
          
          rest_timer = 0;
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          
        }
        break;        
      }


      case 3:{ //Measuring the SOC of each cell
        Serial.println("Measuring SOC of each cell");
        current_ref = 0;

        measure(cellMeasuredID, relayWaitTimer);
        relayWaitTimer++;
        
        if (relayWaitTimer > relayWaitTime) {
          relayWaitTimer = 0;
          cellMeasuredID = cellMeasuredID + 1;
        }
        
        

        
            if (cellMeasuredID >= ( sizeof(SOC)/sizeof(float) )) {   //all cell measurements have been taken and we can now move on to the next state
               
               cellMeasuredID = 0;                                   //reset this index
               
               if ((SOC[0] < capacity[0]) || (SOC[1] < capacity[1])) { // Check to see if any of the cells still require charging
                 
                 next_state = 4; //transition to the balancing state
                 ledBinary(2);
                 
               } else { // If the cells are fully charged
                 next_state = 5;  //move to the use state
                  ledBinary(0);
               }

            } else {
              next_state = 3; //stay in the same state
            }

            
              if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                next_state = 0;
                
              }
              break;
            }

      case 4:{ //Balancing state
        Serial.println("Balancing");
               current_ref = 0;
                bool doneBalancing = true; //default value
                
               if (!startedBalancing){    //balancing has not started so it is started
                  int minIndex = minimumIndex( SOC, (sizeof(SOC)/sizeof(float)) ); //identify the minimum state of charge between all the cells
  
                 //for each of the other cells, the SOC is matched to the one specified by the above found index
                 for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
                    if (index != minIndex){                               //the cell with the minimum SOC will not have its SOC changed
                      startDischarge(index, (SOC[index]-SOC[minIndex]));  //discharge each cell by an amount given by the difference between the SOC of this cell and the minimum SOC of all cells
                    }
                 }
               startedBalancing = true;
               
               } else {                 //wait for balancing to complete

                    
                    for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){

                      if(dischargingTime[index] > 0){
                        doneBalancing = false;
                        dischargingTime[index]--;
                        Serial.println("remaining discharging time (s) : " + String(dischargingTime[index]));
                      }

                   }

                   
               
                    if (doneBalancing) {
                      startedBalancing = false;   //reset the startedBalancingFlag
                      next_state = 1;             //balancing is complete so return to the charging state
                      ledBinary(1);
                      
                      for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){      //stop discharging the cells
                            stopDischarge(index);  
                        }
                 
                    } else {
                      next_state = 4;
                      ledBinary(2);
                    }
               }
  
              if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
                next_state = 0;
                
              }
              break;  

        
               
            }

      
      case 5:{ //Use state (-500mA) //TODO
        socAccumulateParralel(1, current_measure);
               current_ref = -500;

               if (measurePeriodTimer < measurePeriod) {

                  next_state = 5;
                  ledBinary(0);
                  measurePeriodTimer++;
     
              } else {  //it is now time to check the voltage of each cell
      
                next_state = 6;   //go to the state of rest 
                measurePeriodTimer = 0;
              }
      
              if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
                next_state = 0;
                
              }
              break;  

        
               
            }

      case 6:{ // Use rest
        current_ref = 0;
        if (rest_timer < 30) { // Rest here for 30s like before
          next_state = 6;
          
          rest_timer++;
        } else { // When thats done, move to checking the SOC of the cells
          next_state = 7;
          rest_timer = 0;
        }
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          
        }
        break;
      }

      case 7:{ //Measuring the SOC of each cell when they are discharged through use
        socAccumulateParralel(1, current_measure);
        current_ref = 0;
        for (int cellID = 0; cellID < ( sizeof(SOC)/sizeof(float) ); cellID++){

            //measure(cellID);note
            
          
        }

        
               
               if ((V_Cell[0] > 2500) && (V_Cell[1] > 2500)) { // Check to see if any of the cells have discharged

                  next_state = 5;  //stay in the use state
                  ledBinary(0);
                 
                 
               } else { // If the cells can continue discharging

                  next_state = 1; //if any of the cells has discharged, move to the charging state //note 3: 
                 ledBinary(1);
                 
                 
               }

               
              if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                next_state = 0;
                
              }
              break;
            }

            
      case 8: { // ERROR state RED led and no current
        current_ref = 0;
        next_state = 8; // Always stay here
        ledBinary(3);
        
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          
        }
        break;
      }

      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 8; // So if we are here, we go to error
        ledBinary(3);
      }
      
    }


   
   //end of state machine
    dataString = String(state_num) + String(current_ref) + "," + String(current_measure); //build a datastring for the CSV file
    for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
          dataString = dataString +","+String(SOC[index]);    //add to the string, the SOC of each cell
         }
         
   for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
    dataString = dataString +","+String(SOC[index]);    //add to the string, the SOC of each cell
        }
         
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
    int_count = 0; // reset the interrupt count so we dont come back here for 1000ms
  }
}

// Timer A CMP1 interrupt. Every 1000us the program enters this interrupt. This is the fast 1kHz loop
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
