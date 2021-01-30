#pragma once
#include <Common.h>

struct Camera {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 vel;
    u8 keys;
};

void MoveCameraAlong(Camera& cam) {

    u8 keys = cam.keys;

    u8 w = ((keys >> 0) & 1);
    u8 a = ((keys >> 1) & 1);
    u8 s = ((keys >> 2) & 1);
    u8 d = ((keys >> 3) & 1);
    u8 space = ((keys >> 4) & 1);
    u8 shift = ((keys >> 5) & 1); 

    glm::vec2 horizontalForwardDir{cam.direction.x ,cam.direction.z};
    horizontalForwardDir = glm::normalize( horizontalForwardDir );
    glm::vec2 horizontalOrtoDir{horizontalForwardDir.y , -horizontalForwardDir.x};

    int8_t forward = w-s;
    int8_t ortogonal = a-d;

    const float speed = 0.00003f;

    cam.vel.x += (horizontalForwardDir.x * forward * speed) + (horizontalOrtoDir.x * ortogonal * speed);
    cam.vel.z += (horizontalForwardDir.y * forward * speed) + (horizontalOrtoDir.y * ortogonal * speed);

    cam.keys = 0;
}

void RotateCamera(Camera& cam , float vertRotAngle , float horizRotAngle) {

    float cosHoriz = cos(horizRotAngle);
    float sinHoriz = sin(horizRotAngle);

    float cosVert = cos(vertRotAngle);
    float sinVert = sin(vertRotAngle);

    cam.direction.x = cam.direction.x * cosHoriz - cam.direction.z * sinHoriz;
    cam.direction.z = cam.direction.x * sinHoriz + cam.direction.z * cosHoriz;

    cam.direction = glm::normalize(cam.direction);
    glm::vec3 Right = glm::normalize(glm::cross( cam.direction , glm::vec3(0,1,0) ));
    glm::vec3 w = glm::normalize(glm::cross( Right , cam.direction));
    
    cam.direction = cam.direction * cos(vertRotAngle) + w * sin(vertRotAngle);
    cam.direction = glm::normalize(cam.direction);
}

glm::mat4 LookAt(glm::vec3 from , glm::vec3 to , glm::vec3 worldUp = {0,1,0} ) {
    glm::vec3 forward{ glm::normalize(to - from) };
    glm::vec3 right{glm::normalize(glm::cross( forward , worldUp ))};
    glm::vec3 up{glm::normalize(glm::cross(right , forward))};
    
    return glm::mat4{
       right.x , up.x , -forward.x , 0,
       right.y , up.y , -forward.y , 0,
       right.z , up.z , -forward.z , 0,
       -glm::dot(right , from),-glm::dot(up , from),glm::dot(forward , from),1
    };
}

glm::mat4 PerspectiveMatrix(f32 fov , f32 aspect , f32 near , f32 far) {

    f32 tanFov = tan( fov * 0.5 );
    f32 x = 1 / ( aspect * tanFov );
    f32 y = 1 / ( tanFov );
    f32 z = -(far + near) / (far - near);
    f32 w = (-2 * far * near) / (far - near);

    return glm::mat4{
        x,0,0,0,
        0,y,0,0,
        0,0,z,-1,
        0,0,w,0
    };
}