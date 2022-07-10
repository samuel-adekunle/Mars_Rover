#include <math.h>
#include "../include/drive_interpreter.hpp"
double Drive::x        = 50;
double Drive::y        = 50;
double Drive::theta    = 0;

void Drive::getPositionFromRelativePolar(double r_dist, double r_theta, int &r_x, int &r_y){
    double comb_theta = r_theta + theta;

    r_x = x + r_dist * sin(comb_theta);
    r_y = y + r_dist * cos(comb_theta);

    return;
}
