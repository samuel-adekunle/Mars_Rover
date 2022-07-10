// some structure for coordinates for the bounding boxes

// some structure for the bounding boxes i.e. two coords

// array of 4 integers i.e. [x_min, y_min, x_max, y_max]
#include "../include/vision_interpreter.hpp"
#include <BasicLinearAlgebra.h>
#include <math.h>

const unsigned int FOCAL_LENGTH = 1292;
const double BALL_DIAMETER = 4;

const unsigned int IMAGE_WIDTH = 640;
const unsigned int IMAGE_HEIGHT = 480;

double get_distance(unsigned int bb[4]) {
    using namespace BLA;
    unsigned int diameter = (bb[2] - bb[0] > bb[3] - bb[1]) ? bb[2] - bb[0] : bb[3] - bb[1];
    return (BALL_DIAMETER * FOCAL_LENGTH)/diameter;
}

double get_angle(unsigned int bb[4]) {
    using namespace BLA;
    BLA::Matrix<3, 3> A;
    A << FOCAL_LENGTH, 0, IMAGE_WIDTH/2,
       0, FOCAL_LENGTH, IMAGE_HEIGHT/2,
       0, 0, 1;

    unsigned int x_i = (bb[2] + bb[0])/2;
    unsigned int y_i = (bb[3] + bb[1])/2;

    BLA::Matrix<3, 3> A_inv = A.Inverse();

    BLA::Matrix<3> v;
    v(0) = x_i;
    v(1) = y_i;
    v(2) = 1;

    // ray from centre of the ball

    BLA::Matrix<3> r = A_inv * v;

    //todo: optimise this into a constant vector :)

    BLA::Matrix<3> v_c;
    v_c(0) = IMAGE_WIDTH/2;
    v_c(1) = IMAGE_HEIGHT/2;
    v_c(2) = 1;
    // ray from centre of the image
    BLA::Matrix<3> centre = A_inv * v_c;

    // ignore height differential from angle calculation, so zero out the height deviation, this could be the wrong index?

    r(1) = 0;
    centre(1) = 0;

    BLA::Matrix<1,3> centre_transpose = ~centre;
    // dot product of the two rays

    BLA::Matrix<1> res = centre_transpose * r;
    double dot = res(0);
    double length_c = sqrt(centre(0)*centre(0) + centre(1)*centre(1) + centre(2)*centre(2));
    double length_r = sqrt(r(0)*r(0) + r(1)*r(1) + r(2)*r(2));

    double cos_angle = dot / (length_c * length_r);

    int sign = (x_i < IMAGE_WIDTH/2) ? 1 : -1;
    return sign * acos( cos_angle);
}

// double getAngle(unsigned int bb[4]) {
//     Eigen::Matrix3d m;
//     m << FOCAL_LENGTH, 0, IMAGE_WIDTH/2,
//        0, FOCAL_LENGTH, IMAGE_HEIGHT/2,
//        0, 0, 1;
//     Eigen::Matrix3d m_inv = m.inverse();

//     unsigned int x_i = (bb[2] - bb[0])/2;
//     unsigned int y_i = (bb[3] - bb[1])/2;

//     Eigen::Vector3d v(x_i, y_i, 1);
//     Eigen::Vector3d r = m_inv * v;
//     r(1) = 0;

//     Eigen::Vector3d centre(0, 0, 1);
//     double res = centre.dot(r)/(centre.norm()*r.norm());
//     return acos(res);
// }