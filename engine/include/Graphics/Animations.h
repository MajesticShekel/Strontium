#pragma once

// Maximum bones which can influence a vertex.
#define MAX_BONES_PER_VERTEX 4
// Maximum bones in a single model file. Using SSBOs to pass them to the skinning shaders.
#define MAX_BONES_PER_MODEL 512

// Macro include file.
#include "StrontiumPCH.h"

#include "Assets/AssetManager.h"

// Forward declare Assimp garbage.
struct aiAnimation;

namespace Strontium
{
  // Forward declare various classes.
  class Model;

  struct VertexBone
  {
    std::string name;
    std::string parentMesh;

    glm::mat4 offsetMatrix;

    VertexBone(const std::string &name, const std::string &parentMesh,
               const glm::mat4 &offsetMatrix)
      : name(name)
      , parentMesh(parentMesh)
      , offsetMatrix(offsetMatrix)
    { }
  };

  struct AnimationNode
  {
    std::string name;

    std::vector<std::pair<float, glm::vec3>> keyTranslations;
    std::vector<std::pair<float, glm::quat>> keyRotations;
    std::vector<std::pair<float, glm::vec3>> keyScales;

    AnimationNode(const std::string &name)
      : name(name)
    { }

    AnimationNode() = default;
  };

  struct SceneNode
  {
    std::string name;
    std::vector<std::string> childNames;;

    glm::mat4 localTransform;

    SceneNode(const std::string &name, const glm::mat4 &localTransform)
      : name(name)
      , localTransform(localTransform)
    { }

    SceneNode() = default;
  };

  class Animation
  {
  public:
    Animation(const aiAnimation* animation, Model* parentModel);
    Animation(Model* parentModel);
    ~Animation();

    void loadAnimation(const aiAnimation* animation);

    void computeBoneTransforms(float aniTime, std::vector<glm::mat4> &outBones);

    float getDuration() const { return this->duration; }
    float getTPS() const { return this->ticksPerSecond; }
    std::string getName() const { return this->name; }
    std::unordered_map<std::string, AnimationNode>& getAniNodes() { return this->animationNodes; }
  private:
    glm::mat4 interpolateTranslation(float aniTime, const AnimationNode &node);
    glm::mat4 interpolateRotation(float aniTime, const AnimationNode &node);
    glm::mat4 interpolateScale(float aniTime, const AnimationNode &node);

    void readNodeHierarchy(float aniTime, const SceneNode &node,
                           const glm::mat4 parentTransform,
                           std::vector<glm::mat4> &outBones);
    Model* parentModel;

    std::unordered_map<std::string, AnimationNode> animationNodes;

    std::string name;
    float duration;
    float ticksPerSecond;
  };

  class Animator
  {
  public:
    Animator();
    ~Animator() = default;

    void setAnimation(Animation* animation, const AssetHandle &modelHandle);

    void onUpdate(float dt);

    void startAnimation() { this->animating = true; this->paused = false; }
    void pauseAnimation() { this->paused = true; }
    void resumeAnimation() { this->paused = false; }
    void stopAnimation() { this->animating = false; this->currentAniTime = 0.0f; this->paused = true; }

    std::vector<glm::mat4>& getFinalBoneTransforms() { return this->finalBoneTransforms; }
    Animation* getStoredAnimation() { return this->storedAnimation; }
    float& getAnimationTime() { return this->currentAniTime; }
    bool isAnimating() { return this->animating; }
    bool isPaused() { return this->paused; }
    bool animationRenderable() { return this->storedAnimation != nullptr && this->animating; }
  private:
    float currentAniTime;
    AssetHandle storedModel;
    Animation* storedAnimation;
    std::vector<glm::mat4> finalBoneTransforms;

    bool animating;
    bool paused;
  };
}
