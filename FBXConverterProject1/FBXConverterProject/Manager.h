#pragma once
#include <fbxsdk.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <crtdbg.h>
#include "FormatHeader.h"
//#include "ExportHelpers.h"

namespace FBXExport
{
	class DecomposedTransform;
	class Skeleton;
}

class Converter
{
public:
	Converter(const char* fileName);
	~Converter();
	void convertFileToCustomFormat();


	void convertFileToCustomFormatStefan();
private:
	FbxManager * sdk_Manager;
	FbxIOSettings* sdk_IOSettings;
	FbxImporter* sdk_importer;
	FbxScene* sdk_scene;
	FbxNode* scene_rootNode;

	/// ----------------
	std::vector<FbxMatrix> getInverseBindPoseTransforms(FbxNode* skeletonRoot);
	/// ----------------

	bool createMeshFiles(int nrOfVertices, std::vector<FbxVector4>Positions, std::vector<FbxVector4>normals, std::vector<FbxVector2>UV, const char* node_name);
	void getNames(FbxNode* scene_node);
	void getSceneMeshes(FbxNode* scene_node, bool useStefan = false);
	
	bool createAnimatedMeshFile(FbxNode * scene_node, int nrOfVertices, std::vector<FbxVector4> Positions, std::vector<FbxVector4> normals, std::vector<FbxVector2> UV, const char * node_name, std::vector<int> weight_indices_vector);
	bool createAnimatedMeshFileStefan(FbxNode * scene_node, int nrOfVertices, std::vector<FbxVector4> Positions, std::vector<FbxVector4> normals, std::vector<FbxVector2> UV, const char * node_name, std::vector<int> weight_indices_vector);

	bool getSceneAnimationData(FbxNode* scene_node, std::vector<std::vector<Transform>> &transform_vector);
	bool getSceneAnimationDataStefan(FbxNode * scene_node, std::vector<std::vector<FBXExport::DecomposedTransform>> &transform_vector);
	
	bool createSkeletonFile(std::vector<std::string> names, std::vector<FbxAMatrix> joint_matrices, int nrOfJoints, std::vector<FbxNode*> parent_name_list);
	bool createSkeletonFileStefan(const FBXExport::Skeleton& skeleton);
	
	void storeChildren(std::vector<std::string> *vector, FbxNode* scene_node);

	void createAnimationFile(std::vector<std::vector<Transform>> keys);
	void createAnimationFileStefan(std::vector<std::vector<FBXExport::DecomposedTransform>> keys);

	


	std::string getFileName(const char* fullPath)
	{
		std::string temp = fullPath;

		const size_t lastSlashIndex = temp.find_last_of("/\\");
		std::string asd= temp.substr(lastSlashIndex + 1);
		return  temp.substr(lastSlashIndex + 1);
	}

	

};