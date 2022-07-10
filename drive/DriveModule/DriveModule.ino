/*
 * pin6 is PWM output at 62.5kHz.
 * duty-cycle saturation is set as 2% - 98%
 * Control frequency is set as 1.25kHz. 
*/

#include "drive.h"
#include "optical.h"
#include <math.h>

 int DIRRstate = HIGH;              //initializing direction states
 int DIRLstate = LOW;
 
 unsigned int loopTrigger;        //Chnage in SMPS Trigger

 int operation = 0;
 int operation_done = 0;
 int in = 0;

Line line;
Line line_old;

 float y_temp = 0;//for storing param[0]
 float x_temp = 0; //param[1]
 
 float y_prev = 0;
 float x_prev = 0;

 float x_vec,y_vec = 0;
 float x_imag = 0;

 float ang_old = 0;
 int speed_in = 2;
 int speed = 100;

 float turn_ang;

float total_x = 0;
float total_y = 0;

float total_x1 = 0;
float total_y1 = 0;

float x=0;
float y=0;

float distance_x=0;
float distance_y=0;

volatile byte movementflag=0;
volatile int xydat[2];

int tdistance = 0;

float total_distance = 0;

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y];

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  drive.SETUP(); //motor setup
  drive.pinSetup(); //pin setup
  drive.timerInit(); //Timer setup
  drive.writePWMState(0,0);  

  optical.pin_setup(); //Pin setup
  optical.spi_setup(); //SPI Setup
}


 void loop() {
  if(Serial1.available()){
     serialEvent(total_x, total_y);
  }
  
//  if (Serial.available()){
//    //drive.setVref(Serial.readStringUntil('?').toInt());
//    //drive.writePWMState(Serial.readStringUntil('?').toInt(), 0);
//
//    //operation = Serial.readStringUntil('?').toInt();
//    //in = Serial.readStringUntil('?').toInt();
//    serialEvent_pseudo(total_x, total_y);
//  }
  delay(2);

  int val = optical.mousecam_read_reg(ADNS3080_PIXEL_SUM);
  MD md;
  optical.mousecam_read_motion(&md);
  
//  for(int i=0; i<md.squal/4; i++) 
//    Serial.print('*');
//  Serial.print(' ');
//  Serial.print((val*100)/351);
//  Serial.print(' ');
//  Serial.print(md.shutter); Serial.print(" (");
//  Serial.print((int)md.dx); Serial.print(',');
//  Serial.print((int)md.dy); Serial.println(')');
  
  distance_x = md.dx; //convTwosComp(md.dx);
  distance_y = md.dy; //convTwosComp(md.dy);

  total_x1 = (total_x1 + distance_x);
  total_y1 = (total_y1 + distance_y);

  total_x = total_x1/157; //Conversion from counts per inch to mm (400 counts per inch)
  total_y = total_y1/157; //Conversion from counts per inch to mm (400 counts per inch)

  total_x= total_x*10;
  total_y = total_y*10;

 /*  Serial.print('\n');
  Serial.println("Distance_x = " + String(total_x));
  Serial.println("Distance_y = " + String(total_y));
  Serial.print('\n');  */
  
  unsigned long currentMillis = millis();

  drive.interrupt(loopTrigger);
//
//  switch(speed_in){
//    case 1: speed = 50; break;
//    case 2: speed = 100; break;
//    case 3: speed = 150; break;
//    case 4: speed = 200; break;
//    case 5: speed = 255; break;
//    default: speed = 100; break;
//  }

  speed = 100;
  if(operation == 1){
    total_x1 = 0;
    total_y1 = 0;
    operation = 0;
  }

  if (operation_done == 1){
//    Serial.println("d?");
    line_old = drive.mapLine(x_vec, y_vec);
    ang_old = line_old.angle;

    pair coords = drive.getCoords({ang_old, total_y}); //radians, mm x_m, y_m

    Serial.println("ang_old: " + String(ang_old) + "total_y: " + String(total_y));
    Serial.println("coords.x: " + String(coords.x) + "coords.y: " + String(coords.y));

    Serial.println("before:: x_prev: " + String(x_prev) + "y_prev: " + String(y_prev));
    x_prev = x_prev + coords.x; //x_temp 
    y_prev = y_prev + coords.y; //y_temp
    Serial.println("after:: x_prev: " + String(x_prev) + "y_prev: " + String(y_prev));

    total_distance += total_y;

    Serial.println("Total distance traversed so far: " + String (total_distance));

    operation_done = 0;
  }
  
  switch (opp_num)
  {
  case 0: //IDLE
    drive.writePWMState(0, 0);
    if (in == 1)
    { 
      opp_num = 1;
      operation = 1; //Recalibration
      in = 0;
    }
    break;
  case 1: //Turn
    turn_ang = line.angle - ang_old;
    turn_ang = Mod(turn_ang + PI, 2*PI) - PI;
    drive.turn(turn_ang, total_x, speed); //done: opp_num =2, operation =1

    x_imag = total_x;
    break;
  case 2: //Forward
    drive.forward(line.length,total_x, total_y, speed); //done: opp_num = 0, operation =1
    break;
  }
  drive.writeState(DIRRstate, DIRLstate); //Change state based on move
}

ISR(TCA0_CMP1_vect){ //Triggered when 
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loopTrigger = 1;
}
                                                     
void serialEvent(const float &tot_x, const float &tot_y) {
  if(Serial1.available()){ 
    
    String command = Serial1.readStringUntil('?');
    char command_type = command[0];
//    Serial.print("new command " + command);
    switch(command_type){
      case 'a' : {//getters
        pair coords = drive.getCoords({line.angle, tot_y}); //radians, mm

        int x_pr = x_prev + coords.x;//x_m       //x_temp 
        int y_pr = y_prev + coords.y; //y_m      //y_tem

        Serial1.println("a" + String(x_pr) + "," + String(y_pr) + "?"); //add previous coordinates
        break;
      }
      case 'd': {
        //stop
        operation = 1;
        operation_done = 1;
        opp_num = 0;
        break;
      }
      case 'e': {
        Serial1.print("e?");
        break;
      }
      case 'f':
      {
        in = 1;
        int start = 1;
        int index = 0;

        int param[2];

        for (int i = 1; i < command.length(); i++)
        {
          if (command[i] == ',')
          {
            param[index++] = (command.substring(start, i)).toInt();
            start = i + 1;
          }
        }

        //Serial.println("xxxxxxxxxxxx CASE xxxxxxxxxxxxxxxxxxxxxx");
        //Serial1.println("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        //Serial.println("Angle: "+ String(line.angle) + "   Length: " + String(line.length));
        
        x_temp = param[0] ; //1
        y_temp = param[1] ; //1

        x_vec = param[0] - x_prev;
        y_vec = param[1] - y_prev;

        speed_in = param[2];

        line = drive.mapLine(param[0] - x_prev, param[1] - y_prev);// x, y  // mapLine(1-0, 1-0) ang = 45 // mapLine(2-1, 0-1) ang = -45

        //Serial.println("param[0], x_temp: " + String(x_temp) + " param[1], y_temp: "+ String(y_temp));
        /* Serial.println("Angle: "+ String(line.angle) + "   Length: " + String(line.length));
        Serial.println("x_new: " + String(param[0] - x_prev) + " y_new: "+ String(param[1] - y_prev));
        
        Serial.println("speed_in: " + String(speed_in)); */
        
      }
     }
  }
}

void serialEvent_pseudo(const float &tot_x, const float &tot_y) {
  if(Serial.available()){ 
    String command = Serial.readStringUntil('?');
    char command_type = command[0];
    switch(command_type){
      case 'a' : {//getters
        pair coords = drive.getCoords({ang_old, tot_y}); //radians, mm

        int x_pr = x_prev + coords.x;//x_m       //x_temp 
        int y_pr = y_prev + coords.y; //y_m      //y_temp

        Serial.println(String(x_pr) + "," + String(y_pr) + "?"); //add previous coordinates
        break;
      }
      case 'd': {
        //stop
        operation = 1;
        operation_done = 1;
        opp_num = 0;
        break;
      }
      case 'e': {
        Serial.print("e?");
        break;
      }
      case 'f':
      {
        in = 1;
        int start = 1;
        int index = 0;

        int param[2];

        for (int i = 1; i < command.length(); i++)
        {
          if (command[i] == ',')
          {
            param[index++] = (command.substring(start, i)).toInt();
            start = i + 1;
          }
        }

        Serial.println("=================== Received Input ========================");
        //Serial1.println("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        //Serial.println("Angle: "+ String(line.angle) + "   Length: " + String(line.length));
        
        x_temp = param[0] ; //1
        y_temp = param[1] ; //1

        x_vec = param[0] -x_prev;
        y_vec = param[1] - y_prev;

        speed_in = 2 //param[2];
         
        line = drive.mapLine(param[0] - x_prev, param[1] - y_prev);// x, y  // mapLine(1-0, 1-0) ang = 45 // mapLine(2-1, 0-1) ang = -45

        //Serial.println("param[0], x_temp: " + String(x_temp) + " param[1], y_temp: "+ String(y_temp));
        Serial.println("x_in: " + String(param[0]) + " y_new: "+ String(param[1]));
        Serial.println("x_vec: " + String(param[0] - x_prev) + " y_vec: "+ String(param[1] - y_prev));
        Serial.println("Angle: "+ String(line.angle) + "   Length: " + String(line.length));
       
        Serial.println("speed_in: " + String(speed_in));
        
      }
     }
  }
}

template<typename T>
T Mod(T x, T y)
{
    if (0. == y)
        return x;
    double m= x - y * floor(x/y);
    // handle boundary cases resulted from floating-point cut off:
    if (y > 0)              // modulo range: [0..y)
    {
        if (m>=y)           // Mod(-1e-16             , 360.    ): m= 360.
            return 0;
        if (m<0 )
        {
            if (y+m == y)
                return 0  ; // just in case...
            else
                return y+m; // Mod(106.81415022205296 , _TWO_PI ): m= -1.421e-14 
        }
    }
    else                    // modulo range: (y..0]
    {
        if (m<=y)           // Mod(1e-16              , -360.   ): m= -360.
            return 0;
        if (m>0 )
        {
            if (y+m == y)
                return 0  ; // just in case...
            else
                return y+m; // Mod(-106.81415022205296, -_TWO_PI): m= 1.421e-14 
        }
    }
    return m;
}
