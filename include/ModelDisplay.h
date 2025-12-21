#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "context.h"

inline void EnableWall(Context& ctx, bool isEnabled, int backwall, int leftwall, int rightwall, float time) {
    
    for (auto& obj : ctx.objects) {
        if (obj->modelIndex == backwall || 
            obj->modelIndex == leftwall || 
            obj->modelIndex == rightwall) {
            
            bool hidewall = false;
            if (isEnabled) {
                if(time < 11.0f && obj->modelIndex == rightwall) hidewall = true;
                else if((time > 13.0f && obj->modelIndex == leftwall)) hidewall = true;
                
                if (hidewall) {
                    obj->transformMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f));
                } else {
                    obj->transformMatrix = glm::mat4(1.0f); 
                }
            } 
            else {
                obj->transformMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f));
            }
        }
    }
}