#ifndef DRIVE_INTERPRETER_HPP
#define DRIVE_INTERPRETER_HPP
struct Drive{
    static double x;
    static double y;
    static double theta;
    static void getPositionFromRelativePolar(double r_dist, double r_theta, int &r_x, int &r_y);
};
#endif