#ifndef VERTEX_MODEL_H
#define VERTEX_MODEL_H

#include <graphics/resources.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <graphics/model/info.h>
#include "vertex_types.h"

Resource::ModelType getModelType(Vertex2D vert);
Resource::ModelType getModelType(Vertex3D vert);
Resource::ModelType getModelType(VertexAnim3D vert);

template <class T_Vert>
struct Mesh {
    Mesh() {}
    std::vector<T_Vert> verticies;
    std::vector<unsigned int> indices;
    Resource::Texture texture;
    glm::vec4 diffuseColour;

    std::string texToLoad = "";
    void processMeshInfo(ModelInfo::Mesh &dataMesh);
};

template <class T_Vert>
struct LoadedModel {
    LoadedModel(){}
    int ID = -1;
    std::vector<Mesh<T_Vert>*> meshes;
    std::vector<Resource::ModelAnimation> animations;
};

template <class T_Vert>
struct ModelGroup {
    ~ModelGroup() { clearData(); }
    std::vector<LoadedModel<T_Vert>> models;
    size_t vertexDataOffset;
    size_t vertexDataSize;
    void loadModel(ModelInfo::Model &modelData, uint32_t currentID);
    LoadedModel<T_Vert>* getPreviousModel() {
	if(models.size() == 0)
	    return nullptr;
	return &models[models.size() - 1];
    }
    void clearData() {
	for (auto& model : models)
	    for (size_t i = 0; i < model.meshes.size(); i++)
		delete model.meshes[i];
	models.clear();
    }
};

//loadVerticies defined for Vertex Types. Full definition in .cpp file.
void loadVertices(Mesh<VertexAnim3D> *mesh, ModelInfo::Mesh &dataMesh);
void loadVertices(Mesh<Vertex3D> *mesh, ModelInfo::Mesh &dataMesh);
void loadVertices(Mesh<Vertex2D> *mesh, ModelInfo::Mesh &dataMesh);


template <class T_Vert>
void ModelGroup<T_Vert>::loadModel(ModelInfo::Model &modelData,
				   uint32_t currentID) {
    this->models.push_back(LoadedModel<T_Vert>());
    auto model = &this->models[this->models.size() - 1];
    model->ID = currentID;
    for(auto& meshData: modelData.meshes) {
	model->meshes.push_back(new Mesh<T_Vert>());
	Mesh<T_Vert>* mesh = model->meshes.back();
	mesh->processMeshInfo(meshData);
    }
}

template <class T_Vert>
void Mesh<T_Vert>::processMeshInfo(ModelInfo::Mesh &dataMesh) {
    if(dataMesh.diffuseTextures.size() > 0)
	this->texToLoad = dataMesh.diffuseTextures[0];
    this->diffuseColour = dataMesh.diffuseColour;
    loadVertices(this, dataMesh);
    this->indices = dataMesh.indices;
}

ModelInfo::Model makeQuadModel();

#endif
