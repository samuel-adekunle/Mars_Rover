#ifndef drive_h
#define drive_h

extern int DIRRstate;              //initializing direction states
extern int DIRLstate;

extern int operation;
extern int opp_num;
extern int in;
extern int operation_done;

extern float vref;

extern float x_temp;
extern float y_temp;

extern int speed_in;
extern int speed;

extern float r;

#define DISTANCE_STATE 2
#define ANGLE_STATE 3

#define DIST_ERR 0.1
#define ANGLE_ERR 0.1

struct Line{
  float angle;
  float length;
};

struct pair{
  float x;
  float y;
};

class DriveClass{
  public: 
    DriveClass();
    void SETUP(); //Call in Setup
    void pinSetup(); //Call in Setup
    void timerInit(); //Call in Setup
    
    void sampling();
    void operationSetup(bool Boost_mode, bool CL_mode);
    float saturation( float sat_input, float uplim, float lowlim);
    void pwm_modulate(float pwm_input);
    int pwm_modulate_gen(float pwm_input);
    void setVref(float vref_t);
    
    float pidv( float pid_input);
    float pidi(float pid_input);

    void writeState(int DIRR, int DIRL);
    void writePWMState(int pwmrState, int pwmlState);
  
    void turn(float, float, int);
    void forward(float, float, float, int);

    void printvals();

    Line mapLine(float x, float y);
    pair getCoords(Line line);

    void rebaseCoords();
    float Angle_PID(int input);
    float Distance_PID(int input);
    pair Line_PWM_PID(int bearing, float dist);

    void interrupt(int loopTrigger);
    pair Line_PWM_PID_alt(float bearing);

};

extern DriveClass drive;

#endif 