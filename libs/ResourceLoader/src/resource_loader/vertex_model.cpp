#include "vertex_model.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>

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
