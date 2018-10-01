#include "Manager.h"
//#include "ExportHelpers.h"
#include <windows.h>
#include <DirectXMath.h>
#include "ExportHelpers.h"
#define ZERO_IF_SMALL(x) if(std::fabs(x<0.0001))x=0.0f
#define EXPORT_LOCATION "C:/Repos/RipTag/Assets/"
#define COLOR_ATTRIBUTE_GREEN 2
#define COLOR_ATTRIBUTE_RED 4
#define COLOR_ATTRIBUTE_BLUE 9
#define COLOR_ATTRIBUTE_WHITE 7
#define COLOR_ATTRIBUTE_GREY 8
#define COPY_DOUBLE_FLOAT(COUNT, PTR1, PTR2) \
{ \
int counter = -1; while(counter++ < COUNT-1) PTR2[counter] = (float)PTR1[counter];  \
};

HANDLE hConsole;

void ProcessSkeletonHierarchy(FbxNode* inRootNode, FBXExport::Skeleton& skeleton);

Converter::Converter(const char* fileName)
{
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREEN);
	std::cout << "Converting " << fileName << "..." << std::endl << std::endl;

	sdk_Manager = FbxManager::Create();
	sdk_IOSettings = FbxIOSettings::Create(sdk_Manager, IOSROOT);
	sdk_Manager->SetIOSettings(sdk_IOSettings);
	sdk_importer = FbxImporter::Create(sdk_Manager, "");
	sdk_scene = FbxScene::Create(sdk_Manager, "");

	if (!sdk_importer->Initialize(fileName, -1, sdk_Manager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", sdk_importer->GetStatus().GetErrorString());
		exit(-1);
	}
	sdk_importer->Import(sdk_scene);
	sdk_importer->Destroy();
	scene_rootNode = sdk_scene->GetRootNode();
}

Converter::~Converter()
{
	sdk_Manager->Destroy();
}

void Converter::convertFileToCustomFormat()
{
	getSceneMeshes(scene_rootNode);

	std::vector<std::vector<FBXExport::DecomposedTransform>> animation_key_vectors;

	getSceneAnimationData(scene_rootNode, animation_key_vectors) ? printf("GetSceneANimationData run\n") : printf("GetSceneANimationData failed in main\n ");
	if (animation_key_vectors.size() > 0)
		createAnimationFile(animation_key_vectors);
}

void Converter::getSceneMeshes(FbxNode* scene_node)
{

	if (!scene_node)
	{
		std::cout << "Error in getSceneMeshes \n";
		return;
	}

	FbxMesh* scene_mesh = scene_node->GetMesh();

	if (scene_mesh)
	{
		std::vector<FbxVector4> normals;
		std::vector<FbxVector2> UV;
		std::vector<FbxVector4> Positions;
		FbxVector4* vertices = scene_mesh->GetControlPoints();
		std::vector<int> polygon_vertices_indices;
		std::vector<int> tempVertexIndices;
		int* polyVertices = scene_mesh->GetPolygonVertices();
		for (int polygonIndex = 0; polygonIndex < scene_mesh->GetPolygonCount(); polygonIndex++)
		{
			int start = scene_mesh->GetPolygonVertexIndex(polygonIndex);
			for (int polyvertexindex = 0; polyvertexindex < scene_mesh->GetPolygonSize(polygonIndex); polyvertexindex++)
			{
				Positions.push_back(vertices[scene_mesh->GetPolygonVertex(polygonIndex, polyvertexindex)]);

				polygon_vertices_indices.push_back(polyVertices[start + polyvertexindex]);
				FbxVector4 tempVec;
				scene_mesh->GetPolygonVertexNormal(polygonIndex, polyvertexindex, tempVec);
				normals.push_back(tempVec);
				FbxStringList scene_mesh_uv_set_list;
				scene_mesh->GetUVSetNames(scene_mesh_uv_set_list);
				const char* lUVSetName = scene_mesh_uv_set_list.GetStringAt(0);
				FbxGeometryElementUV* lUVElement = scene_mesh->GetElementUV(lUVSetName);
				if (!lUVElement)
					printf("No UVS\n");
				FbxVector2 tempUV;
				bool yayornay = false;
				scene_mesh->GetPolygonVertexUV(polygonIndex, polyvertexindex, lUVSetName, tempUV, yayornay);
				UV.push_back(tempUV);


			}

			tempVertexIndices.clear();
		}

		const char* node_name = scene_node->GetName();
		FbxDeformer* mesh_deformer = (scene_mesh->GetDeformer(0));
		if (!mesh_deformer)
			createMeshFiles(scene_mesh->GetPolygonCount() * 3, Positions, normals, UV, node_name) ? printf("Binary mesh file created\n") : printf("Could not create file");
		else
		{
			FbxSkin* mesh_skin = (FbxSkin*)(scene_mesh->GetDeformer(0, FbxDeformer::eSkin));
			FbxBlendShape* mesh_blend_shape = (FbxBlendShape*)(scene_mesh->GetDeformer(0, FbxDeformer::eBlendShape));

			if (mesh_skin)
				createAnimatedMeshFile(scene_node, scene_mesh->GetPolygonCount() * 3, Positions, normals, UV, node_name, polygon_vertices_indices) ? printf("Animated Mesh file created!\n") : printf("Animated Mesh file could not be created");

			if (mesh_blend_shape) {
				FbxScene * scene = sdk_scene;
				FbxAnimStack* anim_stack = scene->GetCurrentAnimationStack();
				FbxAnimLayer* anim_layer = anim_stack->GetMember<FbxAnimLayer>();
			}
		}
	}

	for (int i = 0; i < scene_node->GetChildCount(); i++)
	{
		getSceneMeshes(scene_node->GetChild(i));
	}
	return;
}

bool Converter::createAnimatedMeshFile(FbxNode * scene_node, int nrOfVertices, std::vector<FbxVector4> Positions, std::vector<FbxVector4> normals, std::vector<FbxVector2> UV, const char * node_name, std::vector<int> weight_indices_vector)
{
	//using FBXExport::Vec2;
	//using FBXExport::Vec3;
	//using FBXExport::Vec4;
	//using FBXExport::Vec4ui;
	//
	std::vector<FBXExport::AnimatedVertexStefan> skin_bound_vertices = std::vector<FBXExport::AnimatedVertexStefan>(nrOfVertices);

	for (int i = 0; i < nrOfVertices; i++)
		skin_bound_vertices[i] = FBXExport::AnimatedVertexStefan(FBXExport::Vec3(Positions[i]), FBXExport::Vec2(UV[i]), FBXExport::Vec3(normals[i]));

	FbxMesh* scene_mesh = scene_node->GetMesh();
	FBXExport::Skeleton eSkeleton;

	if (scene_mesh)
	{
		std::vector<FbxAMatrix> skeleton_joints_temp_vector;
		std::vector<std::string> skeleton_joint_names;
		std::vector<FbxNode*> skeleton_joint_parent_names;

		FbxAMatrix transformation_matrix;
		transformation_matrix.SetT(scene_node->LclTranslation.Get());
		transformation_matrix.SetR(scene_node->LclRotation.Get());
		transformation_matrix.SetS(scene_node->LclScaling.Get());

		FbxCluster * skin_cluster = nullptr;

		int skin_count = scene_node->GetMesh()->GetDeformerCount(FbxDeformer::eSkin);

		std::vector<int> indicesVector;
		std::vector<double> weightVector;
		std::vector<FbxVector4> controlPointsVector;

		for (int i = 0; i < skin_count; i++)
		{
			FbxSkin* mesh_skin = (FbxSkin*)(scene_mesh->GetDeformer(i, FbxDeformer::eSkin));
			if (!mesh_skin)
				continue;

			for (int j = 0; j < mesh_skin->GetClusterCount(); j++)
			{
				if (j == 0) ///root node, build hierarchy
				{
					FbxSkeleton* skeleton = static_cast<FbxSkeleton*>(mesh_skin->GetCluster(j)->GetLink()->GetNodeAttribute());
					if (skeleton)
					{
						ProcessSkeletonHierarchy(skeleton->GetNode(), eSkeleton);
						int i = 0;
					}
				}

				{
					SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREY);
					std::cout << "Processing joint ";
					SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
					std::cout << j << std::endl;
				}

				skin_cluster = mesh_skin->GetCluster(j);

				auto thisJointName = skin_cluster->GetLink()->GetNodeAttribute()->GetNode()->GetNameOnly();
				FBXExport::Bone* thisBone = nullptr;
				int skeletonIndex = 0;
				for (int i = 0; i < eSkeleton.joints.size(); i++)
				{
					if (eSkeleton.joints[i].name == thisJointName)
					{
						thisBone = &eSkeleton.joints[i];
						skeletonIndex = i;
					}
				}

				/// ------------ #InverseBindPose
				std::string connectedJointName = skin_cluster->GetLink()->GetName();
				std::string parent_name = skin_cluster->GetLink()->GetParent()->GetName();

				FbxAMatrix transformation;
				FbxAMatrix linked_Transformation;
				FbxAMatrix inverse_bind_pose;
				/// #todo remove
				skin_cluster->GetTransformMatrix(transformation);
				skin_cluster->GetTransformLinkMatrix(linked_Transformation);
				inverse_bind_pose = linked_Transformation.Inverse() * transformation;

				thisBone->jointInverseBindPoseTransform = linked_Transformation.Inverse();
				thisBone->jointReferenceTransform = transformation;
				/// -------------

				SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREY);
				std::cout << "\t" << connectedJointName << std::endl << std::endl;

				/// ------------- Weights/Indices 
				double * weights = skin_cluster->GetControlPointWeights();
				int *indices = skin_cluster->GetControlPointIndices();

				for (int k = 0; k < skin_bound_vertices.size(); k++)
				{
					for (int x = 0; x < skin_cluster->GetControlPointIndicesCount(); x++)
					{
						if (indices[x] == weight_indices_vector[k])
						{
							for (int m = 0; m < 4; m++)
							{
								if (skin_bound_vertices[k].influencingJoints[m] == 0)
								{
									skin_bound_vertices[k].influencingJoints[m] = j + 1;
									skin_bound_vertices[k].jointWeights[m] = weights[x];
									m = 4;
								}
							}
						}
					}
				}
			}
		}

		createSkeletonFile(eSkeleton);
		int nrOfVerts = nrOfVertices;
		const char * temp = "_Mesh";
		// #todo proper file name
		std::string filename = std::string(EXPORT_LOCATION) + /*std::string(node_name) + */"test_ANIMATION_Mesh.bin";
		std::ofstream outfile(filename, std::ofstream::binary);
		std::string readable_file_name = std::string(EXPORT_LOCATION) + /*std::string(node_name) +*/ "test_ANIMATION_Mesh.txt";
		std::ofstream readable_file(readable_file_name);

		std::cout << "Found mesh:  " << node_name << " Nr of verts: " << nrOfVerts << "\n";

		if (!outfile)
		{
			printf("No file could be opened. Manager.cpp line 122");
			return false;
		}
		if (!readable_file)
		{
			printf("No file could be opened. Manager.cpp line 122");
			return false;
		}


		readable_file << "NrOfverts: " << nrOfVerts << "\n";
		readable_file.write((const char*)node_name, sizeof(char[100]));
		readable_file << "\n";
		outfile.write((const char*)&nrOfVerts, sizeof(int));
		outfile.write((const char*)node_name, sizeof(const char[100]));

		std::vector<float> tanVec;
		// tangent stuff
		{
			float tan[3];
			float dVec1[3];
			float dVec2[3];
			float vec1[3];
			float vec2[3];
			float uVec1[2];
			float uVec2[2];

			for (int i = 0; i < nrOfVerts; i += 3)
			{

				vec1[0] = Positions[i + 1][0] - Positions[i][0];
				vec1[1] = Positions[i + 1][1] - Positions[i][1];
				vec1[2] = Positions[i + 1][2] - Positions[i][2];

				vec2[0] = Positions[i + 2][0] - Positions[i][0];
				vec2[1] = Positions[i + 2][1] - Positions[i][1];
				vec2[2] = Positions[i + 2][2] - Positions[i][2];



				uVec1[0] = UV[i + 1][0] - UV[i][0];
				uVec1[1] = UV[i + 1][1] - UV[i][1];

				uVec2[0] = UV[i + 2][0] - UV[i][0];
				uVec2[1] = UV[i + 2][1] - UV[i][1];

				float denominator = (uVec1[0] * uVec2[1]) - (uVec1[1] * uVec2[0]);
				float someFloat = 1.0f / denominator;



				dVec1[0] = vec1[0] * uVec2[1];
				dVec1[1] = vec1[1] * uVec2[1];
				dVec1[2] = vec1[2] * uVec2[1];

				dVec2[0] = vec2[0] * uVec1[1];
				dVec2[1] = vec2[1] * uVec1[1];
				dVec2[2] = vec2[2] * uVec1[1];


				tan[0] = dVec1[0] - dVec2[0];
				tan[1] = dVec1[1] - dVec2[1];
				tan[2] = dVec1[2] - dVec2[2];

				tan[0] = tan[0] * someFloat;
				tan[1] = tan[1] * someFloat;
				tan[2] = tan[2] * someFloat;

				for (int j = 0; j < 3; j++)
				{
					for (int x = 0; x < 3; x++)
					{
						tanVec.push_back(tan[x]);
					}
				}
			}
		}

		for (int i = 0; i < nrOfVerts; i++)
		{
			FBXExport::appendFloat3AsDirectXVector(outfile, skin_bound_vertices[i].position);
			FBXExport::appendFloat2(outfile, skin_bound_vertices[i].UV);
			FBXExport::appendFloat3AsDirectXVector(outfile, skin_bound_vertices[i].normal);

			auto first = tanVec.begin();
			auto last = tanVec.begin() + 3;
			std::vector<float> x(first, last);
			FBXExport::Vec3 tan = FBXExport::Vec3(x[0], x[1], x[2]);
			tanVec.erase(first, last);
			
			FBXExport::appendFloat3AsDirectXVector(outfile, tan);
			FBXExport::appendUInt4(outfile, skin_bound_vertices[i].influencingJoints);
			FBXExport::appendFloat4(outfile, skin_bound_vertices[i].jointWeights);

			readable_file << i << ":  Position:  X " << skin_bound_vertices[i].position[0] << " Y " << skin_bound_vertices[i].position[1] << " Z " << skin_bound_vertices[i].position[2] << "\n";
			readable_file << i << ":  UV:  U " << skin_bound_vertices[i].UV[0] << "V " << skin_bound_vertices[i].UV[1] << "\n";
			readable_file << i << ":  Normals:  X " << skin_bound_vertices[i].normal[0] << " Y " << skin_bound_vertices[i].normal[1] << " Z " << skin_bound_vertices[i].normal[2] << "\n";
			readable_file << i << ":  tan:  X " << x[0] << " Y " << x[1] << " Z " << x[2] << "\n";
			readable_file << i << ":  Weights:  1: " << temp[0] << " 2: " << temp[1] << " 3: " << temp[2] << " 4: " << temp[3] << "\n";
			readable_file << i << ":  Influencing indices:  1: " << skin_bound_vertices[i].influencingJoints[0] << " 2: " << skin_bound_vertices[i].influencingJoints[1] << " 3: " << skin_bound_vertices[i].influencingJoints[2] << " 4: " << skin_bound_vertices[i].influencingJoints[3] << "\n";
		}

		readable_file.close();
		outfile.close();
	}

	return true;

	return false;
}

bool Converter::createMeshFiles(int nrOfVertices, std::vector<FbxVector4>Positions, std::vector<FbxVector4>normals, std::vector<FbxVector2>UV, const char* node_name)
{

	int nrOfVerts = nrOfVertices;
	const char * temp = "_Mesh";
	std::string filename = std::string(EXPORT_LOCATION) + /*std::string(node_name) +*/ "test_Mesh.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_LOCATION) + /*std::string(node_name) +*/ "test_Mesh.txt";
	std::ofstream readable_file(readable_file_name);

	std::cout << "Found mesh:  " << node_name << " Nr of verts: " << nrOfVerts << "\n";

	if (!outfile)
	{
		return false;
	}
	if (!readable_file)
	{
		return false;
	}

	readable_file << "NrOfverts: " << nrOfVerts << "\n";

	readable_file.write((const char*)node_name, sizeof(char[100]));
	readable_file << "\n";
	outfile.write((const char*)&nrOfVerts, sizeof(int));
	outfile.write((const char*)node_name, sizeof(const char[100]));


	float tan[3];
	float dVec1[3];
	float dVec2[3];
	float vec1[3];
	float vec2[3];
	float uVec1[2];
	float uVec2[2];
	std::vector<float> tanVec;

	for (int i = 0; i < nrOfVerts; i += 3)
	{

		vec1[0] = Positions[i + 1][0] - Positions[i][0];
		vec1[1] = Positions[i + 1][1] - Positions[i][1];
		vec1[2] = Positions[i + 1][2] - Positions[i][2];

		vec2[0] = Positions[i + 2][0] - Positions[i][0];
		vec2[1] = Positions[i + 2][1] - Positions[i][1];
		vec2[2] = Positions[i + 2][2] - Positions[i][2];



		uVec1[0] = UV[i + 1][0] - UV[i][0];
		uVec1[1] = UV[i + 1][1] - UV[i][1];

		uVec2[0] = UV[i + 2][0] - UV[i][0];
		uVec2[1] = UV[i + 2][1] - UV[i][1];

		float denominator = (uVec1[0] * uVec2[1]) - (uVec1[1] * uVec2[0]);
		float someFloat = 1.0f / denominator;



		dVec1[0] = vec1[0] * uVec2[1];
		dVec1[1] = vec1[1] * uVec2[1];
		dVec1[2] = vec1[2] * uVec2[1];

		dVec2[0] = vec2[0] * uVec1[1];
		dVec2[1] = vec2[1] * uVec1[1];
		dVec2[2] = vec2[2] * uVec1[1];


		tan[0] = dVec1[0] - dVec2[0];
		tan[1] = dVec1[1] - dVec2[1];
		tan[2] = dVec1[2] - dVec2[2];

		tan[0] = tan[0] * someFloat;
		tan[1] = tan[1] * someFloat;
		tan[2] = tan[2] * someFloat;

		for (int j = 0; j < 3; j++)
		{
			for (int x = 0; x < 3; x++)
			{
				tanVec.push_back(tan[x]);
			}
		}
		//readable_file << i << ":  Tan:  X " << tan[0] << " Y " << tan[1] << " Z " << tan[2] << "\n";
	}
	int a = tanVec.size();

	for (int i = 0; i < nrOfVerts; i++)
	{
		float temp[3];

		COPY_DOUBLE_FLOAT(3, Positions[i], temp);

		outfile.write((const char*)&temp[0], sizeof(float));
		outfile.write((const char*)&temp[1], sizeof(float));
		outfile.write((const char*)&temp[2], sizeof(float));
		readable_file << i << ":  Position:  X " << temp[0] << " Y " << temp[1] << " Z " << temp[2] << "\n";

		COPY_DOUBLE_FLOAT(2, UV[i], temp);

		outfile.write((const char*)&temp[0], sizeof(float));
		outfile.write((const char*)&temp[1], sizeof(float));

		readable_file << i << ":  UV:  U " << temp[0] << "V " << temp[1] << "\n";


		COPY_DOUBLE_FLOAT(3, normals[i], temp);
		outfile.write((const char*)&temp[0], sizeof(float));
		outfile.write((const char*)&temp[1], sizeof(float));
		outfile.write((const char*)&temp[2], sizeof(float));
		readable_file << i << ":  Normals:  X " << temp[0] << " Y " << temp[1] << " Z " << temp[2] << "\n";


		auto first = tanVec.begin();
		auto last = tanVec.begin() + 3;

		std::vector<float> x(first, last);
		tanVec.erase(first, last);
		outfile.write((const char*)&x[0], sizeof(float));
		outfile.write((const char*)&x[1], sizeof(float));
		outfile.write((const char*)&x[2], sizeof(float));
		readable_file << i << ":  tan:  X " << x[0] << " Y " << x[1] << " Z " << x[2] << "\n";

	}




	readable_file.close();
	outfile.close();

	return true;
}

FbxAMatrix GetAbsoluteTransformFromCurrentTake(FbxNode* pNode, FbxTime time)
{
	FbxAMatrix mat;
	if (!pNode)
	{
		mat.SetIdentity();
	}
	else
	{
		mat = pNode->GetScene()->GetAnimationEvaluator()->GetNodeGlobalTransform(pNode, time);

		//mat = pNode->GetScene()->GetAnimationEvaluator()->GetNodeLocalTransform(pNode, time);
	}

	return mat;
}

bool Converter::getSceneAnimationData(FbxNode * scene_node, std::vector<std::vector<FBXExport::DecomposedTransform>> &transform_vector)
{
	FbxScene * scene = sdk_scene;
	FbxAnimStack* anim_stack = scene->GetCurrentAnimationStack();
	FbxAnimLayer* anim_layer = anim_stack->GetMember<FbxAnimLayer>();


	FbxAnimCurve* anim_curve_rotY = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y); //ghetto

	if (anim_curve_rotY)
	{
		FbxSkeleton* skeleton = static_cast<FbxSkeleton*>(scene_node->GetNodeAttribute());
		if (skeleton)
		{
			float frameTime = 1.0 / 24.0;
			int keyCount = anim_curve_rotY->KeyGetCount();

			float endTime = frameTime * keyCount;
			float currentTime = frameTime;

			std::vector<FbxVector4> rotVector;
			std::vector<FBXExport::DecomposedTransform> thisTransformVector;
			while (currentTime < endTime)
			{
				FbxTime takeTime;
				takeTime.SetSecondDouble(currentTime);

				// #calculateLocalTransform
				FbxAMatrix matAbsoluteTransform2 = GetAbsoluteTransformFromCurrentTake(skeleton->GetNode(), takeTime);
				FbxAMatrix matParentAbsoluteTransform2 = GetAbsoluteTransformFromCurrentTake(skeleton->GetNode()->GetParent(), takeTime);
				FbxAMatrix matInvParentAbsoluteTransform2 = matParentAbsoluteTransform2.Inverse();
				FbxAMatrix matTransform2 = matInvParentAbsoluteTransform2 * matAbsoluteTransform2;

				// Get T, R, S
				auto translation = matTransform2.GetT();
				auto rotation = matTransform2.GetQ();
				auto scale = matTransform2.GetS();

				FBXExport::DecomposedTransform thisTransform = {};
				thisTransform.translation = FBXExport::Vec4(translation);
				thisTransform.rotation    = FBXExport::Vec4(rotation);
				thisTransform.scale       = FBXExport::Vec4(scale);

				thisTransformVector.push_back(thisTransform);

				//increment time
				currentTime += frameTime;
			}
			transform_vector.push_back(thisTransformVector);
		}
	}

	//Keep going for each child (joint)

	for (int i = 0; i < scene_node->GetChildCount(); i++)
	{
		getSceneAnimationData(scene_node->GetChild(i), transform_vector);
	}

	return true;
}

bool Converter::createSkeletonFile(const FBXExport::Skeleton& skeleton)
{
	std::string rootJointName = std::string(skeleton.joints[0].name.Buffer());
	std::string filename = std::string(EXPORT_LOCATION) + rootJointName + std::string("_Skeleton.bin");
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_LOCATION) + rootJointName + std::string("_Skeleton.txt");
	std::ofstream readable_file(readable_file_name);

	FBXExport::appendSkeleton(outfile, skeleton);

	outfile.close();
	readable_file.close();

	return true;
}

void Converter::createAnimationFile(std::vector<std::vector<FBXExport::DecomposedTransform>> keys)
{
	std::string filename = std::string(EXPORT_LOCATION) + std::string("ANIMATION") + "_ANIMATION.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_LOCATION) + std::string("ANIMATION") + "_ANIMATION.txt";
	std::ofstream readable_file(readable_file_name);

	int32_t nrOfKeys = keys[0].size();
	outfile.write((const char*)&nrOfKeys, sizeof(int32_t));
	readable_file << nrOfKeys << "\n";

	for (int i = 0; i < keys[0].size(); i++)
	{
		for (int j = 0; j < keys.size(); j++)
		{
			FBXExport::appendTransform(outfile, keys[j][i]);
		}

	}

	outfile.close();
	readable_file.close();
}

void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, FBXExport::Skeleton& skeleton)
{
	int childCount = inNode->GetChildCount();
	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		FBXExport::Bone currJoint;;
		currJoint.name = inNode->GetNameOnly();

		//find and set parent index
		for (int i = 0; i < skeleton.joints.size(); i++) 
		{
			auto parentName = inNode->GetParent()->GetNameOnly();
			if (skeleton.joints[i].name == parentName)
				currJoint.parentIndex = i;
		}
		
		skeleton.joints.push_back(currJoint);
	}
	for (int i = 0; i < childCount; i++)
	{
		ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), skeleton);
	}
}

void ProcessSkeletonHierarchy(FbxNode* inRootNode, FBXExport::Skeleton& skeleton)
{
	FBXExport::Bone root;
	root.parentIndex = -1;
	root.name = inRootNode->GetNameOnly();
	skeleton.joints.push_back(root);

	int childCount = inRootNode->GetChildCount();

	for (int childIndex = 0; childIndex < childCount; ++childIndex)
	{
		std::cout << "Processing ROOT child " << childIndex << std::endl;
		FbxNode* currNode = inRootNode->GetChild(childIndex);
		ProcessSkeletonHierarchyRecursively(currNode, skeleton);
	}
}
