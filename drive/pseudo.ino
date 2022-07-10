
void setup(){

}

struct Path{
    Point1;
    Point2;
    Point3;
    Point4;
} path;

void loop(){
    x = getX();
    y = getY();
    angle = angle();

    if(at destination){
        remove current  path;
    }
    setMotorSpeedGivenCurrentXYandDesiredXY();
}

void serialEvent(){
    while(Serial.avaliable()){
        if(stop command){
            stop doing path
        }
        if(path command){
            replace current path with new path
        }
    }
}