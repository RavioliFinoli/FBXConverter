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

	void convertFileToCustomFormatStefan();
private:
	FbxManager * sdk_Manager;
	FbxIOSettings* sdk_IOSettings;
	FbxImporter* sdk_importer;
	FbxScene* sdk_scene;
	FbxNode* scene_rootNode;

	void getSceneMeshes(FbxNode* scene_node);
	bool getSceneAnimationData(FbxNode* scene_node, std::vector<std::vector<FBXExport::DecomposedTransform>> &transform_vector);
	bool createMeshFiles(int nrOfVertices, std::vector<FbxVector4>Positions, std::vector<FbxVector4>normals, std::vector<FbxVector2>UV, const char* node_name);
	bool createAnimatedMeshFile(FbxNode * scene_node, int nrOfVertices, std::vector<FbxVector4> Positions, std::vector<FbxVector4> normals, std::vector<FbxVector2> UV, const char * node_name, std::vector<int> weight_indices_vector);
	bool createSkeletonFile(const FBXExport::Skeleton& skeleton);
	void createAnimationFile(std::vector<std::vector<FBXExport::DecomposedTransform>> keys);
};