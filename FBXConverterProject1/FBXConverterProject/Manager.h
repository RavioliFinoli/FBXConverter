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
	struct DecomposedTransform;
	struct Skeleton;
}

enum EXPORT_FLAGS
{
	MESH = 0,
	SKELETON = 1 << 0,
	ANIMATION = 1 << 1
};

class Converter
{
public:
	Converter(const char* fileName);
	~Converter();

	void convertFileToCustomFormat(EXPORT_FLAGS flags);
	void convertFileToCustomFormat();
private:
	FbxManager * sdk_Manager;
	FbxIOSettings* sdk_IOSettings;
	FbxImporter* sdk_importer;
	FbxScene* sdk_scene;
	FbxNode* scene_rootNode;

	std::string modelActualName = "";
	std::string finalExportDirectory = "";

	void getSceneMeshes(FbxNode* scene_node);
	bool getSceneAnimationData(FbxNode* scene_node, std::vector<std::vector<FBXExport::DecomposedTransform>> &transform_vector);
	bool createMeshFiles(int nrOfVertices, std::vector<FbxVector4>Positions, std::vector<FbxVector4>normals, std::vector<FbxVector2>UV, const char* node_name);
	bool createAnimatedMeshFile(FbxNode * scene_node, int nrOfVertices, std::vector<FbxVector4> Positions, std::vector<FbxVector4> normals, std::vector<FbxVector2> UV, const char * node_name, std::vector<int> weight_indices_vector);
	bool createSkeletonFile(const FBXExport::Skeleton& skeleton);
	void createAnimationFile(std::vector<std::vector<FBXExport::DecomposedTransform>> keys);
};