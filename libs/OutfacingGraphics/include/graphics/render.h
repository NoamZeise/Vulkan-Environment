#ifndef OUTFACING_RENDER
#define OUTFACING_RENDER

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <atomic>
#include "resources.h"
#include "render_config.h"
#include "shader_structs.h"

class Render {
 public:
    Render(GLFWwindow* window, RenderConfig conf) {}
    virtual ~Render() {};
    
    virtual Resource::Pool CreateResourcePool() = 0;
    virtual void DestroyResourcePool(Resource::Pool pool) = 0;
    virtual void setResourcePoolInUse(Resource::Pool pool, bool usePool) = 0;

    virtual Resource::Texture LoadTexture(std::string filepath) = 0;
    virtual Resource::Texture LoadTexture(Resource::Pool pool, std::string filepath) = 0;
    // Load 2D image data, takes ownership of data, 4 channels
    virtual Resource::Texture LoadTexture(unsigned char* data, int width, int height) = 0;
    virtual Resource::Texture LoadTexture(Resource::Pool pool, unsigned char* data,
					  int width, int height) = 0;
    virtual Resource::Font LoadFont(std::string filepath) = 0;
    
    virtual Resource::Model LoadModel(Resource::ModelType type, std::string filepath,
				      std::vector<Resource::ModelAnimation> *pAnimations) = 0;
    virtual Resource::Model LoadModel(Resource::Pool pool, Resource::ModelType type,
				      std::string filepath,
				      std::vector<Resource::ModelAnimation> *pAnimations) = 0;
    virtual Resource::Model LoadModel(Resource::ModelType type, ModelInfo::Model& model,
				      std::vector<Resource::ModelAnimation> *pAnimations) = 0;
    virtual Resource::Model LoadModel(Resource::Pool pool, Resource::ModelType type,
				      ModelInfo::Model& model,
				      std::vector<Resource::ModelAnimation> *pAnimations) = 0;
    virtual Resource::Model Load3DModel(std::string filepath) = 0;
    virtual Resource::Model Load3DModel(ModelInfo::Model& model) = 0;
    virtual Resource::Model LoadAnimatedModel(
	    std::string filepath, std::vector<Resource::ModelAnimation> *pGetAnimations) = 0;
	
    virtual void LoadResourcesToGPU() = 0;
    virtual void LoadResourcesToGPU(Resource::Pool pool) = 0;
    virtual void UseLoadedResources() = 0;
    
    virtual void DrawModel(Resource::Model model, glm::mat4 modelMatrix,
			   glm::mat4 normalMatrix) = 0;
    virtual void DrawModel(Resource::Model model, glm::mat4 modelMatrix,
			   glm::mat4 normalMatrix, glm::vec4 colour) = 0;
    virtual void DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix,
			       glm::mat4 normalMatrix,
			       Resource::ModelAnimation *animation) = 0;
    virtual void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix,
			  glm::vec4 colour, glm::vec4 texOffset) = 0;
    virtual void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix,
			  glm::vec4 colour) = 0;
    virtual void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix) = 0;

    virtual void DrawString(Resource::Font font, std::string text, glm::vec2 position,
			    float size, float depth, glm::vec4 colour, float rotate) = 0;
    virtual void DrawString(Resource::Font font, std::string text, glm::vec2 position,
			    float size, float depth, glm::vec4 colour) = 0;
    virtual float MeasureString(Resource::Font font, std::string text, float size) = 0;

    virtual void EndDraw(std::atomic<bool> &submit) = 0;
    
    virtual void FramebufferResize() = 0;
    
    virtual void set3DViewMatrixAndFov(glm::mat4 view, float fov, glm::vec4 camPos) = 0;
    virtual void set2DViewMatrixAndScale(glm::mat4 view, float scale) = 0;
    virtual void setLightingProps(BPLighting lighting) = 0;

    virtual void setRenderConf(RenderConfig renderConf) = 0;
    virtual RenderConfig getRenderConf() = 0;
    virtual void setTargetResolution(glm::vec2 resolution) = 0;
    virtual glm::vec2 getTargetResolution() = 0;
};

#endif
