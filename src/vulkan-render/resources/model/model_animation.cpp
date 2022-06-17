#include "model_animation.h"
#include "glm/ext/quaternion_common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"

namespace Resource
{

ModelAnimation::ModelAnimation(std::vector<glm::mat4> bones, ModelInfo::Animation animation)
{
    this->bones = bones;
    this->animation = animation;
}

void ModelAnimation::Update(Timer &timer)
{
    currentTime = fmod(currentTime + (timer.FrameElapsed() * animation.ticks), animation.duration);
    processNode(animation.nodes[0], glm::mat4(1.0f));
}

void ModelAnimation::processNode(const ModelInfo::AnimNodes &animNode, glm::mat4 parentMat)
{
    glm::mat4 nodeMat;

   //std::cout << "boneNode x: " << parentMat[0][0] << std::endl;

    if(animNode.boneID != -1)
    {
        nodeMat = parentMat * boneTransform(animNode);
        bones[animNode.boneID] = nodeMat * animNode.boneOffset;
    }
    else
    {
        nodeMat = parentMat * animNode.modelNode.transform;
    }


    for(const auto& childID: animNode.modelNode.children)
        processNode(animation.nodes[childID], nodeMat);
}

glm::mat4 ModelAnimation::boneTransform(const ModelInfo::AnimNodes &animNode)
{
    return bonePos(animNode.positions) * boneRot(animNode.rotationsQ) * boneScl(animNode.scalings);
}


glm::mat4 ModelAnimation::bonePos(const std::vector<ModelInfo::AnimationKey::Position> &posFrames)
{
    if(posFrames.size() == 0)
        return glm::mat4(1.0f);
    if(posFrames.size() == 1)
        return glm::translate(glm::mat4(1.0f), posFrames[0].Pos);

    int second = 0;
    for(int i = 0 ; i < posFrames.size(); i++)
    {
        if(posFrames[i].time >= currentTime)
        {
            second = i;
            break;
        }
    }
    int first = (second - 1) % posFrames.size();

    //std::cout << "time: " << currentTime << std::endl;
    //std::cout << "frames: " << posFrames.size() << std::endl;
    //std::cout  << "first: " << first << "  second: " << second << std::endl;

    double factor = getFactor(posFrames[first].time, posFrames[second].time);
    return glm::translate(
        glm::mat4(1.0f),
        glm::mix(posFrames[first].Pos, posFrames[second].Pos, factor)
    );
}

glm::mat4 ModelAnimation::boneRot(const std::vector<ModelInfo::AnimationKey::RotationQ> &rotFrames)
{
    if(rotFrames.size() == 0)
        return glm::mat4(1.0f);
    if(rotFrames.size() == 1)
        return glm::toMat4(glm::normalize(rotFrames[0].Rot));
    int second = 0;
    for(int i = 0 ; i < rotFrames.size(); i++)
    {
        if(rotFrames[i].time >= currentTime)
        {
            second = i;
            break;
        }
    }
    int first = (second - 1) % rotFrames.size();

    float factor = static_cast<float>(getFactor(rotFrames[first].time, rotFrames[second].time));
    glm::quat rot = glm::slerp(rotFrames[first].Rot, rotFrames[second].Rot, factor);
    return glm::toMat4(glm::normalize(rot));
}

glm::mat4 ModelAnimation::boneScl(const std::vector<ModelInfo::AnimationKey::Scaling> &sclFrames)
{
    if(sclFrames.size() == 0)
        return glm::mat4(1.0f);
    if(sclFrames.size() == 1)
        return glm::scale(glm::mat4(1.0f), sclFrames[0].scale);

    int second = 0;
    for(int i = 0 ; i < sclFrames.size(); i++)
    {
        if(sclFrames[i].time >= currentTime)
        {
            second = i;
            break;
        }
    }
    int first = (second - 1) % sclFrames.size();
    double factor = getFactor(sclFrames[first].time, sclFrames[second].time);
    glm::vec3 scale = glm::mix(sclFrames[first].scale, sclFrames[second].scale, factor);
    return glm::scale(glm::mat4(1.0f), scale);
}

}
