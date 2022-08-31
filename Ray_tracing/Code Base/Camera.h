#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <cstdlib>
#include <iostream>
#include <cmath>

#include "Vector3.h"

class Camera
{
    public:
        Vector3 position;
        Vector3 normal;
        
        Camera();
        Camera(const Vector3& pos, const Vector3& norm);
        Camera(float x, float y, float z, float norX, float norY, float norZ);
};

#endif