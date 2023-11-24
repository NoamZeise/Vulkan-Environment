#ifndef OUTFACING_RENDER
#define OUTFACING_RENDER

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <atomic>
#include "render_config.h"
#include "shader_structs.h"
#include "resource_pool.h"

class Render {
 public:
    /// sets up graphics api, makes a default resource pool
    Render(GLFWwindow* window, RenderConfig conf) {}
    virtual ~Render() {};

    
    /// --- Resource Loading ---

    /// Create a new pool to load resource into.
    /// Pool will be in use by default.
    virtual ResourcePool* CreateResourcePool() = 0;
    /// also frees any resources held by the pool
    virtual void DestroyResourcePool(Resource::Pool pool) = 0;
    /// enable or disable using this resource pool's GPU loaded resources
    /// on by default
    /// will only take effect after a frame resource recreation
    /// (such as by calling UseLoadedResources() or if the framebuffer is resized)
    virtual void setResourcePoolInUse(Resource::Pool pool, bool usePool) = 0;
    /// return a created pool with the gievn id
    virtual ResourcePool* pool(Resource::Pool id) = 0;
    // return automatically created (default) pool
    ResourcePool* pool() {
	return pool(Resource::Pool(0));
    }
    /// load any staged resources from the given resource pool into the GPU
    /// need to call UseLoadedResources for these resources to be loaded
    virtual void LoadResourcesToGPU(Resource::Pool pool) = 0;
    /// Reload frame resources, using any resources that have been loaded
    /// to the GPU from resource pools, and that have useResourcePool
    /// set to true.
    void LoadResourcesToGPU(ResourcePool* pool) {
	return LoadResourcesToGPU(pool->id());
    }
    /// update render to reflect newly loaded resources
    /// destroyed pools or set resourcePoolInUse changes
    virtual void UseLoadedResources() = 0;


    /// --- Resource Drawing ---
    
    virtual void DrawModel(Resource::Model model, glm::mat4 modelMatrix,
			   glm::mat4 normalMatrix, glm::vec4 colour) {};
    void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix) {
	DrawModel(model, modelMatrix, normalMatrix, glm::vec4(0.0f));
    }
    virtual void DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix,
			       glm::mat4 normalMatrix,
			       Resource::ModelAnimation *animation) = 0;
    virtual void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix,
			  glm::vec4 colour, glm::vec4 texOffset) = 0;
    void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour) {
	DrawQuad(texture, modelMatrix, colour, glm::vec4(0, 0, 1, 1));
    }
    void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix) {
	DrawQuad(texture, modelMatrix, glm::vec4(1));
    }

    virtual void DrawString(Resource::Font font, std::string text, glm::vec2 position,
			    float size, float depth, glm::vec4 colour, float rotate) = 0;
    void DrawString(Resource::Font font, std::string text, glm::vec2 position,
		    float size, float depth, glm::vec4 colour) {
	DrawString(font, text, position, size, depth, colour, 0.0f);
    }

    virtual void EndDraw(std::atomic<bool> &submit) = 0;

    
    /// --- Render State Config ---

    /// call in the window manager when the window is resized
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
