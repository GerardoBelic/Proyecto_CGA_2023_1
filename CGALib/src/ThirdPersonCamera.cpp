#include "Headers/ThirdPersonCamera.h"

ThirdPersonCamera::ThirdPersonCamera(){
    pitch = glm::radians(20.0f);
    yaw = 0.0f;
    angleAroundTarget = 0.0f;
    angleTarget = 0.0;
    distanceFromTarget = 1.0f;
    sensitivity = SENSITIVTY;
    worldUp = glm::vec3(0.0, 1.0, 0.0);
    updateCamera();
}

void ThirdPersonCamera::mouseMoveCamera(float xoffset, float yoffset, float dt){
    // Camera controls
    float cameraSpeed = sensitivity * dt;

    // Calculate pitch
    pitch -= yoffset * cameraSpeed;
    // Calculate Angle Arround
    //angleAroundTarget -= xoffset * cameraSpeed;
    yaw += xoffset * cameraSpeed;

    //std::cout << xoffset * cameraSpeed << " " << yoffset * cameraSpeed << "\n";
    //std::cout << yaw << " " << pitch << "\n";

    if (pitch > M_PI / 2)
        pitch = M_PI / 2 - 0.01;
    if(pitch < -M_PI / 2)
        pitch = -M_PI / 2 + 0.01;

    if (apuntar)
    {
        if (pitch > glm::radians(38.0f))
            pitch = glm::radians(38.0f) - 0.001f;
        else if (pitch < glm::radians(-38.0f))
            pitch = glm::radians(-38.0f) + 0.001f;
    }

    updateCamera();
}

void ThirdPersonCamera::scrollMoveCamera(float soffset, float dt){
    // Camera controls
    float cameraSpeed = sensitivity * dt;
    // Calculate zoom
    float zoomLevel = soffset * cameraSpeed;
    distanceFromTarget -= zoomLevel;

    updateCamera();
}

/*void ThirdPersonCamera::updateCamera() {
    //Calculate Horizontal distance
    float horizontalDistance = distanceFromTarget * cos(pitch);
    //Calculate Vertical distance
    float verticalDistance = distanceFromTarget * sin(pitch);

    //Calculate camera position
    //float theta = angleTarget + angleAroundTarget;
    float theta = yaw;
    float offsetx = horizontalDistance * sin(theta);
    float offsetz = horizontalDistance * cos(theta);
    position.x = cameraTarget.x - offsetx;
    position.z = cameraTarget.z - offsetz;
    position.y = cameraTarget.y + verticalDistance;

    //yaw = angleTarget - (180 + angleAroundTarget);

    if (distanceFromTarget < 0)
    	front = glm::normalize(position - cameraTarget);
    else
    	front = glm::normalize(cameraTarget - position);

    this->right = glm::normalize(glm::cross(this->front, this->worldUp));
    this->up = glm::normalize(glm::cross(this->right, this->front));
}*/

void ThirdPersonCamera::updateCamera() {
    
    front.x = std::cos(yaw) * std::cos(pitch);
    front.y = std::sin(pitch);
    front.z = std::sin(yaw) * std::cos(pitch);
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));

    if (!apuntar)
        position = cameraTarget - front * distanceFromTarget + glm::vec3(0.0f, height, 0.0f) + right * 0.9f;
    else
    {
        //float distance_up = height * std::cos(pitch);
        //float distance_front = height * std::sin(pitch);
        position = cameraTarget + right * 0.9f - front * 2.0f + /*front * distance_front +*/ up * height;

    }
        
    
}
