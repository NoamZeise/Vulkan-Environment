#ifndef MODEL_ANIMATION_H
#define MODEL_ANIMATION_H

#include "model_info.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <string>

namespace Resource
{

class ModelAnimation
{
public:
    ModelAnimation() {}
    ModelAnimation(std::vector<glm::mat4> bones, ModelInfo::Animation animation);
    void Update(float frameElapsedMillis);
    std::vector<glm::mat4>* getCurrentBones() { return &bones; }
    std::string getName() { return animation.name; }

private:

    void processNode(const ModelInfo::AnimNodes &animNode, glm::mat4 parentMat);
    glm::mat4 boneTransform (const ModelInfo::AnimNodes &animNode);

    inline double getFactor(double t1, double t2)
    {
        return (currentTime - t1) / (t2 - t1);
    }
    glm::mat4 bonePos(const std::vector<ModelInfo::AnimationKey::Position> &posFrames);
    glm::mat4 boneRot(const std::vector<ModelInfo::AnimationKey::RotationQ> &rotFrames);
    glm::mat4 boneScl(const std::vector<ModelInfo::AnimationKey::Scaling> &sclFrames);


    std::vector<glm::mat4> bones;
    ModelInfo::Animation animation;
    double currentTime = 0;
};

}



#endif
