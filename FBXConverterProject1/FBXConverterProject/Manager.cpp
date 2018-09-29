#include "Manager.h"
#include <windows.h>
#include <DirectXMath.h>
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
	//getGroups(scene_rootNode);
	//getCamera(scene_rootNode);
	//if (cameras.size() > 0)
	//	createCameraFiles(cameras);
	//getLight(scene_rootNode);
	//if (lights.size() > 0)
	//	createLightFile(lights);
	getSceneMeshes(scene_rootNode);


	std::vector<std::vector<Transform>> animation_key_vectors;

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
		/*getCustomAttributes(scene_node);
		createMaterialsFile(scene_node);*/
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
				/*createBlendShapeFiles(scene_mesh, anim_layer);*/
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
	std::vector<AnimatedVertex> skin_bound_vertices = std::vector<AnimatedVertex>(nrOfVertices);

	for (int i = 0; i < nrOfVertices; i++)
	{
		COPY_DOUBLE_FLOAT(3, Positions[i], skin_bound_vertices[i].vertex_position);
		COPY_DOUBLE_FLOAT(3, normals[i], skin_bound_vertices[i].vertex_normal);
		COPY_DOUBLE_FLOAT(2, UV[i], skin_bound_vertices[i].vertex_UVCoord);


		skin_bound_vertices[i].joint_weights[0] = 0.0f;
		skin_bound_vertices[i].joint_weights[1] = 0.0f;
		skin_bound_vertices[i].joint_weights[2] = 0.0f;
		skin_bound_vertices[i].joint_weights[3] = 0.0f;

		skin_bound_vertices[i].influencing_joint[0] = 0;
		skin_bound_vertices[i].influencing_joint[1] = 0;
		skin_bound_vertices[i].influencing_joint[2] = 0;
		skin_bound_vertices[i].influencing_joint[3] = 0;


	}
	FbxMesh* scene_mesh = scene_node->GetMesh();

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
		std::vector<int> correspondingJointVec;
		std::vector<FbxVector4> controlPointsVector;
		FbxAMatrix tempMat;

		for (int i = 0; i < skin_count; i++)
		{
			FbxSkin* mesh_skin = (FbxSkin*)(scene_mesh->GetDeformer(i, FbxDeformer::eSkin));
			if (!mesh_skin)
				continue;
			FbxAMatrix temp;
			temp.SetIdentity();
			for (int j = 0; j < mesh_skin->GetClusterCount(); j++)
			{

				{
					SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREY);
					std::cout << "Processing joint ";
					SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
					std::cout << j << std::endl;
				}
				
				skin_cluster = mesh_skin->GetCluster(j);
				std::string connectedJointName = skin_cluster->GetLink()->GetName();
				std::string parent_name = skin_cluster->GetLink()->GetParent()->GetName();

				FbxAMatrix transformation;
				FbxAMatrix linked_Transformation;


				//skin_cluster->GetTransformMatrix(transformation);
				//skin_cluster->GetTransformLinkMatrix(linked_Transformation);
				//linked_Transformation.SetR(skin_cluster->GetLink()->EvaluateLocalRotation());
				//FbxAMatrix inverse_bind_pose = linked_Transformation.Inverse() * transformation * transformation_matrix;

				//auto rot = inverse_bind_pose.GetR();
				//rot.mData[0] *= -1.0f;
				//rot.mData[1] *= -1.0f;
				//inverse_bind_pose.SetR(rot);
				skin_cluster->GetTransformMatrix(transformation);
				skin_cluster->GetTransformLinkMatrix(linked_Transformation);
				//linked_Transformation.SetR(skin_cluster->GetLink()->GetPreRotation(FbxNode::eSourcePivot));
				//linked_Transformation.SetR(skin_cluster->GetLink()->GetPreRotation(FbxNode::eSourcePivot));

				FbxAMatrix inverse_bind_pose = linked_Transformation.Inverse() * transformation;

				auto test = inverse_bind_pose.GetR();
				auto test2 = inverse_bind_pose.GetQ();

				auto rebuilt = DirectX::XMQuaternionRotationRollPitchYaw(test.mData[0], test.mData[1], test.mData[2]);
				auto rebuiltNegated = rebuilt;
				

				inverse_bind_pose.SetR(skin_cluster->GetLink()->EvaluateLocalRotation());
				if (j == 0)
				{
					inverse_bind_pose.SetIdentity();
				}
				else
				{
					inverse_bind_pose = skin_cluster->GetLink()->EvaluateGlobalTransform().Inverse();
				}

					//FbxAMatrix pre;
					//pre.SetR(skin_cluster->GetLink()->PreRotation.Get());
					//FbxAMatrix post;
					//post.SetR(skin_cluster->GetLink()->GetPostTargetRotation());
					////inverse_bind_pose = pre * linked_Transformation * post;
					//inverse_bind_pose.set

					////inverse_bind_pose.SetR(skin_cluster->GetLink()->GetPreRotation(FbxNode::eSourcePivot));
					////inverse_bind_pose *= skin_cluster->GetLink()->EvaluateLocalTransform(FBXSDK_TIME_INFINITE).Inverse();
					////inverse_bind_pose = skin_cluster->GetLink()->EvaluateLocalTransform(FBXSDK_TIME_INFINITE);
					//std::cout << skin_cluster->GetLink()->GetName() << std::endl;
					//JOINT INFORMATION
				skeleton_joint_names.push_back(connectedJointName);
				skeleton_joints_temp_vector.push_back(inverse_bind_pose);
				skeleton_joint_parent_names.push_back(skin_cluster->GetLink());

				SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREY);
				std::cout << "\t" << connectedJointName << std::endl << std::endl;
				//getNames(scene_node);
				//	printf("Iteration on: %d\n", i);

				//printf("| [%f, %f, %f, %f |\n", inverse_bind_pose[0][0], inverse_bind_pose[0][1], inverse_bind_pose[0][2], inverse_bind_pose[0][3]);
				//printf("| [%f, %f, %f, %f |\n", inverse_bind_pose[1][0], inverse_bind_pose[1][1], inverse_bind_pose[1][2], inverse_bind_pose[1][3]);
				//printf("| [%f, %f, %f, %f |\n", inverse_bind_pose[2][0], inverse_bind_pose[2][1], inverse_bind_pose[2][2], inverse_bind_pose[2][3]);
				//printf("| [%f, %f, %f, %f |\n", inverse_bind_pose[3][0], inverse_bind_pose[3][1], inverse_bind_pose[3][2], inverse_bind_pose[3][3]);
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
								if (skin_bound_vertices[k].influencing_joint[m] == 0)
								{
									skin_bound_vertices[k].influencing_joint[m] = j + 1;
									skin_bound_vertices[k].joint_weights[m] = weights[x];
									m = 4;
								}

							}
						}


					}

				}
			}


		}

		createSkeletonFile(skeleton_joint_names, skeleton_joints_temp_vector, skeleton_joint_names.size(), skeleton_joint_parent_names);
		int nrOfVerts = nrOfVertices;
		const char * temp = "_Mesh";
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


		/*readable_file << "Name of Mesh: " << node_name << "\n";
		readable_file << "Number of vertices: " << nrOfVerts << "\n";*/
		readable_file << "NrOfverts: " << nrOfVerts << "\n";
		//readable_file.write((const char*)&nrOfVertices, sizeof(nrOfVerts));
		readable_file.write((const char*)node_name, sizeof(char[100]));
		readable_file << "\n";
		for (int i = 0; i < nrOfVerts; i++)
		{
			/*	readable_file << i << ":  Position:  X " << Positions[i][0] << " Y " << Positions[i][1] << " Z " << Positions[i][2] << "\n"
			"Normals:  X " << normals[i][0] << " Y" << normals[i][1] << " Z " << normals[i][2] << "\n"
			" U: " << UV[i][0] << " V: " << UV[i][1] << " \n";
			*/
		}
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


		for (int i = 0; i < nrOfVerts; i++)
		{
			//TODO READ/WRITE FLOAT INSTEAD DOUBLES->PAIR WITH ENGINE
			float temp[4];
			unsigned int tempWeights[4];
			for (int j = 0; j < 4; j++)
			{
				tempWeights[j] = skin_bound_vertices[i].influencing_joint[j];
			}
			COPY_DOUBLE_FLOAT(3, skin_bound_vertices[i].vertex_position, temp);

			outfile.write((const char*)&skin_bound_vertices[i].vertex_position[0], sizeof(float));
			outfile.write((const char*)&skin_bound_vertices[i].vertex_position[1], sizeof(float));
			outfile.write((const char*)&skin_bound_vertices[i].vertex_position[2], sizeof(float));
			readable_file << i << ":  Position:  X " << temp[0] << " Y " << temp[1] << " Z " << temp[2] << "\n";

			COPY_DOUBLE_FLOAT(2, UV[i], temp);

			outfile.write((const char*)&skin_bound_vertices[i].vertex_UVCoord[0], sizeof(float));
			outfile.write((const char*)&skin_bound_vertices[i].vertex_UVCoord[1], sizeof(float));

			readable_file << i << ":  UV:  U " << temp[0] << "V " << temp[1] << "\n";


			COPY_DOUBLE_FLOAT(3, normals[i], temp);
			outfile.write((const char*)&skin_bound_vertices[i].vertex_normal[0], sizeof(float));
			outfile.write((const char*)&skin_bound_vertices[i].vertex_normal[1], sizeof(float));
			outfile.write((const char*)&skin_bound_vertices[i].vertex_normal[2], sizeof(float));

			readable_file << i << ":  Normals:  X " << skin_bound_vertices[i].vertex_normal[0] << " Y " << skin_bound_vertices[i].vertex_normal[1] << " Z " << skin_bound_vertices[i].vertex_normal[2] << "\n";
			auto first = tanVec.begin();
			auto last = tanVec.begin() + 3;

			std::vector<float> x(first, last);
			tanVec.erase(first, last);
			outfile.write((const char*)&x[0], sizeof(float));
			outfile.write((const char*)&x[1], sizeof(float));
			outfile.write((const char*)&x[2], sizeof(float));
			readable_file << i << ":  tan:  X " << x[0] << " Y " << x[1] << " Z " << x[2] << "\n";

			outfile.write((const char*)&tempWeights[0], sizeof(unsigned int));
			outfile.write((const char*)&tempWeights[1], sizeof(unsigned int));
			outfile.write((const char*)&tempWeights[2], sizeof(unsigned int));
			outfile.write((const char*)&tempWeights[3], sizeof(unsigned int));

			COPY_DOUBLE_FLOAT(4, skin_bound_vertices[i].joint_weights, temp);
			outfile.write((const char*)&temp[0], sizeof(float));
			outfile.write((const char*)&temp[1], sizeof(float));
			outfile.write((const char*)&temp[2], sizeof(float));
			outfile.write((const char*)&temp[3], sizeof(float));

			readable_file << i << ":  Weights:  1: " << temp[0] << " 2: " << temp[1] << " 3: " << temp[2] << " 4: " << temp[3] << "\n";

			readable_file << i << ":  Influencing indices:  1: " << tempWeights[0] << " 2: " << tempWeights[1] << " 3: " << tempWeights[2] << " 4: " << tempWeights[3] << "\n";

		}



		readable_file.close();
		outfile.close();
	}

	return true;
}


std::vector<FbxMatrix> Converter::getInverseBindPoseTransforms(FbxNode* skeletonRoot)
{
	return std::vector<FbxMatrix>();
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

void Converter::getNames(FbxNode * scene_node)
{
	printf("Node name:\t%s \t", scene_node->GetName());
	printf("Node type:\t%s\n", scene_node->GetTypeName());
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

bool Converter::getSceneAnimationData(FbxNode * scene_node, std::vector<std::vector<Transform>> &transform_vector)
{
	FbxScene * scene = sdk_scene;
	FbxAnimStack* anim_stack = scene->GetCurrentAnimationStack();
	FbxAnimLayer* anim_layer = anim_stack->GetMember<FbxAnimLayer>();


	FbxAnimCurve* anim_curve_rotY = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);

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
			std::vector<Transform> thisTransformVector;
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
				auto rotation = matTransform2.GetR();
				//DirectX::XMFLOAT4 q;
				//DirectX::XMStoreFloat4(&q, DirectX::XMQuaternionRotationRollPitchYaw(rotation.mData[0], rotation.mData[1], rotation.mData[2]));
				auto scale = matTransform2.GetS();

				Transform thisTransform = {};
				thisTransform.transform_position[0] = translation.mData[0];
				thisTransform.transform_position[1] = translation.mData[1];
				thisTransform.transform_position[2] = translation.mData[2];

				thisTransform.transform_rotation[0] = rotation.mData[0];
				thisTransform.transform_rotation[1] = rotation.mData[1];
				thisTransform.transform_rotation[2] = rotation.mData[2];

				thisTransform.transform_scale[0] = scale.mData[0];
				thisTransform.transform_scale[1] = scale.mData[1];
				thisTransform.transform_scale[2] = scale.mData[2];

				thisTransformVector.push_back(thisTransform);

				currentTime += frameTime;
			}
			transform_vector.push_back(thisTransformVector);

		}
		//std::vector<Transform> transform_temp;


		//std::vector<FbxVector4> temp_translations;
		//std::vector<FbxVector4> temp_rotations;
		//std::vector<FbxVector4> temp_scaling;
		//FbxAnimCurve* anim_curve_rotZ = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);
		//FbxAnimCurve* anim_curve_rotX = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);
		//
		//auto postRot = scene_node->GetPostRotation(FbxNode::eSourcePivot);
		//auto preRot = scene_node->GetPreRotation(FbxNode::eSourcePivot);

		//FbxAnimCurve* anim_curve_translateY = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);
		//FbxAnimCurve* anim_curve_translateX = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);
		//FbxAnimCurve* anim_curve_translateZ = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);

		//FbxAnimCurve* anim_curve_scaleY = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);
		//FbxAnimCurve* anim_curve_scaleX = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);
		//FbxAnimCurve* anim_curve_scaleZ = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);


		//std::vector<FbxAMatrix> matrices;
		//FbxAMatrix matrix;


		//for (int i = 0; i < anim_curve_rotY->KeyGetCount(); i++)
		//{
		//	FbxVector4 translation((double)anim_curve_translateX->KeyGetValue(i), (double)anim_curve_translateY->KeyGetValue(i), (double)anim_curve_translateZ->KeyGetValue(i), 1);
		//	FbxVector4 rotation((double)anim_curve_rotX->KeyGetValue(i), (double)anim_curve_rotY->KeyGetValue(i), (double)anim_curve_rotZ->KeyGetValue(i), 0);
		//	FbxVector4 scale((double)anim_curve_scaleX->KeyGetValue(i), (double)anim_curve_scaleY->KeyGetValue(i), (double)anim_curve_scaleZ->KeyGetValue(i), 0);

		//	Transform transform_to_push_back;

		//	transform_to_push_back.transform_position[0] = (float)anim_curve_translateX->KeyGetValue(i);
		//	transform_to_push_back.transform_position[1] = (float)anim_curve_translateY->KeyGetValue(i);
		//	transform_to_push_back.transform_position[2] = (float)anim_curve_translateZ->KeyGetValue(i);

		//	FbxAMatrix animRotMatrix;
		//	FbxAMatrix preRotMatrix;
		//	FbxAMatrix postRotMatrix;
		//	FbxVector4 animRotation((float)anim_curve_rotX->KeyGetValue(i), (float)anim_curve_rotY->KeyGetValue(i), (float)anim_curve_rotZ->KeyGetValue(i));
		//	animRotMatrix.SetR(animRotation);
		//	preRotMatrix.SetR(preRot);
		//	postRotMatrix.SetR(postRot);

		//	animRotMatrix = preRotMatrix * animRotMatrix * postRotMatrix;

		//	auto rot = animRotMatrix.GetR();
		//	transform_to_push_back.transform_rotation[0] = rot.mData[0];
		//	transform_to_push_back.transform_rotation[1] = rot.mData[1];
		//	transform_to_push_back.transform_rotation[2] = rot.mData[2];
		//	transform_to_push_back.transform_rotation[0] = (float)anim_curve_rotX->KeyGetValue(i);// +preRot.mData[0] + postRot.mData[0];
		//	transform_to_push_back.transform_rotation[1] = (float)anim_curve_rotY->KeyGetValue(i);// +preRot.mData[1] + postRot.mData[1];
		//	transform_to_push_back.transform_rotation[2] = (float)anim_curve_rotZ->KeyGetValue(i);// +preRot.mData[2] + postRot.mData[2];
		//	transform_to_push_back.transform_scale[0] = (float)anim_curve_scaleX->KeyGetValue(i);
		//	transform_to_push_back.transform_scale[1] = (float)anim_curve_scaleY->KeyGetValue(i);
		//	transform_to_push_back.transform_scale[2] = (float)anim_curve_scaleZ->KeyGetValue(i);

		//	transform_temp.push_back(transform_to_push_back);
		//	matrix.SetTRS(translation, rotation, scale);

		//}
		//transform_vector.push_back(transform_temp);
	}


	int yo = scene_node->GetChildCount();
	for (int i = 0; i < scene_node->GetChildCount(); i++)
	{
		getSceneAnimationData(scene_node->GetChild(i), transform_vector);
	}

	return true;
}

bool Converter::createSkeletonFile(std::vector<std::string> names, std::vector<FbxAMatrix> joint_matrices, int nrOfJoints, std::vector<FbxNode*> parent_name_list)
{
	const char * temp = "_Skeleton";
	std::string filename = std::string(EXPORT_LOCATION) + std::string(names[0]) + "_Skeleton.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_LOCATION) + std::string(names[0]) + "_Skeleton.txt";
	std::ofstream readable_file(readable_file_name);

	std::vector<int> parent_index_vector(nrOfJoints);


	std::vector<FbxAMatrix> parentOrients;
	std::vector<std::string> tempParentNames;
	for (int i = 0; i < nrOfJoints; i++)
	{
		tempParentNames.push_back(parent_name_list[i]->GetParent()->GetName());
	}
	parent_index_vector[0] = 0;
	for (int i = 0; i < nrOfJoints; i++)
	{
		for (int j = 0; j < nrOfJoints; j++)
		{
			if (names[j] == tempParentNames[i])
				parent_index_vector[i] = j;
		}
	}
	/*for (int i = 0; i < nrOfJoints; i++)
	{
		for (int j = 0; j < nrOfJoints; j++)
		{
			if()
		}
	}*/
	// #todo
	std::vector<FbxAMatrix> tempMultiplied;
	for (int i = 0; i < joint_matrices.size(); i++)
	{
		FbxAMatrix temp = joint_matrices[i];
		//for (int j = i; j < parentOrients.size(); j++)
		//{
		//	temp *= parentOrients[j];
		//}
		tempMultiplied.push_back(temp);
	}


	for (int i = 0; i < tempMultiplied.size(); i++)
	{
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
		std::cout << "Joint " << i << "orientation:  " << std::endl;

		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_RED);
		std::cout << "X:  ";
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
		std::cout << tempMultiplied[i].GetR()[0] << std::endl;
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_GREEN);
		std::cout << "Y:  ";
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
		std::cout << tempMultiplied[i].GetR()[1] << std::endl;
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_BLUE);
		std::cout << "Z:  ";
		SetConsoleTextAttribute(hConsole, COLOR_ATTRIBUTE_WHITE);
		std::cout << tempMultiplied[i].GetR()[2] << std::endl << std::endl;
	}
	printf("Joint names depth first:\n");
	std::cout << names[0] << std::endl;
	std::cout << nrOfJoints << std::endl;

	readable_file << nrOfJoints << "\n";


	const char * nameForBinary = names[0].c_str();
	outfile.write((const char*)&nrOfJoints, sizeof(int));
	outfile.write((const char*)nameForBinary, sizeof(char[100]));
	Transform temp_transform;

	
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetS().mData, temp_transform.transform_scale);
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetT().mData, temp_transform.transform_position);
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetR().mData, temp_transform.transform_rotation);

	//COPY_DOUBLE_FLOAT(4, &q, temp_transform.transform_rotationQuaternion);

	// #negate
	//temp_transform.transform_rotation[0] *= -1;
	//temp_transform.transform_rotation[1] *= -1;
	temp_transform.transform_position[2] *= -1;

	outfile.write((const char*)nameForBinary, sizeof(char[100]));
	outfile.write((const char*)&temp_transform, sizeof(Transform));
	outfile.write((const char*)&parent_index_vector[0], sizeof(unsigned int));
	readable_file << names[0] << "\n";
	readable_file << " Rotation:  ";
	readable_file << temp_transform.transform_rotation[0] << "  ";
	readable_file << temp_transform.transform_rotation[1] << "  ";
	readable_file << temp_transform.transform_rotation[2] << " Position: ";
	readable_file << temp_transform.transform_position[0] << "  ";
	readable_file << temp_transform.transform_position[1] << "  ";
	readable_file << temp_transform.transform_position[2] << " Scale: ";
	readable_file << temp_transform.transform_scale[0] << "  ";
	readable_file << temp_transform.transform_scale[1] << "  ";
	readable_file << temp_transform.transform_scale[2] << "  \n";
	readable_file << "Parent index: " << names[parent_index_vector[0]] << "\n";
	for (int i = 1; i < nrOfJoints; i++)
	{
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetR().mData, temp_transform.transform_rotation);
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetS().mData, temp_transform.transform_scale);
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetT().mData, temp_transform.transform_position);

		// #negate
		//temp_transform.transform_rotation[0] *= -1;
		//temp_transform.transform_rotation[1] *= -1;
		//temp_transform.transform_position[2] *= -1;

		readable_file << names[i] << "\n";
		readable_file << " Rotation:  ";
		readable_file << (float)temp_transform.transform_rotation[0] << "  ";
		readable_file << (float)temp_transform.transform_rotation[1] << "  ";
		readable_file << (float)temp_transform.transform_rotation[2] << " Position ";
		readable_file << (float)temp_transform.transform_position[0] << "  ";
		readable_file << (float)temp_transform.transform_position[1] << "  ";
		readable_file << (float)temp_transform.transform_position[2] << " Scale: ";
		readable_file << (float)temp_transform.transform_scale[0] << "  ";
		readable_file << (float)temp_transform.transform_scale[1] << "  ";
		readable_file << (float)temp_transform.transform_scale[2] << "  \n";
		readable_file << "Parent index: " << names[parent_index_vector[i]] << "\n";
		const char * nameForBinary = names[i].c_str();
		outfile.write((const char*)nameForBinary, sizeof(char[100]));
		outfile.write((const char*)&temp_transform, sizeof(Transform));
		int temp = parent_index_vector[i];
		outfile.write((const char*)&temp, sizeof(int));
		std::cout << "namn";
		std::cout << nameForBinary << std::endl;
	}


	outfile.close();
	readable_file.close();

	return true;
}

void Converter::storeChildren(std::vector<std::string>* vector, FbxNode* scene_node)
{
	vector->push_back(scene_node->GetName());

	for (int i = 0; i < scene_node->GetChildCount(); i++)
	{
		storeChildren(vector, scene_node->GetChild(i));

	}

}
void Converter::createAnimationFile(std::vector<std::vector<Transform>> keys)
{

	const char * temp = "Animation";
	std::string filename = std::string(EXPORT_LOCATION) + std::string("ANIMATION") + "_ANIMATION.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_LOCATION) + std::string("ANIMATION") + "_ANIMATION.txt";
	std::ofstream readable_file(readable_file_name);
	int nrOf = keys[0].size();
	outfile.write((const char*)&nrOf, sizeof(unsigned int));
	readable_file << keys[0].size() << "\n";
	Transform temp_transform;
	for (int i = 0; i < keys[0].size(); i++)
	{
		for (int j = 0; j < keys.size(); j++)
		{

			COPY_DOUBLE_FLOAT(3, keys[j][i].transform_position, temp_transform.transform_position);
			COPY_DOUBLE_FLOAT(3, keys[j][i].transform_rotation, temp_transform.transform_rotation);
			COPY_DOUBLE_FLOAT(3, keys[j][i].transform_scale, temp_transform.transform_scale);

			//ZERO_IF_SMALL(temp_transform.transform_rotation[0]);
			//ZERO_IF_SMALL(temp_transform.transform_rotation[1]);
			//ZERO_IF_SMALL(temp_transform.transform_rotation[2]);

			// #negate
			//temp_transform.transform_rotation[0] *= -1;
			//temp_transform.transform_rotation[1] *= -1;
			//temp_transform.transform_position[2] *= -1;

			outfile.write((const char*)&temp_transform.transform_position[0], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_position[1], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_position[2], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_rotation[0], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_rotation[1], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_rotation[2], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_scale[0], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_scale[1], sizeof(float));
			outfile.write((const char*)&temp_transform.transform_scale[2], sizeof(float));
			readable_file << "Translation: " << "X:\t" << (float)keys[j][i].transform_position[0] << " Y:\t" << (float)keys[j][i].transform_position[1] << " Z:\t" << (float)keys[j][i].transform_position[2] << "\n";
			readable_file << "Rotation: " << "X:\t" << keys[j][i].transform_rotation[0] << " Y:\t" << keys[j][i].transform_rotation[1] << " Z:\t" << keys[j][i].transform_rotation[2] << "\n";
			readable_file << "Scale: " << "X:\t" << (float)keys[j][i].transform_scale[0] << " Y:\t" << (float)keys[j][i].transform_scale[1] << " Z:\t" << (float)keys[j][i].transform_scale[2] << "\n";
		}

	}


	outfile.close();
	readable_file.close();

}
