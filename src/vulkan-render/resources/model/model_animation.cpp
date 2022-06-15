#include "model_animation.h"
#include "glm/ext/quaternion_common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"

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

    if(animNode.boneID != -1)
    {
        nodeMat = parentMat * boneTransform(animNode);
        bones[animNode.boneID] = nodeMat * animNode.boneOffset;
    }
    else
    {
        nodeMat = parentMat * animNode.modelNode.transform;
    }

    nodeMat = parentMat * nodeMat;

    for(const auto& childID: animNode.modelNode.children)
        processNode(animation.nodes[childID], nodeMat);
}

glm::mat4 ModelAnimation::boneTransform(const ModelInfo::AnimNodes &animNode)
{
    return bonePos(animNode.positions) * boneRot(animNode.rotationsQ) * boneScl(animNode.scalings);
}


glm::mat4 ModelAnimation::bonePos(const std::vector<ModelInfo::AnimationKey::Position> &posFrames)
{
    if(posFrames.size() == 1)
        return glm::translate(glm::mat4(1.0f), posFrames[0].Pos);

    int second = 0;
    for(int i = 0 ; i < posFrames.size(); i++)
    {
        if(posFrames[i].time > currentTime)
            second = i;
    }
    int first = (second - 1) % posFrames.size();

    float factor = getFactor(posFrames[first].time, posFrames[second].time);
    return glm::translate(
        glm::mat4(1.0f),
        glm::mix(posFrames[first].Pos, posFrames[second].Pos, factor)
    );
}

glm::mat4 ModelAnimation::boneRot(const std::vector<ModelInfo::AnimationKey::RotationQ> &rotFrames)
{
    int second = 0;
    for(int i = 0 ; i < rotFrames.size(); i++)
    {
        if(rotFrames[i].time > currentTime)
            second = i;
    }
    int first = (second - 1) % rotFrames.size();

    float factor = getFactor(rotFrames[first].time, rotFrames[second].time);
    glm::quat rot = glm::slerp(rotFrames[first].Rot, rotFrames[second].Rot, factor);
    return glm::toMat4(glm::normalize(rot));
}

glm::mat4 ModelAnimation::boneScl(const std::vector<ModelInfo::AnimationKey::Scaling> &sclFrames)
{
    int second = 0;
    for(int i = 0 ; i < sclFrames.size(); i++)
    {
        if(sclFrames[i].time > currentTime)
            second = i;
    }
    int first = (second - 1) % sclFrames.size();
    float factor = getFactor(sclFrames[first].time, sclFrames[second].time);
    glm::vec3 scale = glm::mix(sclFrames[first].scale, sclFrames[second].scale, factor);
    return glm::scale(glm::mat4(1.0f), scale);
}
