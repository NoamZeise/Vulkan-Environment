#include <resource_loader/vertex_model.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>
#include <cstring>

Resource::ModelType getModelType(Vertex2D vert) { return Resource::ModelType::m2D; }
Resource::ModelType getModelType(Vertex3D vert) { return Resource::ModelType::m3D; }
Resource::ModelType getModelType(VertexAnim3D vert) { return Resource::ModelType::m3D_Anim; }

//loadVerticies defined for Vertex Types

void loadVertices(Mesh<VertexAnim3D> *mesh, ModelInfo::Mesh &dataMesh) {
    for(int vert = 0; vert < dataMesh.verticies.size(); vert++) {
	VertexAnim3D vertex;
	vertex.Position = glm::vec4(dataMesh.verticies[vert].Position, 1.0f);
	vertex.Normal = dataMesh.verticies[vert].Normal;
	vertex.TexCoord = dataMesh.verticies[vert].TexCoord;
	for(int vecElem = 0; vecElem < 4; vecElem++) {
	    if(dataMesh.verticies[vert].BoneIDs.size() <= vecElem) {
		vertex.BoneIDs[vecElem] = -1;
		vertex.Weights[vecElem] = 0;
	    }
	    else {
		vertex.BoneIDs[vecElem] = dataMesh.verticies[vert].BoneIDs[vecElem];
		vertex.Weights[vecElem] = dataMesh.verticies[vert].BoneWeights[vecElem];
	    }
	}
	if(dataMesh.verticies[vert].BoneIDs.size() > 4)
	    std::cerr <<
		"vertex influenced by more than 4 bones, but only 4 bones will be used!\n";
	mesh->verticies.push_back(vertex);
    }
}

void loadVertices(Mesh<Vertex3D> *mesh, ModelInfo::Mesh &dataMesh) {
    glm::mat4 meshTransform = dataMesh.bindTransform;
    for(int vert = 0; vert < dataMesh.verticies.size(); vert++) {
	Vertex3D vertex;
	vertex.Position = meshTransform * glm::vec4(dataMesh.verticies[vert].Position, 1.0f);
	vertex.Normal = glm::mat3(glm::inverseTranspose(meshTransform)) *
	    dataMesh.verticies[vert].Normal;
	vertex.TexCoord = dataMesh.verticies[vert].TexCoord;
	mesh->verticies.push_back(vertex);
    }
}

void loadVertices(Mesh<Vertex2D> *mesh, ModelInfo::Mesh &dataMesh) {
    glm::mat4 meshTransform = dataMesh.bindTransform;
    for(int vert = 0; vert < dataMesh.verticies.size(); vert++) {
	Vertex2D vertex;
	vertex.Position = meshTransform * glm::vec4(dataMesh.verticies[vert].Position, 1.0f);
	vertex.TexCoord = dataMesh.verticies[vert].TexCoord;
	mesh->verticies.push_back(vertex);
    }
}

ModelInfo::Model makeQuadModel() {
    ModelInfo::Mesh mesh;
    mesh.verticies.resize(4);
    mesh.verticies[0].Position = {0.0f, 0.0f, 0.0f};
    mesh.verticies[0].TexCoord = {0.0f, 0.0f};
    mesh.verticies[1].Position = {1.0f, 0.0f, 0.0f};
    mesh.verticies[1].TexCoord = {1.0f, 0.0f};
    mesh.verticies[2].Position = {1.0f, 1.0f, 0.0f};
    mesh.verticies[2].TexCoord = {1.0f, 1.0f};
    mesh.verticies[3].Position = {0.0f, 1.0f, 0.0f};
    mesh.verticies[3].TexCoord = {0.0f, 1.0f};
    mesh.indices = { 0, 3, 2, 2, 1, 0};
    ModelInfo::Model quad;
    quad.meshes.push_back(mesh);
    return quad;
}
    
