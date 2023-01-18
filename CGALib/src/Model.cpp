/*
 * Model.cpp
 *
 *  Created on: 13/09/2016
 *      Author: rey
 */

#include "Headers/Model.h"
#include "Headers/TimeManager.h"
#include "Headers/mathUtil.h"

#include <iostream>

 //glfw include
#include <GLFW/glfw3.h>

Model::Model() {
	this->aabb.mins.x = FLT_MAX;
	this->aabb.mins.y = FLT_MAX;
	this->aabb.mins.z = FLT_MAX;
	this->aabb.maxs.x = -FLT_MAX;
	this->aabb.maxs.y = -FLT_MAX;
	this->aabb.maxs.z = -FLT_MAX;
	this->animationIndex = 0;
}

Model::~Model() {
	for (GLuint i = 0; i < this->meshes.size(); i++){
		delete this->meshes[i]->bones;
		delete this->meshes[i];
	}
	for (int i = 0; i < this->textureLoaded.size(); i++)
		delete this->textureLoaded[i];
}

float Model::render(glm::mat4 parentTrans, float timeReset) {

	float runningTime = TimeManager::Instance().GetRunningTime();
	//float runningTime = TimeManager::Instance().DeltaTime;

	if (timeReset > -0.5f)
	{
		runningTime = runningTime - timeReset;
		if (runningTime < 0.0f)
			runningTime = 0.0001f;
	}
		


	float animationPercentage = -1.0f;

	for (GLuint i = 0; i < this->meshes.size(); i++) {
		this->meshes[i]->setShader(this->getShader());
		this->meshes[i]->setPosition(this->getPosition());
		this->meshes[i]->setScale(this->getScale());
		this->meshes[i]->setOrientation(this->getOrientation());
		if(scene->mNumAnimations > 0){
			this->meshes[i]->bones->setAnimationIndex(this->animationIndex);
			if(this->meshes[i]->bones != nullptr){
				shader_ptr->setInt("numBones", this->meshes[i]->bones->getNumBones());
				std::vector<glm::mat4> transforms;
				animationPercentage = this->meshes[i]->bones->bonesTransform(runningTime, transforms, scene);
				for (unsigned int j = 0; j < transforms.size(); j++) {
					std::stringstream ss;
					ss << "bones[" << j << "]";
					shader_ptr->setMatrix4(ss.str(), 1, GL_FALSE,
							glm::value_ptr(m_GlobalInverseTransform * transforms[j]));
				}
				//std::cout << transforms.size() << "\n";
			}
			else
				parentTrans = m_GlobalInverseTransform * parentTrans;
		}
		this->meshes[i]->render(parentTrans);
		glActiveTexture(GL_TEXTURE0);
		shader_ptr->setInt("numBones", 0);
	}

	return animationPercentage;

}

std::vector<glm::vec4> Model::render(glm::mat4 parentTrans, unsigned vbo, unsigned mesh, std::vector<unsigned> vertexesToRetrieve, float timeReset) {

	float runningTime = TimeManager::Instance().GetRunningTime();
	//float runningTime = TimeManager::Instance().DeltaTime;

	if (timeReset > -0.5f)
	{
		runningTime = runningTime - timeReset;
		if (runningTime < 0.0f)
			runningTime = 0.0001f;
	}

	shader_ptr->setInt("recording", 1);

	std::vector<glm::vec4> vertexesWorldSpace(meshes[mesh]->getVertexArray().size());

	//for (GLuint i = 0; i < this->meshes.size(); i++) {
	for (GLuint i = mesh; i < mesh + 1; i++) {
		this->meshes[i]->setShader(this->getShader());
		this->meshes[i]->setPosition(this->getPosition());
		this->meshes[i]->setScale(this->getScale());
		this->meshes[i]->setOrientation(this->getOrientation());
		if (scene->mNumAnimations > 0) {
			this->meshes[i]->bones->setAnimationIndex(this->animationIndex);
			if (this->meshes[i]->bones != nullptr) {
				shader_ptr->setInt("numBones", this->meshes[i]->bones->getNumBones());
				std::vector<glm::mat4> transforms;
				this->meshes[i]->bones->bonesTransform(runningTime, transforms, scene);
				for (unsigned int j = 0; j < transforms.size(); j++) {
					std::stringstream ss;
					ss << "bones[" << j << "]";
					shader_ptr->setMatrix4(ss.str(), 1, GL_FALSE,
						glm::value_ptr(m_GlobalInverseTransform * transforms[j]));
				}
				//std::cout << transforms.size() << "\n";
			}
			else
				parentTrans = m_GlobalInverseTransform * parentTrans;
		}

		shader_ptr->turnOn();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, vbo);

		this->meshes[i]->render(parentTrans);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		//std::cout << meshes[i]->getVertexArray().size() << std::endl;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vertexesWorldSpace.size() * sizeof(glm::vec4), &vertexesWorldSpace[0]);

		glActiveTexture(GL_TEXTURE0);
		shader_ptr->setInt("numBones", 0);
	}

	shader_ptr->setInt("recording", 0);

	if (vertexesToRetrieve.empty())
		return vertexesWorldSpace;

	std::vector<glm::vec4> requestedVertexes(vertexesToRetrieve.size());
	for (int i = 0; i < vertexesToRetrieve.size(); ++i)
	{
		requestedVertexes[i] = vertexesWorldSpace[vertexesToRetrieve[i]];
	}

	return requestedVertexes;
}

void Model::loadModel(const std::string & path) {
	std::cout << path << ": reading";
	// Lee el archivo via ASSIMP
	scene = importer.ReadFile(path.c_str(),
			aiProcess_Triangulate | aiProcess_FlipUVs
					| aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	// Revisa errores
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE
			|| !scene->mRootNode) // if is Not Zero
			{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString()
				<< std::endl;
		return;
	}

	std::cout << ".";

	CopyMat(scene->mRootNode->mTransformation, this->m_GlobalInverseTransform);
	this->m_GlobalInverseTransform = glm::inverse(
			this->m_GlobalInverseTransform);

	// Recupera el path del directorio del archivo.
	this->directory = path.substr(0, path.find_last_of('/'));

	std::cout << ".";

	// Se procesa el nodo raiz recursivamente.
	this->processNode(scene->mRootNode, scene);

	std::cout << ".";

	// Se crea la SBB
	this->sbb.c = glm::vec3((this->aabb.mins.x + this->aabb.maxs.x) / 2.0f,
			(this->aabb.mins.y + this->aabb.maxs.y) / 2.0f,
			(this->aabb.mins.z + this->aabb.maxs.z) / 2.0f);
	/*this->sbb.ratio = sqrt(
			pow(this->aabb.mins.x - this->aabb.maxs.x, 2)
					+ pow(this->aabb.mins.y - this->aabb.maxs.y, 2)
					+ pow(this->aabb.mins.z - this->aabb.maxs.z, 2)) / 2.0f;*/
	float distx = std::abs(this->aabb.maxs.x - this->aabb.mins.x);
	float disty = std::abs(this->aabb.maxs.y - this->aabb.mins.y);
	float distz = std::abs(this->aabb.maxs.z - this->aabb.mins.z);
	float ratio = glm::max(distx, glm::max(disty, distz));
	this->sbb.ratio = ratio / 2.0;


	// Se crea la obb
	this->obb.c = this->sbb.c;
	/*this->obb.e.x = aabb.maxs.x - aabb.mins.x;
	this->obb.e.y = aabb.maxs.y - aabb.mins.y;
	this->obb.e.z = aabb.maxs.z - aabb.mins.z;*/
	this->obb.e = (aabb.maxs - aabb.mins) / 2.0f;
	this->obb.u = glm::quat(0.0, 0.0, 0.0, 1);

	std::cout << "done\n";
}

void Model::loadSceneParallel(const std::string& path) {

	std::cout << path << ": reading";
	// Lee el archivo via ASSIMP
	scene = importer.ReadFile(path.c_str(),
		aiProcess_Triangulate | aiProcess_FlipUVs
		| aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	// Revisa errores
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE
		|| !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString()
			<< std::endl;
		return;
	}

	std::cout << ".";

	CopyMat(scene->mRootNode->mTransformation, this->m_GlobalInverseTransform);
	this->m_GlobalInverseTransform = glm::inverse(
		this->m_GlobalInverseTransform);

	// Recupera el path del directorio del archivo.
	this->directory = path.substr(0, path.find_last_of('/'));

	std::cout << ".";

}

void Model::loadMeshesSequential() {

	this->processNode(scene->mRootNode, scene);

	std::cout << ".";

	// Se crea la SBB
	this->sbb.c = glm::vec3((this->aabb.mins.x + this->aabb.maxs.x) / 2.0f,
		(this->aabb.mins.y + this->aabb.maxs.y) / 2.0f,
		(this->aabb.mins.z + this->aabb.maxs.z) / 2.0f);
	/*this->sbb.ratio = sqrt(
			pow(this->aabb.mins.x - this->aabb.maxs.x, 2)
					+ pow(this->aabb.mins.y - this->aabb.maxs.y, 2)
					+ pow(this->aabb.mins.z - this->aabb.maxs.z, 2)) / 2.0f;*/
	float distx = std::abs(this->aabb.maxs.x - this->aabb.mins.x);
	float disty = std::abs(this->aabb.maxs.y - this->aabb.mins.y);
	float distz = std::abs(this->aabb.maxs.z - this->aabb.mins.z);
	float ratio = glm::max(distx, glm::max(disty, distz));
	this->sbb.ratio = ratio / 2.0;


	// Se crea la obb
	this->obb.c = this->sbb.c;
	/*this->obb.e.x = aabb.maxs.x - aabb.mins.x;
	this->obb.e.y = aabb.maxs.y - aabb.mins.y;
	this->obb.e.z = aabb.maxs.z - aabb.mins.z;*/
	this->obb.e = (aabb.maxs - aabb.mins) / 2.0f;
	this->obb.u = glm::quat(0.0, 0.0, 0.0, 1);

	std::cout << "done\n";

}

//void Model::loadModel(const std::string& path, Model &baseMesh) {
//	std::cout << "reading";
//	// Lee el archivo via ASSIMP
//	scene = importer.ReadFile(path.c_str(),
//		aiProcess_Triangulate | aiProcess_FlipUVs
//		| aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
//	// Revisa errores
//	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE
//		|| !scene->mRootNode) // if is Not Zero
//	{
//		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString()
//			<< std::endl;
//		return;
//	}
//
//	std::cout << ".";
//
//	CopyMat(scene->mRootNode->mTransformation, this->m_GlobalInverseTransform);
//	this->m_GlobalInverseTransform = glm::inverse(
//		this->m_GlobalInverseTransform);
//
//	// Recupera el path del directorio del archivo.
//	this->directory = path.substr(0, path.find_last_of('/'));
//
//	std::cout << ".";
//
//	// Se procesa el nodo raiz recursivamente.
//	this->processNode(scene->mRootNode, scene, baseMesh);
//
//	std::cout << ".";
//
//	// Se crea la SBB
//	this->sbb.c = glm::vec3((this->aabb.mins.x + this->aabb.maxs.x) / 2.0f,
//		(this->aabb.mins.y + this->aabb.maxs.y) / 2.0f,
//		(this->aabb.mins.z + this->aabb.maxs.z) / 2.0f);
//	/*this->sbb.ratio = sqrt(
//			pow(this->aabb.mins.x - this->aabb.maxs.x, 2)
//					+ pow(this->aabb.mins.y - this->aabb.maxs.y, 2)
//					+ pow(this->aabb.mins.z - this->aabb.maxs.z, 2)) / 2.0f;*/
//	float distx = std::abs(this->aabb.maxs.x - this->aabb.mins.x);
//	float disty = std::abs(this->aabb.maxs.y - this->aabb.mins.y);
//	float distz = std::abs(this->aabb.maxs.z - this->aabb.mins.z);
//	float ratio = glm::max(distx, glm::max(disty, distz));
//	this->sbb.ratio = ratio / 2.0;
//
//
//	// Se crea la obb
//	this->obb.c = this->sbb.c;
//	/*this->obb.e.x = aabb.maxs.x - aabb.mins.x;
//	this->obb.e.y = aabb.maxs.y - aabb.mins.y;
//	this->obb.e.z = aabb.maxs.z - aabb.mins.z;*/
//	this->obb.e = (aabb.maxs - aabb.mins) / 2.0f;
//	this->obb.u = glm::quat(0.0, 0.0, 0.0, 1);
//
//	std::cout << "done";
//}

void Model::processNode(aiNode* node, const aiScene* scene) {
	// Procesa cada maya del nodo actual
	
	for (GLuint i = 0; i < node->mNumMeshes; i++) {
		//std::cout << " Processing mesh ->";
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		Mesh * meshModel = this->processMesh(mesh, scene);
		//std::cout << " Mesh processed ->";
		Bones * bones = new Bones(meshModel->getVAO(), mesh->mNumVertices);
		//std::cout << " Bones created ->";
		bones->loadBones(i, mesh);
		//std::cout << " Bones loaded ->";
		meshModel->bones = bones;
		this->meshes.push_back(meshModel);
	}
	for (GLuint i = 0; i < node->mNumChildren; i++) {
		this->processNode(node->mChildren[i], scene);
	}
}

//void Model::processNode(aiNode* node, const aiScene* scene, Model& baseMesh) {
//	// Procesa cada maya del nodo actual
//	for (GLuint i = 0; i < node->mNumMeshes; i++) {
//		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//		Mesh* meshModel = this->processMesh(mesh, scene, baseMesh.meshes[i]->textures);
//		Bones* bones = new Bones(meshModel->getVAO(), mesh->mNumVertices);
//		bones->loadBones(i, mesh);
//		meshModel->bones = bones;
//		this->meshes.push_back(meshModel);
//	}
//	for (GLuint i = 0; i < node->mNumChildren; i++) {
//		this->processNode(node->mChildren[i], scene, baseMesh);
//	}
//}

Mesh * Model::processMesh(aiMesh* mesh, const aiScene* scene) {
	std::vector<AbstractModel::Vertex> vertices;
	std::vector<GLuint> indices;
	std::vector<Texture*> textures;
	//std::cout << "(recorrer vertices->";
	// Recorre los vertices de cada maya
	for (GLuint i = 0; i < mesh->mNumVertices; i++) {
		AbstractModel::Vertex vertex;
		glm::vec3 vector;
		// Compute the AABB
		if (mesh->mVertices[i].x < aabb.mins.x)
			aabb.mins.x = mesh->mVertices[i].x;
		if (mesh->mVertices[i].x > aabb.maxs.x)
			aabb.maxs.x = mesh->mVertices[i].x;
		if (mesh->mVertices[i].y < aabb.mins.y)
			aabb.mins.y = mesh->mVertices[i].y;
		if (mesh->mVertices[i].y > aabb.maxs.y)
			aabb.maxs.y = mesh->mVertices[i].y;
		if (mesh->mVertices[i].z < aabb.mins.z)
			aabb.mins.z = mesh->mVertices[i].z;
		if (mesh->mVertices[i].z > aabb.maxs.z)
			aabb.maxs.z = mesh->mVertices[i].z;

		// Positions
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.m_pos = vector;
		// Normals
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.m_normal = vector;
		// Texture Coordinates
		if (mesh->mTextureCoords[0]) // Esto se ejecuta si la maya contiene texturas.
		{
			glm::vec2 vec;
			// Un vertice puede contener hasta 8 diferentes coordenadas de textura, unicamente se considera 
			// que los modelos tiene una coordenada de textura por vertice, y corresponde a la primera en el arreglo.
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.m_tex = vec;
		} else
			vertex.m_tex = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}
	//std::cout << "recorrer caras->";
	// Se recorre cada cara de la maya (una cara es un triangulo en la maya) y recupera su correspondiente indice del vertice.
	for (GLuint i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		// Recupera todos los indices de la cara y los almacena en el vector de indices
		for (GLuint j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	//std::cout << "procesar materiales->";
	// Process materials
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// Diffuse: texture_diffuseN
		// Specular: texture_specularN
		// Normal: texture_normalN

		// 1. Diffuse maps
		std::vector<Texture*> diffuseMaps = this->loadMaterialTextures(material,
				aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. Specular maps
		std::vector<Texture*> specularMaps = this->loadMaterialTextures(material,
				aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(),
				specularMaps.end());
		// 3. Normal maps
		std::vector<Texture*> normalMaps = this->loadMaterialTextures(material,
				aiTextureType_NORMALS, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		// 4. Height maps
		std::vector<Texture*> heightMaps = this->loadMaterialTextures(material,
				aiTextureType_HEIGHT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
	}
	//std::cout << "crear mesh->";
	Mesh * meshModel = new Mesh(vertices, indices, textures);
	// Regresa la maya de un objeto creado de los datos extraidos.
	//std::cout << "listo)";
	return meshModel;
}

//Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene, std::vector<Texture*> &textures) {
//	std::vector<AbstractModel::Vertex> vertices;
//	std::vector<GLuint> indices;
//	//std::cout << "Hola\n";
//	// Recorre los vertices de cada maya
//	for (GLuint i = 0; i < mesh->mNumVertices; i++) {
//		AbstractModel::Vertex vertex;
//		glm::vec3 vector;
//		// Compute the AABB
//		if (mesh->mVertices[i].x < aabb.mins.x)
//			aabb.mins.x = mesh->mVertices[i].x;
//		if (mesh->mVertices[i].x > aabb.maxs.x)
//			aabb.maxs.x = mesh->mVertices[i].x;
//		if (mesh->mVertices[i].y < aabb.mins.y)
//			aabb.mins.y = mesh->mVertices[i].y;
//		if (mesh->mVertices[i].y > aabb.maxs.y)
//			aabb.maxs.y = mesh->mVertices[i].y;
//		if (mesh->mVertices[i].z < aabb.mins.z)
//			aabb.mins.z = mesh->mVertices[i].z;
//		if (mesh->mVertices[i].z > aabb.maxs.z)
//			aabb.maxs.z = mesh->mVertices[i].z;
//
//		// Positions
//		vector.x = mesh->mVertices[i].x;
//		vector.y = mesh->mVertices[i].y;
//		vector.z = mesh->mVertices[i].z;
//		vertex.m_pos = vector;
//		// Normals
//		vector.x = mesh->mNormals[i].x;
//		vector.y = mesh->mNormals[i].y;
//		vector.z = mesh->mNormals[i].z;
//		vertex.m_normal = vector;
//		// Texture Coordinates
//		if (mesh->mTextureCoords[0]) // Esto se ejecuta si la maya contiene texturas.
//		{
//			glm::vec2 vec;
//			// Un vertice puede contener hasta 8 diferentes coordenadas de textura, unicamente se considera 
//			// que los modelos tiene una coordenada de textura por vertice, y corresponde a la primera en el arreglo.
//			vec.x = mesh->mTextureCoords[0][i].x;
//			vec.y = mesh->mTextureCoords[0][i].y;
//			vertex.m_tex = vec;
//		}
//		else
//			vertex.m_tex = glm::vec2(0.0f, 0.0f);
//
//		vertices.push_back(vertex);
//	}
//	// Se recorre cada cara de la maya (una cara es un triangulo en la maya) y recupera su correspondiente indice del vertice.
//	for (GLuint i = 0; i < mesh->mNumFaces; i++) {
//		aiFace face = mesh->mFaces[i];
//		// Recupera todos los indices de la cara y los almacena en el vector de indices
//		for (GLuint j = 0; j < face.mNumIndices; j++)
//			indices.push_back(face.mIndices[j]);
//	}
//
//	Mesh* meshModel = new Mesh(vertices, indices, textures);
//	// Regresa la maya de un objeto creado de los datos extraidos.
//	return meshModel;
//}

std::vector<Texture*> Model::loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, std::string typeName) {
	std::vector<Texture*> textures;
	for (GLuint i = 0; i < mat->GetTextureCount(type); i++) {
		aiString str;
		std::cout << "(get texture->";
		mat->GetTexture(type, i, &str);
		// Verifica si la textura fue cargada antes y si es as�, continua con la siguiente iteracion: en caso controaio se salta la carga
		GLboolean skip = false;
		for (GLuint j = 0; j < textureLoaded.size(); j++) {
			if (textureLoaded[j]->getFileName() == str.C_Str()) {
				textures.push_back(textureLoaded[j]);
				skip = true;
				break;
			}
		}
		std::cout << "verified done->";
		if (!skip) {
			std::string filename = std::string(str.C_Str());
			filename = this->directory + '/' + filename;
			Texture * texture = new Texture(GL_TEXTURE_2D, filename);
			texture->load();
			texture->setType(typeName);
			textures.push_back(texture);
			this->textureLoaded.push_back(texture); // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
		std::cout << "skip done)";
	}
	return textures;
}

bool Model::rayPicking(glm::vec3 init, glm::vec3 end, glm::vec3 &intersection) {
	return false;
}
