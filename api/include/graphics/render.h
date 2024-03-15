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
    /// Will free any resources currently in the gpu as part of this pool
    /// usually need to call UseLoadedResources for these resources to be loaded
    /// but if pool has gpu resources already, this will be called for you
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
    
    virtual void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix) {}
    void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
		   glm::vec4 overrideColour, Resource::Texture overrideTex) {
	model.colour = overrideColour;
	model.overrideTexture = overrideTex;
	DrawModel(model, modelMatrix, normalMatrix);
    }
    void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
		   Resource::Texture overrideTex) {
	DrawModel(model, modelMatrix, normalMatrix, glm::vec4(0), overrideTex);
    }
    void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
		   glm::vec4 overrideColour) {
	DrawModel(model, modelMatrix, normalMatrix, overrideColour,
		  Resource::Texture(Resource::NULL_ID));
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

    /// atomic bool is set to true when draw commands finish being sent
    /// to the gpu
    virtual void EndDraw(std::atomic<bool> &submit) = 0;
    void EndDraw() {
	std::atomic<bool> drawSubmitted;
	EndDraw(drawSubmitted);
    }
    
    /// --- Render State Config ---

    /// call in the window manager when the window is resized
    virtual void FramebufferResize() = 0;
    /// Shader Variable Setters
    virtual void set3DViewMat(glm::mat4 view, glm::vec4 camPos) = 0;
    void set3DViewMat(glm::mat4 view, glm::vec3 camPos) {
	set3DViewMat(view, glm::vec4(camPos, 0));
    }
    virtual void set2DViewMat(glm::mat4 view) = 0;
    virtual void set3DProjMat(glm::mat4 proj) = 0;
    virtual void set2DProjMat(glm::mat4 proj) = 0;
    virtual void setLightingProps(BPLighting lighting) = 0;

    virtual void setRenderConf(RenderConfig renderConf) = 0;
    virtual RenderConfig getRenderConf() = 0;
    virtual glm::vec2 offscreenSize() = 0;
};

#endif
