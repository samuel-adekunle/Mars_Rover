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
unsigned int rest_time = 0;  //There is no rest time for the current implementation of the system
unsigned int loop_trigger;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
unsigned int slow_count = 0;
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float ui_max = 1, ui_min = 0; //anti-windup limitation
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0, error_amps; // Current Control
float pwm_out;
float V_Bat;
boolean input_switch;
int state_num,next_state=3;            //note that the initial state is the measure state
String dataString;
float V_Cell[] = {0, 0, 0};           //variable used to store the voltage of a measured cell
float chargingCurrentPerCell;   //variable used to store the current of each individual cell when they are connected in parralel and charging
float dischargingCurrentPerCell[] = {0, 0, 0}; //Current of each cell when the discharge circuit is being used
int dischargingTime[] = {0, 0, 0};
int measurePeriod = 180;
int measurePeriodTimer = 0;
float capacity[] = {599, 599, 599}; 
bool startedBalancing = false;
const float socCharacteristic[] = {0, 1.6559e-05, 3.3119e-05, 4.9678e-05, 6.6237e-05, 8.2797e-05, 9.9356e-05, 0.00011592, 0.00013137, 0.00014683, 0.00016228, 0.00017774, 0.00019319, 0.00020865, 0.0002241, 0.00034775, 0.00057958, 0.00069549, 0.00086936, 0.00098528, 0.0011592, 0.001391, 0.0017387, 0.0019706, 0.0020865, 0.0023183, 0.002782, 0.0030138, 0.003207, 0.0033615, 0.0035934, 0.0037093, 0.0038832, 0.004057, 0.0044048, 0.0046366, 0.0049844, 0.0051003, 0.0053707, 0.005448, 0.0055639, 0.0057378, 0.0059117, 0.0061435, 0.0066072, 0.006839, 0.0069549, 0.0071867, 0.0076504, 0.0081141, 0.0083459, 0.0084618, 0.0086936, 0.0091573, 0.009505, 0.009621, 0.0099687, 0.010316, 0.010548, 0.010664, 0.010896, 0.011128, 0.011418, 0.011592, 0.012229, 0.012635, 0.012751, 0.012982, 0.013098, 0.013562, 0.014142, 0.014489, 0.014605, 0.014837, 0.015301, 0.015649, 0.01588, 0.016228, 0.016576, 0.01704, 0.017097, 0.017155, 0.017445, 0.017967, 0.018431, 0.018662, 0.01901, 0.01959, 0.019937, 0.020285, 0.020517, 0.021097, 0.021444, 0.021908, 0.022314, 0.022835, 0.023299, 0.023531, 0.023879, 0.024458, 0.024806, 0.02527, 0.025675, 0.026545, 0.02666, 0.027124, 0.027472, 0.027994, 0.028399, 0.028863, 0.029172, 0.02979, 0.029906, 0.030718, 0.031297, 0.031645, 0.031761, 0.031993, 0.032804, 0.033615, 0.033963, 0.034427, 0.03489, 0.035586, 0.03605, 0.036629, 0.037035, 0.037209, 0.037904, 0.038658, 0.039179, 0.039643, 0.039759, 0.041382, 0.041614, 0.042251, 0.042657, 0.043642, 0.044048, 0.044453, 0.045033, 0.045439, 0.046366, 0.047293, 0.047757, 0.049264, 0.04938, 0.049728, 0.050307, 0.051119, 0.051814, 0.052452, 0.053147, 0.054016, 0.055176, 0.056219, 0.056567, 0.057726, 0.059233, 0.060624, 0.061435, 0.061551, 0.064333, 0.068854, 0.077084, 0.086473, 0.092616, 0.10293, 0.10629, 0.10769, 0.11742, 0.12136, 0.12809, 0.12948, 0.13435, 0.13817, 0.13968, 0.14814, 0.14884, 0.15498, 0.15799, 0.15869, 0.16298, 0.16669, 0.16889, 0.17213, 0.17839, 0.18303, 0.18407, 0.19265, 0.19451, 0.1981, 0.20227, 0.20436, 0.20992, 0.21636, 0.22105, 0.22511, 0.23253, 0.23994, 0.24226, 0.24841, 0.2513, 0.25977, 0.26463, 0.26765, 0.28596, 0.29495, 0.32433, 0.3569, 0.38148, 0.42379, 0.44314, 0.46262, 0.49078, 0.49623, 0.53124, 0.53518, 0.57135, 0.57795, 0.5805, 0.58792, 0.61238, 0.61968, 0.6322, 0.64124, 0.65167, 0.65701, 0.6737, 0.69474, 0.72528, 0.74313, 0.76353, 0.78376, 0.79726, 0.80677, 0.81929, 0.82856, 0.84299, 0.86067, 0.87377, 0.87678, 0.89034, 0.89765, 0.90078, 0.90848, 0.91411, 0.91967, 0.92721, 0.92732, 0.93149, 0.93694, 0.94424, 0.94564, 0.95074, 0.95305, 0.95572, 0.95839, 0.95966, 0.96001, 0.96331, 0.96581, 0.96766, 0.96928, 0.9723, 0.97299, 0.97461, 0.97612, 0.97693, 0.97798, 0.9789, 0.97948, 0.98064, 0.98122, 0.98296, 0.98366, 0.98435, 0.98516, 0.98539, 0.98748, 0.9876, 0.98806, 0.98876, 0.9891, 0.99015, 0.99084, 0.99165, 0.99189, 0.99235, 0.9927, 0.99293, 0.99432, 0.99461, 0.9949, 0.99502, 0.99536, 0.99606, 0.99652, 0.99699, 0.99757, 0.99815, 0.99838, 0.99872, 0.99907};
int cellMeasuredID = 0;
int relayWaitTime = 1;        //the number of seconds waited for the relay to settle
int relayWaitTimer = 0;       //timer used to count the number of seconds waited for the relay to settle

int minIndex = 0;

float SOC[] = {0, 0, 0};   //State of Charge (SOC) for each of the cells (in mAh)
float previousCycle_SOC[] = {0,0,0};
float totalCapacity;   //The total capacity of all cells combined
float cycles[] = {7, 7, 7}; //note TODO


int operation_mode = 0; //This indicates the mode in which the SMPS may operate in. 
                        //0 indicates constant current buck,
                        //1 indicates constant voltage buck
                        //2 indicates constant voltage boost

float output_voltage = 0; //the desired output voltage of the smps
float CV_stop_current_ma = 4.5; //The per cell current limit bellow which 
                               //the batteries will be considered charged when 
                               //in the constant voltage charging mode TODO: find

int CV_settling_timer = 10; //the waiting time after which the the constant voltage buck output has reached its final value (exaggerated)

bool on = false;               //This is true when the batteries are discharging through 
                               //the boost to provide a 5V output 
                               //(set to true when corresponding message is recieved)
                               
bool off = false;               //This is true when the batteries are to stop powering other 
                               //modules and start charging 
                               //(set to true when corresponding message is recieved)

                               

float u0v,u1v,delta_uv,e0v,e1v,e2v; // u->output; e->error; 0->this time; 1->last time; 2->last 
float uv_max=4, uv_min=0; //anti-windup limitation
float kpv=0.05024,kiv=15.78,kdv=0; // voltage pid.

float instantaneous_power;
float power = 2;
float previous_power = 1;

float duty_step = 0.005;

int n = 40;              //number of previous values to average for moving average algorithm
int moving_average_index = 0;                  //the index used for the moving average algorithm
float powerValues[40];     //array that holds n previous values for moving average algorithm




  
  
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

  if (SD.exists("cycle.csv")) { // Retrieve the previously saved cycle count of each cell (executed once on start up)
    File dataFileCycles = SD.open("cycle.csv", FILE_WRITE); // open our CSV file

    String totalCyclesContents = "";
    char nextChar = ' ';
    int characterCount = 0;
    while (dataFileCycles.available()){//
      nextChar = dataFileCycles.read();
      totalCyclesContents = totalCyclesContents + nextChar;
      characterCount++;
    }
    char totalCycleCharacters[characterCount];
    totalCyclesContents.toCharArray(totalCycleCharacters, characterCount);
    
    Serial.println("Total Cycles Retrieved as: " + totalCyclesContents);
    sscanf(totalCycleCharacters, "%f,%f,%f", &cycles[0], &cycles[1] ,&cycles[2]);
    
    //TODO retrieve from SD
   
  
  
  }

  
  noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  //SMPS Pins
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

  
  pinMode(7, OUTPUT);// This is the output used to control the relay pin of cell 2
  pinMode(8, OUTPUT);// This is the output used to control the discharging of cell 2

  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A1, INPUT);


  pinMode(5, OUTPUT);  // This is the output used to control the relay pin of cell 0
  pinMode(4, OUTPUT); // This is the output used to control the relay pin of cell 1


  
  pinMode(9, OUTPUT);   // This is the output used to control the discharging of cell 0
  pinMode(10, OUTPUT); // This is the output used to control the relay pin of cell 1

  pinMode(A2, INPUT); //This is the pin used to measure the voltage of cell 0

  pinMode(A7, OUTPUT); //This pin is used to control the relays at port A switching between the solar panel connection and the circuit to be powered

  
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
        Serial1.println(transmittedString);           
        break;
      }
      
      case 'b': { //number of cycles per cell 

             String transmittedString = "b";
         
             for (int index = 0; index < ( sizeof(cycles)/sizeof(float) ); index++){
              transmittedString = transmittedString +","+String(cycles[index]);    //add to the string, the percentage charge of the cell given by the index
             }
    
             transmittedString = transmittedString+"?";
    
    
            //The structure of the transmitted string is: (number of cycles of cell 0), (percentage charge of cell 1), (total percentage charge)
            Serial1.println(transmittedString);          
            break;
          }
  
      

      case 'c': { //check for if the batteries are fully charged and ready
             
             if (next_state == 6){
              Serial.println("c,1?");     //ready
             } else {
              Serial.println("c,0?");     //not ready
             };
             
            break;
      }
      
      
      case 'd': { //request for battery engagement
             Serial.println("battery engaged");
             next_state = 7;             //the next state is the use state
             operation_mode = 2;        //The smps is set as a boost operating at constant voltage mode
             on = true;
             
            break;
      }

      case 'e': { //request for battery disengagement
             Serial.println("battery disengaged");
             next_state = 3;             //the next state is the measuring state
             operation_mode = 0;        //The smps is set as a buck operating at constant current mode
             off = true;
             
            break;
      }
      
       
     }
  }


}

void loop() {

  if (loop_trigger == 1){ // FAST LOOP (1kHZ)

      state_num = next_state; //state transition
      V_Bat = analogRead(A1)*4.096/1.037; 
      
      if ((state_num != 3) && (state_num != 4)) { //Checking for Error states (does not include voltage errors in states of charge_rest and measurement as pin A1 will be floating due to the measurement taking place) 
          if ((V_Bat > 3600) && (state_num != 5) && (state_num != 6) && (state_num != 7)) {   
             next_state = 5;    //move to the constant voltage charging stage
             operation_mode = 1;    
          } else if (V_Bat < 2500) {    
            state_num = 8; //go to the error state 
            operation_mode = 0;   
            next_state = 8; // stay in the error state      
            current_ref = 0; // no current    
          }
      }
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      
      switch(operation_mode) {

        case 0:{ //constant current buck

          digitalWrite(A7, LOW);  //connect the solar panels to port A

          error_amps = (current_ref - current_measure) / 1000; //PID error calculation
          pwm_out = pidi(error_amps); //Perform the PID controller calculation
          pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
          analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
          break;
        }
        
        case 1:{ //constant voltage buck
          
          digitalWrite(A7, LOW);  //connect the solar panels to port A
          
          float current_limit = 3;
          float error_voltage = output_voltage - V_Bat;   //the voltage error at this time
          float cv = pidv(error_voltage);                            //the voltage pid
          cv = saturation(cv, current_limit, 0); //current demand is saturated within current limit
          error_amps = cv - (current_measure)/1000;
          pwm_out = pidi(error_amps); //Perform the PID controller calculation
          pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
          analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
          break;
        }

        case 2:{ //constant voltage boost

         output_voltage = 5;
         digitalWrite(A7, HIGH);  //connect the other subsystems to be powered to port A
         
         float current_limit = 2; 
         float va = analogRead(A0)*10.808/1000;    
         Serial.println(va);    
         float error_voltage = output_voltage - va;   //the voltage error at this time             
         Serial.println("error voltage: "+String(error_voltage));   
         float cv = pidv(error_voltage);
         cv = saturation(cv, current_limit, 0); //current demand is saturated within current limit             
         Serial.println("cv: "+String(cv));            
         error_amps = cv - (-current_measure)/1000;    
         Serial.println("current: "+String((-current_measure)/1000));   
         Serial.println("error amps: "+String(error_amps));   
         pwm_out = pidi(error_amps); //Perform the PID controller calculation 
         Serial.println("PWM: "+String(pwm_out));   
         pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation            
         analogWrite(6, (int)(pwm_out*255)); // write it out 
         break;
        }

        case 3:{ //MPPT
          digitalWrite(A7, LOW);  //connect the solar panels to port A
          
          instantaneous_power = (V_Bat/1000) * (current_measure/1000) * 1000;      //update the current value of power in mW
          power = movingAverage(instantaneous_power);

          if (moving_average_index == 0){  //the below correction for the MPPT tracking occurs when n values have been averaged

              
              //workout the size of the step as a linear function of the difference in performance provided by the previous duty cycle change
              float powerDifference = power - previous_power;
              float linearFactor = 10000;
              float absolute_duty_step = fabs(powerDifference/duty_step/linearFactor);
              
              absolute_duty_step = saturation(absolute_duty_step, 0.1, 0.005);  //staurate the duty cycle step between 0.5% and 10%
              
              //duty_step = float(sign(duty_step))*absolute_duty_step; //uncomment this to enable smart mppt
              
              //check if the previous step led to an improvement in the power
                if (power<previous_power) {         //If this is true, the previous step led to a decrease of power
                  if ( current_measure < (250*3) ){             //check if the current limit is respected
                    duty_step = -duty_step;                     //the step is inverted to provide a power increase
                  } 
                    
                } else {
                  if ( current_measure > (250*3) ){             //check if the current limit is being exceeded
                    duty_step = -duty_step;                     //the step is inverted to provide a power decrease so as to respect the current limit
                  } 
                }
        
              //The change to the duty cycle is made according to the set step
              pwm_out = pwm_out + duty_step; 
               pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
              analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)

              previous_power = power;             //store the current power as previous power, to be used during the next execution of this loop
            }
            
          break;
        }



      
      }
      
        
        int_count++; //count how many interrupts since this was last reset to zero
        slow_count++; //count how many interrupts since this was last reset to zero
      
      

      if(Serial1.available()){ //note s: kala edw?                
           serialEvent();
        }

      
      loop_trigger = 0; //reset the trigger and move on with life
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)
    input_switch = digitalRead(2); //get the OL/CL switch status
   
   //state machine


 
 
 switch (state_num) { // STATE MACHINE 
      case 0:{ // Idle state (no current, no LEDs)
        current_ref = 0;
        if (input_switch == 1) { // if switch, move to charge
          next_state = 1;
          operation_mode = 3; //The buck is set to MPPT mode
        } else { // otherwise stay put
          next_state = 0;
          operation_mode = 0;                             //The buck is set to constant current mode
          
        }
        break;
      }

      
      case 1:{ // Charge state (750mA and a green LED) //Changed from 250mA as there are now three batteries in parralel
        socAccumulateParralel(1, current_measure);
        Serial.println("Charge State (measurePeriodTimer " + String(measurePeriodTimer) + "/" + String(measurePeriod) + ")");
        

        if (measurePeriodTimer < measurePeriod) {

            next_state = 1;
            operation_mode = 3; //The buck is set to MPPT mode
            measurePeriodTimer++;
     
        } else {  //it is now time to check the voltage of each cell

          next_state = 2;
          operation_mode = 0; //The buck is set to constant current mode
          measurePeriodTimer = 0;
        }

        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          operation_mode = 0;                             //The buck is set to constant current mode
          
        }
        break;  
        
      }

      
      case 2:{ // Charge Rest, green LED is off and no current
         Serial.println("Charge Rest (rest_timer " + String(rest_timer) + "/30)");
        current_ref = 0;
        
        
        if (rest_timer < rest_time) { // Stay here if timer < 30
          next_state = 2;
          operation_mode = 0; //The buck is set to constant current mode
          
          rest_timer++;
        } else { // Or move to measuring the SOC (and reset the timer)
          next_state = 3;
          operation_mode = 0; //The buck is set to constant current mode
          
          rest_timer = 0;
        }
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          operation_mode = 0;  //The buck is set to constant current mode
          
        }
        break;        
      }


      case 3:{ //Measuring the SOC of each cell
        Serial.println("Measuring SOC of each cell");
        current_ref = 0;

        measure(cellMeasuredID, relayWaitTimer, true);
        relayWaitTimer++;
        
        if (relayWaitTimer > relayWaitTime) {
          relayWaitTimer = 0;
          cellMeasuredID = cellMeasuredID + 1;
        }
        
        

        
            if (cellMeasuredID >= ( sizeof(SOC)/sizeof(float) )) {   //all cell measurements have been taken and we can now move on to the next state
               Serial.println("all cells have been measured");
               cellMeasuredID = 0;                                   //reset this index
               
                 if ((V_Cell[0] < 3600) && (V_Cell[1] < 3600) && (V_Cell[2] < 3600)) { // Check to see that no cell has reached maximum voltage
                 
                 next_state = 4; //transition to the balancing state
                 operation_mode = 0; //The buck is set to constant current mode
                 
               } else { // If the voltage of any cell has reached the voltage limit, the next state will be that of constant voltage charging
                 next_state = 5;  //move to the Constant Voltage charging state
                 operation_mode = 1; //The buck is set to operate in a closed loop constant voltage mode
               }

            } else {
              next_state = 3; //stay in the same state
              operation_mode = 0; //The buck is set to constant current mode
            }

            
              if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                next_state = 0;
                operation_mode = 0;                             //The buck is set to constant current mode
                
              }
              break;
            }

      case 4:{ //Balancing state
        Serial.println("Balancing");
               current_ref = 0;
                bool doneBalancing = true; //default value
                
               if (!startedBalancing){    //balancing has not started so it is started

                  //disconnect all cells from the SMPS port to avoid current flowing while balancing
                  digitalWrite(5,true); 
                  digitalWrite(4,true); 
                  digitalWrite(7,true); 
                  
                  minIndex = minimumIndex( SOC, (sizeof(SOC)/sizeof(float)) ); //identify the minimum state of charge between all the cells
  
                 //for each of the other cells, the SOC is matched to the one specified by the above found index
                 for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
                    if ((index != minIndex) && ((SOC[index]-SOC[minIndex]) > 2) ){                               //the cell with the minimum SOC will not have its SOC changed
                      startDischarge(index, (SOC[index]-SOC[minIndex]));  //discharge each cell by an amount given by the difference between the SOC of this cell and the minimum SOC of all cells
                    }
                 }
               startedBalancing = true;
               
               } else {                 //wait for balancing to complete

                    
                    
                    for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){

                        if (index != minIndex){                               //the cell with the minimum SOC will not have its SOC changed
                      
                            if((dischargingTime[index] > 0) ){              
                              doneBalancing = false;
                              socAccumulateBalancing(index, 1);             //accumulation of charge during discharge to calculate SOC
                              
                              //measure(index, relayWaitTime, false); //as the measure pin is alredy high, the voltage of the cell is measured and the SOC is calculated
                              dischargingTime[index]--;
                              Serial.println("remaining discharging time (s) (cell "+String(index)+"): " + String(dischargingTime[index]));
                            } else {
                              stopDischarge(index, false);
                            }
                        }
                   }

                   
               
                    if (doneBalancing) {
                      startedBalancing = false;   //reset the startedBalancingFlag
                      next_state = 1;             //balancing is complete so return to the charging state
                      operation_mode = 3;         //The buck is set to MPPT mode
                      
                      for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){      //stop discharging the cells
                            stopDischarge(index, true);  
                        }
                 
                    } else {
                      next_state = 4;
                      operation_mode = 0; //The buck is set to constant current mode
                    }
               }
  
              if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
                next_state = 0;
                operation_mode = 0;                             //The buck is set to constant current mode
                
              }
              break;  

        
               
            }

        case 5:{ //Constant voltage charging mode //TODO: test
                socAccumulateParralel(1, current_measure);
                operation_mode = 1; //The buck is set to operate in a closed loop constant voltage mode
                output_voltage = 3.6; //The buck output voltage is set to the cell upper voltage limit

                 if ((current_measure > (3*CV_stop_current_ma)) || (CV_settling_timer > 0)){    //the current measured is still above the limit set and so CV charging contnues

                  next_state = 5;                                 //stay in the constant voltage charging state
                  operation_mode = 1; //The buck is set to operate in a closed loop constant voltage mode
                  
                } else {                                          //the current measured has reached the limit set and so the batteries are now fully charged
                  next_state = 6;                                 //move to the charged/ready state
                  operation_mode = 0;                             //The buck is set to constant current mode
                  CV_settling_timer = 10;
                }
                  CV_settling_timer--;

                if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                    next_state = 0;
                    operation_mode = 0;                             //The buck is set to constant current mode
                    
                  }

                      break;                 
                }
      

         case 6:{ //Ready/Charged state
          //In this state, the batteries are charged and resting and waiting for an engage signal so that they can be used
                  socAccumulateParralel(1, current_measure);
                  current_ref = 0;

                  if (on) {   //The message to provide power to the other modules has been recieved
                    next_state = 7;  //The next state is the use state
                    operation_mode = 2;                             //The smps is set as a boost operating at constant voltage mode
                    on = false;
                  } else {
                    next_state = 6;   //stay in this state
                    operation_mode = 0;                             //The buck is set to constant current mode
                  }
                  
                  if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                    next_state = 0;
                    operation_mode = 0;                             //The buck is set to constant current mode
                    
                  }
                  break;
        }

        case 7: { //USE state
        socAccumulateParralel(1, current_measure);
        output_voltage = 5;               //The output voltage is set to 5V
                                          //This will be provided by the boost to power the other modules
                                          
        operation_mode = 2;              //The smps is set as a boost operating at constant voltage mode

        for (int cellID = 0; cellID < ( sizeof(SOC)/sizeof(float) ); cellID++){   //checking if the SOC of any of the cells has reached 0

            if (SOC[cellID] <= 0) {
              next_state = 3;                                  //move to the measure state
              operation_mode = 0;                             //The buck is set to constant current mode
            }
            
          
        }

        if (V_Bat<2500) {
              next_state = 1;                                  //move to the charging state
              operation_mode = 3;                             //The buck is set to constant current mode
        }

        if (off) {           //the message has been recieved that the batteries are to be disengaged and charged

          next_state = 3;                   //move to the measure state
          operation_mode = 0;              //The smps is set as a buck operating at constant voltage mode
          off = false;
          
        } else {
          next_state = 7;                   //stay in the same state
          operation_mode = 2;              //The smps is set as a boost operating at constant voltage mode
        }

          if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
                    next_state = 0;
                    operation_mode = 0;                             //The buck is set to constant current mode
          }
          break;
        }

            
      case 8: { // ERROR state RED led and no current
        current_ref = 0;

          if (on) {   //The message to provide power to the other modules has been recieved
                      next_state = 7;  //The next state is the use state
                      operation_mode = 2;                             //The smps is set as a boost operating at constant voltage mode
                      on = false;
                  } else if (off) {           //the message has been recieved that the batteries are to be disengaged and charged

                        next_state = 3;                   //move to the measure state
                        operation_mode = 0;              //The smps is set as a boost operating at constant voltage mode
                        off = false;
                  } else {
                    next_state = 8;                                 //stay in this state
                    operation_mode = 0;                             //The buck is set to constant current mode
                  }
        
        break;
      }

      default :{ // Should not end up here ....
        Serial.println("Error Encountered");
        current_ref = 0;
        next_state = 8; // So if we are here, we go to error
      }
      
    }


   
   //end of state machine
   
   float V_PD = analogRead(A0)*10.808;
   
   
    dataString = String(state_num) +","+ String(operation_mode) +","+ String(current_ref) + "," + String(current_measure)+ "," + String(V_Bat) + "," + String(V_PD); //build a datastring for the CSV file
    for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
          dataString = dataString +","+String(SOC[index]);    //add to the string, the SOC of each cell
         }
         
   for (int index = 0; index < ( sizeof(SOC)/sizeof(float) ); index++){
    dataString = dataString +","+String(SOC[index]) +","+String(V_Cell[index]);    //add to the string, the SOC and Voltage of each cell
    
        }

     dataString = dataString + ","+String(pwm_out) + ","+String(power) ;
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

  if (slow_count == 20000) {      //slow loop executes every 30 seconds
    if (SD.exists("cycle.csv")){
      SD.remove("cycle.csv");
    }
    
        File dataFileCycles = SD.open("cycle.csv", FILE_WRITE); // open our CSV file
        if (dataFileCycles){ //If we succeeded (usually this fails if the SD card is out)
              String cycleData = "";
          for (int index = 0; index < ( sizeof(cycles)/sizeof(float) ); index++){
              float cycles_progress = (float(SOC[index]) - float(previousCycle_SOC[index]))/float(capacity[index]);   //the progress in cycles between this and the 
              if (cycles_progress > 0) {
                cycles[index] = cycles[index] + cycles_progress;    //only if the cell is charging, will the total amount charged in the last second (as a percentage of a full charge) be added to the total cycles
              }
              cycleData = cycleData +","+String(cycles[index]);    //add to the string, the number of cycles of each cell
              previousCycle_SOC[index] = SOC[index];
            }
          dataFileCycles.println(cycleData); // print the data
        } else {
          Serial.println("File not open"); //otherwise print an error
        }
        dataFileCycles.close(); // close the file

        slow_count = 0; //reset the interrupt count so that this code is not executed for 20000ms
  }
}




void startDischarge(int cellIdentificationNumber, float dischargeAmount) {                        

  dischargingCurrentPerCell[cellIdentificationNumber] = V_Cell[cellIdentificationNumber]/150; //current through the cell when discharging is calculated from ohms law 
                                                                                              //considering the voltage across the two parralel 300Ω resistors
                                                                                             

  dischargingTime[cellIdentificationNumber] = round((dischargeAmount/dischargingCurrentPerCell[cellIdentificationNumber])*3600); //The discharging time required expressed in seconds
  switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  digitalWrite(9,true); //select cell 0 for discharging
                  digitalWrite(5,true); //disconnect parralel connection to other cells
                  break;
                  
        }
  
        case 1:{  //cell 1 selected
                  digitalWrite(3,true); //select cell 1 for discharging
                  digitalWrite(4,true); //disconnect parralel connection to other cells
                  break;
        }

        case 2:{  //cell 2 selected
                  digitalWrite(8,true); //select cell 2 for discharging
                  digitalWrite(7,true); //disconnect parralel connection to other cells
                  break;
        }
  
    }

  Serial.println("Starting Discharge of cell " + String(cellIdentificationNumber) + ". Discharge Amount = "+ String(dischargeAmount)+". Discharge Time (s) = "+ String(dischargingTime[cellIdentificationNumber]));
}


void stopDischarge(int cellIdentificationNumber, bool stopMeasure) {                        

  switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  digitalWrite(9,false); //stop cell 0 discharging
                  digitalWrite(5,(!stopMeasure)); //reconnect parralel connection to other cells
                  break;
        }
  
        case 1:{  //cell 1 selected
                  digitalWrite(3,false); //stop cell 1 discharging
                  digitalWrite(4,(!stopMeasure)); //reconnect parralel connection to other cells
                  break;
        }

        case 2:{  //cell 2 selected
                  digitalWrite(8,false); //stop cell 2 discharging
                  digitalWrite(7,(!stopMeasure)); //reconnect parralel connection to other cells
                  break;
        }
  
    }

  Serial.println("Stopped Discharge of cell " + String(cellIdentificationNumber));
}


void socLUT(int cellIdentificationNumber) {

  
    //rounding to the nearest 2 
    float compensation_factor = 1.000;
    float currentCompensationFactor = 57;
    int cellVoltageRounded = 2*((round((V_Cell[cellIdentificationNumber]+currentCompensationFactor)*compensation_factor))/2);

    //accounting for the offset in the SOC characteristic array whose first element correponds to 2990
    int cellVoltageIndex = (cellVoltageRounded-2990)/2;
    if (cellVoltageIndex < 0){    //ensure that cell voltages out of the characterised range do not give undefined SOC results
      cellVoltageIndex = 0;
    } else if (cellVoltageIndex > 305) {
      cellVoltageIndex = 305;
    }
  SOC[cellIdentificationNumber] = socCharacteristic[cellVoltageIndex]*capacity[cellIdentificationNumber]; //retrieving the state of charge from the look up table array for the specific cell
  Serial.println("SOC of cell " + String(cellIdentificationNumber) + " is " + String(SOC[cellIdentificationNumber]) + "(index "+String(cellVoltageIndex)+")");
   
}

void socAccumulateParralel(float accumulationTime, float currentMeasured) {    


  chargingCurrentPerCell = currentMeasured/( sizeof(SOC)/sizeof(float) ); 
  
  for (int cellId = 0; cellId < ( sizeof(SOC)/sizeof(float) ); cellId++) {
    SOC[cellId] += chargingCurrentPerCell*(accumulationTime/3600);        
  }

  
}

void socAccumulateBalancing(int cellId, float accumulationTime){

  SOC[cellId] -= dischargingCurrentPerCell[cellId]*(accumulationTime/3600);
}

void measure(int cellIdentificationNumber, int timer, bool revertRelayAfterMeasurement) { //note
  
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

        case 2:{  //cell 2 selected
                  digitalWrite(7,true); //select cell 2 for measurement by turning on the relay of this cell
                  
                  break;
        }

        
      }
  } else if (timer >= relayWaitTime) { //at the end of the wait time, the relay has settled so the value is measured

      switch (cellIdentificationNumber) { //choosing which cell to measure
        case 0:{  //cell 0 selected
                  
                  V_Cell[0] = analogRead(A2)*4.096*0.9747; //measure the cell voltage 
                  if (revertRelayAfterMeasurement) {
                    digitalWrite(5,false); //stop measuring
                  }
                  break;
        }
  
        case 1:{  //cell 1 selected
                  
                  V_Cell[1] = analogRead(A3)*4.096*0.9747; //measure the cell voltage 
                  if (revertRelayAfterMeasurement) {
                    digitalWrite(4,false); //stop measuring
                  }
                  break;
        }

        case 2:{  //cell 2 selected
                  
                  V_Cell[2] = analogRead(A6)*4.096*0.9747; //measure the cell voltage 
                  if (revertRelayAfterMeasurement) {
                     digitalWrite(7, false); //stop measuring
                  }
                  break;
        }

        
      }
      
      Serial.println("Voltage of cell "+String(cellIdentificationNumber)+" is "+String(V_Cell[cellIdentificationNumber]));
      
      
      
      socLUT(cellIdentificationNumber);                             //the SOC of the battery is updated using the look up table method for the measured voltage
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

float sum = 0;                    //the sum of n previous values for the moving average algorithm 
float movingAverageValue;

float movingAverage(float currentPower){
  

  sum = sum - powerValues[moving_average_index] + currentPower;  //subtracting the previous value at the current index and adding the new one in its place
  powerValues[moving_average_index] = currentPower;
  if (moving_average_index < (n-1)) {
    
    moving_average_index = moving_average_index + 1;
    
  } else {
  
    moving_average_index = 0;
  
  }

  movingAverageValue = float(sum/float(n));
  
  return movingAverageValue;
   
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

float pidv( float pid_input){
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
