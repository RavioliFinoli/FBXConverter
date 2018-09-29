#include "Manager.h"
#include <windows.h>
#define EXPORT_DIRECTORY "C:/Repos/RipTag/Assets/"
#define COPY_DOUBLE_FLOAT(COUNT, PTR1, PTR2) \
{ \
int counter = -1; while(counter++ < COUNT-1) PTR2[counter] = (float)PTR1[counter];  \
};

Converter::Converter(const char* fileName)
{
	sdk_Manager = FbxManager::Create();
	sdk_IOSettings = FbxIOSettings::Create(sdk_Manager, IOSROOT);
	sdk_Manager->SetIOSettings(sdk_IOSettings);
	sdk_importer = FbxImporter::Create(sdk_Manager, "");
	sdk_scene = FbxScene::Create(sdk_Manager, "");

	FbxAxisSystem axisSystem;
	axisSystem = FbxAxisSystem::DirectX;
	axisSystem.ConvertScene(sdk_scene);
	int shit = 1;
	auto poop = sdk_scene->GetGlobalSettings().GetAxisSystem().GetCoorSystem() ;


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
	if(animation_key_vectors.size()>0)
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
		{
			createMeshFiles(scene_mesh->GetPolygonCount() * 3, Positions, normals, UV, node_name)
				? printf("Binary mesh file created\n")
				: printf("Could not create file");
		}
		else
		{

			FbxSkin* mesh_skin = (FbxSkin*)(scene_mesh->GetDeformer(0, FbxDeformer::eSkin));

			if (mesh_skin)
				createAnimatedMeshFile(scene_node, scene_mesh->GetPolygonCount() * 3, Positions, normals, UV, node_name, polygon_vertices_indices) ? printf("Animated Mesh file created!\n") : printf("Animated Mesh file could not be created");

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
				std::cout << "HELLO " << j << std::endl;
				skin_cluster = mesh_skin->GetCluster(j);
				std::string connectedJointName = skin_cluster->GetLink()->GetName();
				std::string parent_name = skin_cluster->GetLink()->GetParent()->GetName();

				FbxAMatrix transformation;
				FbxAMatrix linked_Transformation;

				///OLD
				skin_cluster->GetTransformMatrix(transformation);
				skin_cluster->GetTransformLinkMatrix(linked_Transformation);
				linked_Transformation.SetR(skin_cluster->GetLink()->EvaluateLocalRotation());
				FbxAMatrix inverse_bind_pose = linked_Transformation.Inverse() * transformation * transformation_matrix;

				//FbxAMatrix transformMatrix;
				//FbxAMatrix transformLinkMatrix;
				//FbxAMatrix globalBindposeInverseMatrix;
				//skin_cluster->GetTransformMatrix(transformMatrix); // The transformation of the mesh at binding time
				//skin_cluster->GetTransformLinkMatrix(transformLinkMatrix); // The transformation of the cluster(joint) at binding time from joint space to world space
				//FbxAMatrix inverse_bind_pose = transformLinkMatrix.Inverse() * transformMatrix;
				// Update the information in mSkeleton
				//
				//inverse_bind_pose = linked_Transformation;
			    //inverse_bind_pose.SetR(skin_cluster->GetLink()->EvaluateLocalRotation());
				//if (j == 0)
				//{
				//	inverse_bind_pose.SetIdentity();
				//}
				//else
				//{
				//	inverse_bind_pose *= skin_cluster->GetLink()->GetParent()->EvaluateLocalTransform().Inverse();
				//}
			//	inverse_bind_pose = skin_cluster->GetLink()->EvaluateGlobalTransform();
				
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
				std::cout << connectedJointName << std::endl;
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
						auto ControlPointIndexCount = skin_cluster->GetControlPointIndicesCount();
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
		// #createSkeletonFile_call
		createSkeletonFile(skeleton_joint_names, skeleton_joints_temp_vector, skeleton_joint_names.size(), skeleton_joint_parent_names);
		int nrOfVerts = nrOfVertices;
		const char * temp = "_Mesh";
		std::string filename = std::string(EXPORT_DIRECTORY) + std::string(node_name) + "_ANIMATION_Mesh.bin";
		std::ofstream outfile(filename, std::ofstream::binary);
		std::string readable_file_name = std::string(EXPORT_DIRECTORY) + std::string(node_name) + "_ANIMATION_Mesh.txt";
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


bool Converter::createMeshFiles(int nrOfVertices, std::vector<FbxVector4>Positions, std::vector<FbxVector4>normals, std::vector<FbxVector2>UV, const char* node_name)
{

	int nrOfVerts = nrOfVertices;
	const char * temp = "_Mesh";
	std::string filename = std::string(node_name) + "_Mesh.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(node_name) + "_Mesh.txt";
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

bool Converter::getSceneAnimationData(FbxNode * scene_node, std::vector<std::vector<Transform>> &transform_vector)
{
	FbxScene * scene = sdk_scene;
	FbxAnimStack* anim_stack = scene->GetCurrentAnimationStack();
	FbxAnimLayer* anim_layer = anim_stack->GetMember<FbxAnimLayer>();


	FbxAnimCurve* anim_curve_rotY = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);

	if (anim_curve_rotY)
	{
		std::vector<Transform> transform_temp;


		std::vector<FbxVector4> temp_translations;
		std::vector<FbxVector4> temp_rotations;
		std::vector<FbxVector4> temp_scaling;
		FbxAnimCurve* anim_curve_rotZ = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);
		FbxAnimCurve* anim_curve_rotX = scene_node->LclRotation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);

		FbxAnimCurve* anim_curve_translateY = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);
		FbxAnimCurve* anim_curve_translateX = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);
		FbxAnimCurve* anim_curve_translateZ = scene_node->LclTranslation.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);

		FbxAnimCurve* anim_curve_scaleY = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Y);
		FbxAnimCurve* anim_curve_scaleX = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_X);
		FbxAnimCurve* anim_curve_scaleZ = scene_node->LclScaling.GetCurve(anim_layer, FBXSDK_CURVENODE_COMPONENT_Z);


		std::vector<FbxAMatrix> matrices;
		FbxAMatrix matrix;


		for (int i = 0; i < anim_curve_rotY->KeyGetCount(); i++)
		{
			FbxVector4 translation((double)anim_curve_translateX->KeyGetValue(i), (double)anim_curve_translateY->KeyGetValue(i), (double)anim_curve_translateZ->KeyGetValue(i), 1);
			FbxVector4 rotation((double)anim_curve_rotX->KeyGetValue(i), (double)anim_curve_rotY->KeyGetValue(i), (double)anim_curve_rotZ->KeyGetValue(i), 0);
			FbxVector4 scale((double)anim_curve_scaleX->KeyGetValue(i), (double)anim_curve_scaleY->KeyGetValue(i), (double)anim_curve_scaleZ->KeyGetValue(i), 0);

			Transform transform_to_push_back;

			transform_to_push_back.transform_position[0] = (float)anim_curve_translateX->KeyGetValue(i);
			transform_to_push_back.transform_position[1] = (float)anim_curve_translateY->KeyGetValue(i);
			transform_to_push_back.transform_position[2] = (float)anim_curve_translateZ->KeyGetValue(i);
			transform_to_push_back.transform_rotation[0] = (float)anim_curve_rotX->KeyGetValue(i);
			transform_to_push_back.transform_rotation[1] = (float)anim_curve_rotY->KeyGetValue(i);
			transform_to_push_back.transform_rotation[2] = (float)anim_curve_rotZ->KeyGetValue(i);
			transform_to_push_back.transform_scale[0] = (float)anim_curve_scaleX->KeyGetValue(i);
			transform_to_push_back.transform_scale[1] = (float)anim_curve_scaleY->KeyGetValue(i);
			transform_to_push_back.transform_scale[2] = (float)anim_curve_scaleZ->KeyGetValue(i);

			transform_temp.push_back(transform_to_push_back);
			matrix.SetTRS(translation, rotation, scale);

		}
		transform_vector.push_back(transform_temp);
	}


	int yo = scene_node->GetChildCount();
	for (int i = 0; i < scene_node->GetChildCount(); i++)
	{
		getSceneAnimationData(scene_node->GetChild(i), transform_vector);
	}

	return true;
}

void PrintMatrix(FbxAMatrix& matrix)
{
	auto mat = matrix.Double44();
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			std::cout << mat[col][row] << " ";
		}
		std::cout << std::endl;
	}
}

bool Converter::createSkeletonFile(std::vector<std::string> names, std::vector<FbxAMatrix> joint_matrices, int nrOfJoints, std::vector<FbxNode*> parent_name_list)
{
	const char * temp = "_Skeleton";
	std::string filename = std::string(EXPORT_DIRECTORY) + std::string(names[0]) + "_Skeleton.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_DIRECTORY) + std::string(names[0]) + "_Skeleton.txt";
	std::ofstream readable_file(readable_file_name);

	std::cout << "Found Skeleton:  " << names[0] << "_Skeleton! Number of joints in skeleton: " << nrOfJoints << std::endl;
	std::vector<int> parent_index_vector(nrOfJoints);

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
			if(names[j] == tempParentNames[i])
				parent_index_vector[i] = j;
		}
	}

	std::vector<FbxAMatrix> tempMultiplied;
	for (int i = 0; i < joint_matrices.size(); i++)
	{
		FbxAMatrix temp = joint_matrices[i];
		tempMultiplied.push_back(temp);
	}
	
	
	std::cout << "Matrices: " << std::endl;
	for (int i = 0; i < tempMultiplied.size(); i++)
	{
		//std::cout << "ORIENTATION!:  ";
		//std::cout << "X:  " << tempMultiplied[i].GetR()[0] << " Y: " << tempMultiplied[i].GetR()[1] << " Z: " << tempMultiplied[i].GetR()[2] << std::endl;
		//std::cout << "tX:  " << tempMultiplied[i].GetT()[0] << " tY: " << tempMultiplied[i].GetT()[1] << " tZ: " << tempMultiplied[i].GetT()[2] << std::endl;
		PrintMatrix(tempMultiplied[i]);
	}
	printf("Joint names depth first:\n");
	std::cout << names[0] << std::endl;
	std::cout << nrOfJoints << std::endl;

	readable_file << nrOfJoints << "\n";


	const char * nameForBinary = names[0].c_str();
	outfile.write((const char*)&nrOfJoints, sizeof(int));
	outfile.write((const char*)nameForBinary, sizeof(char[100]));
	Transform temp_transform;
	
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetS(), temp_transform.transform_scale);
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetT(), temp_transform.transform_position);
	COPY_DOUBLE_FLOAT(3, joint_matrices[0].GetR(), temp_transform.transform_rotation);

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
		
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetS(), temp_transform.transform_scale);
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetT(), temp_transform.transform_position);
		COPY_DOUBLE_FLOAT(3, joint_matrices[i].GetR(), temp_transform.transform_rotation);
		readable_file << names[i] << "\n";
		readable_file << " Rotation:  ";
		
		//TODO
		{
			//{
			//	temp_transform.transform_rotation[0] = 0.0;
			//	temp_transform.transform_rotation[1] = 0.0;
			//	temp_transform.transform_rotation[2] = 0.0;
			//}

			//if (i == 2)
			//{
			//	temp_transform.transform_position[0] = -2;
			//	temp_transform.transform_position[1] = 0;
			//}
		}

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
	std::string filename = std::string(EXPORT_DIRECTORY) + std::string("ANIMATION") + "_ANIMATION.bin";
	std::ofstream outfile(filename, std::ofstream::binary);
	std::string readable_file_name = std::string(EXPORT_DIRECTORY) + std::string("ANIMATION") + "_ANIMATION.txt";
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
			readable_file << "Rotation: " << "X:\t" << (float)keys[j][i].transform_rotation[0] << " Y:\t" << (float)keys[j][i].transform_rotation[1] << " Z:\t" << (float)keys[j][i].transform_rotation[2] << "\n";
			readable_file << "Scale: " << "X:\t" << (float)keys[j][i].transform_scale[0] << " Y:\t" << (float)keys[j][i].transform_scale[1] << " Z:\t" << (float)keys[j][i].transform_scale[2] << "\n";
		}

	}


	outfile.close();
	readable_file.close();

}
//void Converter::getCamera(FbxNode * scene_node)
//{
//	FbxCamera * cam = scene_node->GetCamera();
//	Camera camera;
//
//	if (cam)
//	{
//		
//		for (int i = 0; i < 100; i++)
//		{
//			camera.cam_name[i] = cam->GetName()[i];
//			std::cout << camera.cam_name[i];
//		}
//		std::cout << std::endl;
//
//		FbxDouble3 translation = scene_node->LclTranslation.Get();
//		FbxDouble3 rotation = scene_node->LclRotation.Get();
//		FbxDouble3 scaling = scene_node->LclScaling.Get();
//
//		camera.cam_transform.transform_position[0] = (float)translation[0];
//		camera.cam_transform.transform_position[1] = (float)translation[1];
//		camera.cam_transform.transform_position[2] = (float)translation[2];		camera.cam_transform.transform_rotation[0] = (float)rotation[0];
//		camera.cam_transform.transform_rotation[1] = (float)rotation[1];
//		camera.cam_transform.transform_rotation[2] = (float)rotation[2];
//
//		camera.cam_transform.transform_scale[0] = (float)scaling[0];
//		camera.cam_transform.transform_scale[1] = (float)scaling[1];
//		camera.cam_transform.transform_scale[2] = (float)scaling[2];
//
//		std::cout << "Cam: Translation: X: " << camera.cam_transform.transform_position[0] << " Y: " << camera.cam_transform.transform_position[1] << " Z: " << camera.cam_transform.transform_position[2] <<  std::endl;
//		std::cout << "Cam: Rotation: X: " << camera.cam_transform.transform_rotation[0] << " Y: " << camera.cam_transform.transform_rotation[1] << " Z: " << camera.cam_transform.transform_rotation[2] << std::endl;
//		std::cout << "Cam: Scale: X: " << camera.cam_transform.transform_scale[0] << " Y: " << camera.cam_transform.transform_scale[1] << " Z: " << camera.cam_transform.transform_scale[2] << std::endl;
//		
//		camera.cam_FOV = cam->FieldOfView.Get();
//		
//		
//		cameras.push_back(camera);
//
//	}
//
//	for (int i = 0; i < scene_node->GetChildCount(); i++)
//	{
//		getCamera(scene_node->GetChild(i));
//	}
//
//}
//
//bool Converter::createCameraFiles(std::vector<Camera> camera)
//{
//	const char * temp = "_Camera";
//	std::string filename = std::string("CAMERAS") + "_Camera.bin";
//	std::ofstream outfile(filename, std::ofstream::binary);
//	std::string readable_file_name = std::string("CAMERAS") + "_Camera.txt";
//	std::ofstream readable_file(readable_file_name);
//	int nrOfCameras = camera.size();
//	readable_file << nrOfCameras << "\n";
//	outfile.write((const char*)&nrOfCameras, sizeof(unsigned int));
//	for (int i = 0; i < camera.size(); i++)
//	{
//	
//
//	if (!outfile)
//	{
//		return false;
//	}
//	if (!readable_file)
//	{
//		return false;
//	}
//
//	readable_file.write((const char*)camera[i].cam_name, sizeof(char[100]));
//	readable_file << "\n";
//
//	readable_file << "Position:  X " << camera[i].cam_transform.transform_position[0] << " Y " << camera[i].cam_transform.transform_position[1] << " Z " << camera[i].cam_transform.transform_position[2] << "\n"
//		"Rotation:  X " << camera[i].cam_transform.transform_rotation[0] << " Y " << camera[i].cam_transform.transform_rotation[1] << " Y " << camera[i].cam_transform.transform_rotation[2] << "\n"
//		"Scale:  X " << camera[i].cam_transform.transform_scale[0] << " Y " << camera[i].cam_transform.transform_scale[1] << " Z " << camera[i].cam_transform.transform_scale[2] << "\n";
//
//	outfile.write((const char*)camera[i].cam_name, sizeof(char[100]));
//
//
//	outfile.write((const char*)&camera[i].cam_transform.transform_position[0], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_position[1], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_position[2], sizeof(float));
//
//
//	outfile.write((const char*)&camera[i].cam_transform.transform_rotation[0], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_rotation[1], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_rotation[2], sizeof(float));
//
//	outfile.write((const char*)&camera[i].cam_transform.transform_scale[0], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_scale[1], sizeof(float));
//	outfile.write((const char*)&camera[i].cam_transform.transform_scale[2], sizeof(float));
//
//
//	outfile.write((const char*)&camera[i].cam_FOV, sizeof(float));
//
//
//	
//	}
//	readable_file.close();
//	outfile.close();
//
//	return true;
//}
//
//void Converter::getLight(FbxNode * scene_node)
//{
//	FbxLight * light = scene_node->GetLight();
//
//
//	if (light) {
//		Light lightStruct;
//		for(int i = 0; i < 100; i++)
//		lightStruct.light_name[i] = scene_node->GetName()[i];
//		//Transform
//		FbxDouble3 translation = scene_node->LclTranslation.Get();
//		FbxDouble3 rotation = scene_node->LclRotation.Get();
//		FbxDouble3 scaling = scene_node->LclScaling.Get();
//
//		lightStruct.light_transform.transform_position[0] = (float)translation.mData[0];
//		lightStruct.light_transform.transform_position[1] = (float)translation.mData[1];
//		lightStruct.light_transform.transform_position[2] = (float)translation.mData[2];
//
//		lightStruct.light_transform.transform_rotation[0] = (float)rotation.mData[0];
//		lightStruct.light_transform.transform_rotation[1] = (float)rotation.mData[1];
//		lightStruct.light_transform.transform_rotation[2] = (float)rotation.mData[2];
//
//		lightStruct.light_transform.transform_scale[0] = (float)scaling.mData[0];
//		lightStruct.light_transform.transform_scale[1] = (float)scaling.mData[1];
//		lightStruct.light_transform.transform_scale[2] = (float)scaling.mData[2];
//
//		//Light
//		lightStruct.light_color[0] = (float)light->Color.Get().mData[0];
//		lightStruct.light_color[1] = (float)light->Color.Get().mData[1];
//		lightStruct.light_color[2] = (float)light->Color.Get().mData[2];
//		lightStruct.light_color[3] = (float)light->Intensity.Get() / 100.0;
//
//
//		
//		lights.push_back(lightStruct);
//		std::cout << light->GetName() << std::endl;
//	}
//
//	for (int i = 0; i < scene_node->GetChildCount(); i++)
//	{
//		getLight(scene_node->GetChild(i));
//	}
//
//}
//
//bool Converter::createLightFile(std::vector<Light> lights)
//{
//
//	const char * temp = "_Light";
//	const char* node_name = "Lights file";
//	std::string filename = std::string("LIGHTFILE") + "_Light.bin";
//	std::ofstream outfile(filename, std::ofstream::binary);
//	std::string readable_file_name = std::string(node_name) + "_Light.txt";
//	std::ofstream readable_file(readable_file_name);
//
//	int nrOfLights = lights.size();
//	outfile.write((const char*)&nrOfLights, sizeof(unsigned int));
//	readable_file << nrOfLights;
//	if (!outfile)
//	{
//		printf("No file could be opened. Manager.cpp line 958");
//		return false;
//	}
//	if (!readable_file)
//	{
//		printf("No file could be opened. Manager.cpp line 958");
//		return false;
//	}
//
//	for (int i = 0; i < lights.size(); i++)
//	{
//			readable_file.write((const char*)lights[i].light_name, sizeof(char[100]));
//			readable_file << "\n";
//
//		readable_file << "Position:  X " << lights[i].light_transform.transform_position[0] << " Y " << lights[i].light_transform.transform_position[1] << " Z " << lights[i].light_transform.transform_position[2] << "\n"
//		"Rotation:  X " << lights[i].light_transform.transform_rotation[0] << " Y " << lights[i].light_transform.transform_rotation[1] << " Z " << lights[i].light_transform.transform_rotation[2] << "\n"
//		"Scale:  X " << lights[i].light_transform.transform_scale[0] << " Y " << lights[i].light_transform.transform_scale[1] << " Z " << lights[i].light_transform.transform_scale[2] << "\n"
//		"Color:  R " << lights[i].light_color[0] << " G " << lights[i].light_color[1] << " B " << lights[i].light_color[2] << " Intensity " << lights[i].light_color[3] << "\n";
//		outfile.write((const char*)lights[i].light_name, sizeof(const char[100]));
//
//		outfile.write((const char*)&lights[i].light_transform.transform_position[0], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_position[1], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_position[2], sizeof(float));
//
//		outfile.write((const char*)&lights[i].light_transform.transform_rotation[0], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_rotation[1], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_rotation[2], sizeof(float));
//
//		outfile.write((const char*)&lights[i].light_transform.transform_scale[0], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_scale[1], sizeof(float));
//		outfile.write((const char*)&lights[i].light_transform.transform_scale[2], sizeof(float));
//
//		outfile.write((const char*)&lights[i].light_color[0], sizeof(float));
//		outfile.write((const char*)&lights[i].light_color[1], sizeof(float));
//		outfile.write((const char*)&lights[i].light_color[2], sizeof(float));
//		outfile.write((const char*)&lights[i].light_color[3], sizeof(float));
//	}
//
//	readable_file.close();
//	outfile.close();
//
//	return true;
//}
//
//bool Converter::createBlendShapeFiles(FbxMesh * mesh_node, FbxAnimLayer* anim_layer)
//{
//
//	std::vector<std::vector<Vertex>> targets;
//
//	//base shape
//	std::vector<Vertex> base;
//	for (int h = 0; h < mesh_node->GetControlPointsCount(); h++)
//	{
//		Vertex vertex;
//		vertex.vertex_position[0] = mesh_node->GetControlPointAt(h).mData[0];
//		vertex.vertex_position[1] = mesh_node->GetControlPointAt(h).mData[1];
//		vertex.vertex_position[2] = mesh_node->GetControlPointAt(h).mData[2];
//
//
//		FbxLayerElementArrayTemplate<FbxVector4>* nrms;
//		mesh_node->GetNormals(&nrms);
//		vertex.vertex_normal[0] = nrms->GetAt(h).mData[0];
//		vertex.vertex_normal[1] = nrms->GetAt(h).mData[1];
//		vertex.vertex_normal[2] = nrms->GetAt(h).mData[2];
//
//		vertex.vertex_UVCoord[0] = 0;
//		vertex.vertex_UVCoord[1] = 0;
//		base.push_back(vertex);
//	}
//	targets.push_back(base);
//
//
//	//other shapes
//	int blendCount = mesh_node->GetDeformerCount(FbxDeformer::eBlendShape);
//	for (int h = 0; h < blendCount; h++)
//	{
//		FbxBlendShape* mesh_blend_shape = (FbxBlendShape*)(mesh_node->GetDeformer(0, FbxDeformer::eBlendShape));
//		if (mesh_blend_shape) {
//			int channelCount = mesh_blend_shape->GetBlendShapeChannelCount();
//			for (int i = 0; i < channelCount; i++)
//			{
//				FbxBlendShapeChannel *blendChannel = mesh_blend_shape->GetBlendShapeChannel(i);
//				if (blendChannel) {
//					FbxAnimCurve* animCurve = mesh_node->GetShapeChannel(h, i, anim_layer);
//					if (!animCurve) continue;
//
//					int keyCount = animCurve->KeyGetCount();
//					for (int j = 0; j < keyCount; j++)
//					{
//						FbxAnimCurveKey key = animCurve->KeyGet(j);
//						int value = key.GetValue();
//
//
//						FbxShape* shape = blendChannel->GetTargetShape(j);
//						if (shape) {
//							int ctrlCount = shape->GetControlPointsCount();
//							FbxVector4* controlPointsArr = shape->GetControlPoints();
//
//							FbxLayerElementArrayTemplate<FbxVector4>* nrms;
//							shape->GetNormals(&nrms);
//
//
//							std::vector<Vertex> othershape;
//							for (int h = 0; h < mesh_node->GetControlPointsCount(); h++)
//							{
//								Vertex vertex;
//								vertex.vertex_position[0] = shape->GetControlPointAt(h).mData[0];
//								vertex.vertex_position[1] = shape->GetControlPointAt(h).mData[1];
//								vertex.vertex_position[2] = shape->GetControlPointAt(h).mData[2];
//
//
//								FbxLayerElementArrayTemplate<FbxVector4>* nrms;
//								shape->GetNormals(&nrms);
//								vertex.vertex_normal[0] = nrms->GetAt(h).mData[0];
//								vertex.vertex_normal[1] = nrms->GetAt(h).mData[1];
//								vertex.vertex_normal[2] = nrms->GetAt(h).mData[2];
//
//								vertex.vertex_UVCoord[0] = 0;
//								vertex.vertex_UVCoord[1] = 0;
//								othershape.push_back(vertex);
//							}
//							targets.push_back(othershape);
//
//
//
//						}
//					}
//
//
//				}
//			}
//		}
//
//	}
//
//
//
//	const char * temp = "_BlendShape";
//	const char* node_name = mesh_node->GetName();
//	std::string filename = std::string(node_name) + "_BlendShape.bin";
//	std::ofstream outfile(filename, std::ofstream::binary);
//	std::string readable_file_name = std::string(node_name) + "_BlendShape.txt";
//	std::ofstream readable_file(readable_file_name);
//
//	if (!outfile)
//	{
//		return false;
//	}
//	if (!readable_file)
//	{
//		return false;
//	}
//
//
//	//Write files
//	readable_file.write((const char*)node_name, sizeof(char[100]));
//	readable_file << "\n";
//
//	for (int h = 0; h < targets.size(); h++)
//	{
//		readable_file << "Shape " << h << std::endl;
//
//		for (int index = 0; index < targets[h].size(); index++)
//		{
//			readable_file << "Position x" << targets[h][index].vertex_position[0] << " y" << targets[h][index].vertex_position[1] << " z" << targets[h][index].vertex_position[2] << std::endl;
//			readable_file << "uv u" << targets[h][index].vertex_UVCoord[0] << " v" << targets[h][index].vertex_UVCoord[1] << std::endl;
//			readable_file << "normal x" << targets[h][index].vertex_normal[0] << " y" << targets[h][index].vertex_normal[1] << " z" << targets[h][index].vertex_normal[2] << std::endl;
//
//		}
//
//	}
//
//	//nrOfShapes
//	int shapes = targets.size();
//	outfile.write((const char*)node_name, sizeof(char[100]));
//	int verts = targets[0].size() * shapes;
//	outfile.write((const char*)&verts, sizeof(int));
//	for (int h = 0; h < targets.size(); h++)
//	{
//
//		//nrOfVertices
//		for (int index = 0; index < targets[h].size(); index++)
//		{
//			outfile.write((const char*)&targets[h][index].vertex_position[0], sizeof(float));
//			outfile.write((const char*)&targets[h][index].vertex_position[1], sizeof(float));
//			outfile.write((const char*)&targets[h][index].vertex_position[2], sizeof(float));
//
//			outfile.write((const char*)&targets[h][index].vertex_UVCoord[0], sizeof(float));
//			outfile.write((const char*)&targets[h][index].vertex_UVCoord[1], sizeof(float));
//
//			outfile.write((const char*)&targets[h][index].vertex_normal[0], sizeof(float));
//			outfile.write((const char*)&targets[h][index].vertex_normal[1], sizeof(float));
//			outfile.write((const char*)&targets[h][index].vertex_normal[2], sizeof(float));
//
//
//
//		}
//
//	}
//
//	readable_file.close();
//	outfile.close();
//
//	std::cout << "Done" << std::endl;
//
//	return false;
//}
//
//void Converter::getCustomAttributes(FbxNode* scene_node)
//{
//#pragma warning(disable:4996)
//	FbxProperty property = scene_node->GetFirstProperty();
//	int i = 0;
//	std::string file;
//	CustomAttributes customAttributes;
//
//	while (property.IsValid())
//	{
//		if (i > 70)
//		{
//			switch (property.GetPropertyDataType().GetType())
//			{
//			case eFbxBool:
//				customAttributes.customBool = property.Get<FbxBool>();
//				break;
//
//			case eFbxDouble:
//				customAttributes.customFloat = property.Get<FbxFloat>();
//				break;
//
//			case eFbxInt:
//				customAttributes.customInt = property.Get<FbxInt>();
//				break;
//
//			case eFbxEnum:
//				customAttributes.customEnum = property.Get<FbxEnum>();
//				break;
//
//			case eFbxDouble3:
//				customAttributes.customVector[0] = property.Get<FbxDouble3>()[0];
//				customAttributes.customVector[1] = property.Get<FbxDouble3>()[1];
//				customAttributes.customVector[2] = property.Get<FbxDouble3>()[2];
//				break;
//
//			case eFbxString:
//				std::strcpy(customAttributes.customString, property.Get<FbxString>());
//				break;
//
//			default:
//				std::cout << "def \n";
//				break;
//			}
//
//
//			file += "Mesh: " + std::string(scene_node->GetName()) + "\n" + "Custom Attribut " + std::to_string(i) + "\n" +
//				"Display Name: " + std::string(property.GetLabel()) + "\n" + "Type: " + std::string(property.GetPropertyDataType().GetName()) + "\n";
//
//			if (property.GetPropertyDataType().GetType() == eFbxDouble3)
//			{
//				FbxDouble3 vector = property.Get<FbxDouble3>();
//				file += "Vector Value X: " + std::to_string(vector[0]) + "\n";
//				file += "Vector Value y: " + std::to_string(vector[1]) + "\n";
//				file += "Vector Value z: " + std::to_string(vector[2]) + "\n";
//			}
//			else
//			{
//				file += "Value: " + property.Get<FbxString>() + "\n";
//			}
//
//			file += "\n\n";
//		}
//		i++;
//		property = scene_node->GetNextProperty(property);
//	}
//	std::cout << file;
//
//	for (int i = 0; i < scene_node->GetChildCount(); i++)
//	{
//		getCustomAttributes(scene_node->GetChild(i));
//	}
//	customAttributes.nrOfCustomAttributes = (i - 70);
//	createCustomAttributesFiles(scene_node, file, customAttributes);
//
//	return;
//}
//bool Converter::createCustomAttributesFiles(FbxNode* scene_node, std::string data, CustomAttributes customAttributes)
//{
//	const char * temp = "_CustomAttributes";
//	const char * node_name = scene_node->GetName();
//	std::string filename = std::string(node_name) + "_CustomAttributes.bin";
//	std::ofstream outfile(filename, std::ofstream::binary);
//	std::string readable_file_name = std::string(node_name) + "_CustomAttributes.txt";
//	std::ofstream readable_file(readable_file_name);
//
//	if (!outfile)
//	{
//		return false;
//	}
//
//	if (!readable_file)
//	{
//		return false;
//	}
//
//	readable_file << data;
//
//	outfile.write((const char*)&customAttributes.customBool, sizeof(bool));
//	outfile.write((const char*)&customAttributes.customFloat, sizeof(float));
//	outfile.write((const char*)&customAttributes.customInt, sizeof(int));
//	outfile.write((const char*)&customAttributes.customEnum, sizeof(int));
//	outfile.write((const char*)&customAttributes.customVector, sizeof(float[3]));
//	outfile.write((const char*)&customAttributes.customString, sizeof(char[128]));
//
//
//	readable_file.close();
//	outfile.close();
//
//	return true;
//}
//
//void Converter::createMaterialsFile(FbxNode* scene_node)
//{
//
//	if (!scene_node)
//	{
//		std::cout << "Error in printMaterials \n";
//		return;
//	}
//	else
//	{
//		MaterialHeader MatHead;
//		std::vector<Material> MatVector;
//		FbxSurfaceMaterial* tempMaterial;
//		int nrOfChilds = scene_node->GetChildCount();
//		FbxPropertyT<FbxDouble3> Double3Variable;
//		FbxPropertyT<FbxDouble> Double1Variable;
//		FbxPropertyT<FbxCharPtr> CharPointerVariable;
//		FbxColor ColorVariable;
//		int nrOfTextures = 0, nrOfMaterials = 0;
//
//		
//			const char* bumpName = nullptr;
//			const char* TextureName = nullptr;
//
//			//Getting the information about FBX.
//			FbxGeometry *scene_mesh = scene_node->GetMesh();
//			
//				FbxNode* TempNode = scene_node;
//
//				std::string filename = (std::string)TempNode->GetName() + "_Material.bin";
//				std::ofstream outfile(filename, std::ofstream::binary);
//
//				std::string readable_file_name = (std::string)TempNode->GetName() + "_Material.txt";
//				std::ofstream readable_file(readable_file_name);
//
//
//				printf("\nMesh found: {%s}, It has [%d] material(s)\n\n", TempNode->GetName(), TempNode->GetMaterialCount());
//				nrOfMaterials = TempNode->GetMaterialCount();
//				nrOfTextures = sdk_scene->GetTextureCount();
//
//				MatHead.material_nrOfMaterials = nrOfMaterials;
//				MatHead.material_nrOfTextures = nrOfTextures;
//
//				readable_file << "NrOfMaterials: " << nrOfMaterials << " in Mesh.\n";
//				readable_file << "NrOfTextures: " << nrOfTextures << " in scene.\n";
//				outfile.write((const char*)&nrOfMaterials, sizeof(int));
//				outfile.write((const char*)&nrOfTextures, sizeof(int));
//
//				std::cout << "NrOfMaterials: " << nrOfMaterials << " in Mesh.\n";
//				std::cout << "NrOfTextures: " << nrOfTextures << " in scene.\n";
//
//				//MATERIAL
//				for (int i = 0; i < nrOfMaterials; i++)
//				{
//					FbxSurfaceMaterial *tempMaterial = TempNode->GetMaterial(i);
//					Material NewMaterial = { 0,0,0,0,0,0,0 };
//
//					if (tempMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
//					{
//						std::cout << "\nMaterial(" << i << ")" << "is an PHONG" << " in scene.\n";
//						std::cout << "\nThis material is holding these variables:\n";
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->AmbientFactor;
//						std::cout << "AmbientFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.AmbientFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Ambient;
//						std::cout << "AmbientColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.AmbientColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.AmbientColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.AmbientColor[2] = (float)Double3Variable.Get()[2];
//
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->DiffuseFactor;
//						std::cout << "DiffuseFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.DiffuseFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Diffuse;
//						std::cout << "DiffuseColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.DiffuseColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.DiffuseColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.DiffuseColor[2] = (float)Double3Variable.Get()[2];
//
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->EmissiveFactor;
//						std::cout << "EmissiveFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.EmissiveFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Emissive;
//						std::cout << "EmissiveColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.EmissiveColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.EmissiveColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.EmissiveColor[2] = (float)Double3Variable.Get()[2];
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->TransparencyFactor;
//						std::cout << "Transparency(Factor): " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.Transparency = (float)Double1Variable.Get();
//
//						//Phong Only
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->SpecularFactor;
//						std::cout << "SpecularFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.SpecularFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Specular;
//						std::cout << "SpecularColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.SpecularColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.SpecularColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.SpecularColor[2] = (float)Double3Variable.Get()[2];
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->Shininess;
//						std::cout << "Shininess(Cosine Power): " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.Shininess = (float)Double1Variable.Get();
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->ReflectionFactor;
//						std::cout << "Reflection: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.Reflection = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Reflection;
//						std::cout << "ReflectionColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.ReflectionColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.ReflectionColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.ReflectionColor[2] = (float)Double3Variable.Get()[2];
//
//						for (unsigned int j = 0; j < nrOfTextures; j++)
//						{
//							FbxProperty propertyS;
//							propertyS = tempMaterial->FindProperty(FbxSurfacePhong::sBump);
//							FbxTexture *testTextureBump = propertyS.GetSrcObject<FbxTexture>();
//
//							if (testTextureBump)
//							{
//								FbxFileTexture* TestFileTexture = FbxCast<FbxFileTexture>(testTextureBump);
//								bumpName = TestFileTexture->GetRelativeFileName();
//								const char* temp = TestFileTexture->GetFileName();
//								for (unsigned int d = 0; d < strlen(temp); d++)
//								{
//									NewMaterial.BumpPath[d] = temp[d];
//								}
//							}
//							else
//							{
//								const char* temp = "Empty.";
//								for (unsigned int o = 0; o < strlen(temp); o++)
//								{
//									NewMaterial.BumpPath[o] = temp[o];
//								}
//							}
//
//							propertyS = tempMaterial->FindProperty(FbxSurfacePhong::sDiffuse);
//							FbxTexture *testTexture = propertyS.GetSrcObject<FbxTexture>();
//
//							if (testTexture)
//							{
//								FbxFileTexture* TestFileTexture = FbxCast<FbxFileTexture>(testTexture);
//								TextureName = TestFileTexture->GetRelativeFileName();
//								const char* temp = TestFileTexture->GetFileName();
//								for (unsigned int d = 0; d < strlen(temp); d++)
//								{
//									NewMaterial.TexturePath[d] = temp[d];
//								}
//							}
//							else
//							{
//								const char* temp = "Empty.";
//								for (unsigned int o = 0; o < strlen(temp); o++)
//								{
//									NewMaterial.TexturePath[o] = temp[o];
//								}
//							}
//						}
//
//						std::cout << std::endl;
//						MatVector.push_back(NewMaterial);
//					}
//					else if (tempMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
//					{
//						std::cout << "\nMaterial(" << i << ")" << "is an LAMBERT" << " in scene.\n";
//						std::cout << "\nThis material is holding these variables:\n";
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->AmbientFactor;
//						std::cout << "AmbientFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.AmbientFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Ambient;
//						std::cout << "AmbientColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.AmbientColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.AmbientColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.AmbientColor[2] = (float)Double3Variable.Get()[2];
//
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->DiffuseFactor;
//						std::cout << "DiffuseFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.DiffuseFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Diffuse;
//						std::cout << "DiffuseColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.DiffuseColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.DiffuseColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.DiffuseColor[2] = (float)Double3Variable.Get()[2];
//
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->EmissiveFactor;
//						std::cout << "EmissiveFactor: " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.EmissiveFactor = (float)Double1Variable.Get();
//						Double3Variable = ((FbxSurfacePhong *)tempMaterial)->Emissive;
//						std::cout << "EmissiveColor: " << (float)Double3Variable.Get()[0] << ", " << (float)Double3Variable.Get()[1] << ", " << (float)Double3Variable.Get()[2] << "\n";
//						NewMaterial.EmissiveColor[0] = (float)Double3Variable.Get()[0]; NewMaterial.EmissiveColor[1] = (float)Double3Variable.Get()[1]; NewMaterial.EmissiveColor[2] = (float)Double3Variable.Get()[2];
//
//						Double1Variable = ((FbxSurfacePhong *)tempMaterial)->TransparencyFactor;
//						std::cout << "Transparency(Factor): " << (float)Double1Variable.Get() << "\n";
//						NewMaterial.Transparency = (float)Double1Variable.Get();
//
//						NewMaterial.SpecularFactor = 0;
//						NewMaterial.SpecularColor[0] = 0; NewMaterial.SpecularColor[1] = 0; NewMaterial.SpecularColor[2] = 0;
//						NewMaterial.Shininess = 0;
//						NewMaterial.Reflection = 0;
//						NewMaterial.ReflectionColor[0] = 0;	NewMaterial.ReflectionColor[1] = 0;	NewMaterial.ReflectionColor[2] = 0;
//
//						for (unsigned int j = 0; j < nrOfTextures; j++)
//						{
//							FbxProperty propertyS;
//							propertyS = tempMaterial->FindProperty(FbxSurfacePhong::sBump);
//							FbxTexture *testTextureBump = propertyS.GetSrcObject<FbxTexture>();
//
//							if (testTextureBump)
//							{
//								FbxFileTexture* TestFileTexture = FbxCast<FbxFileTexture>(testTextureBump);
//								bumpName = TestFileTexture->GetRelativeFileName();
//								const char* temp = TestFileTexture->GetFileName();
//								for (unsigned int d = 0; d < strlen(temp); d++)
//								{
//									NewMaterial.BumpPath[d] = temp[d];
//								}
//							}
//							else
//							{
//								const char* temp = "Empty.";
//								for (unsigned int o = 0; o < strlen(temp); o++)
//								{
//									NewMaterial.BumpPath[o] = temp[o];
//								}
//							}
//
//							propertyS = tempMaterial->FindProperty(FbxSurfacePhong::sDiffuse);
//							FbxTexture *testTexture = propertyS.GetSrcObject<FbxTexture>();
//
//							if (testTexture)
//							{
//								FbxFileTexture* TestFileTexture = FbxCast<FbxFileTexture>(testTexture);
//								TextureName = TestFileTexture->GetRelativeFileName();
//								const char* temp = TestFileTexture->GetFileName();
//								for (unsigned int d = 0; d < strlen(temp); d++)
//								{
//									NewMaterial.TexturePath[d] = temp[d];
//								}
//							}
//							else
//							{
//								const char* temp = "Empty.";
//								for (unsigned int o = 0; o < strlen(temp); o++)
//								{
//									NewMaterial.TexturePath[o] = temp[o];
//								}
//							}
//						}
//
//						std::cout << std::endl;
//						MatVector.push_back(NewMaterial);
//					}
//					else
//					{
//						std::cout << "Material(" << i << ")" << "is an UNKNOWN" << " in scene.\n ERROR!";
//					}
//				}
//
//
//
//				//Copying / moving files to the custom file folder.
//				for (unsigned int y = 0; y < MatVector.size(); y++)
//				{
//					char NewFolderPathTexture[150];
//					char NewFolderPathBump[150];
//
//					if (TextureName)
//					{
//						GetCurrentDirectoryA(150, NewFolderPathTexture);
//						strcat_s(NewFolderPathTexture, "/Materials&Textures/");
//						std::string tempString = MatVector.at(y).TexturePath;
//						tempString = getFileName(tempString.c_str());
//						TextureName = tempString.c_str();
//						strcat_s(NewFolderPathTexture, TextureName);
//
//						if (CopyFile((const char*)&MatVector.at(y).TexturePath, (const char*)&NewFolderPathTexture, false))
//						{
//							std::cout << "Material(" << y + 1 << ") Copied Diffuse Texture File: Success!" << std::endl << std::endl;
//							for (unsigned int i = 0; i < 150; i++)
//								MatVector.at(y).TexturePath[i] = NewFolderPathTexture[i];
//						}
//						else
//						{
//							std::cout << "Material(" << y + 1 << ") Copied Diffuse Texture File: Failed!" << std::endl;
//						}
//					}
//
//					if (bumpName)
//					{
//						GetCurrentDirectoryA(150, NewFolderPathBump);
//						strcat_s(NewFolderPathBump, "/Materials&Textures/");
//						std::string tempString = MatVector.at(y).BumpPath;
//						tempString = getFileName(tempString.c_str());
//						bumpName = tempString.c_str();
//						strcat_s(NewFolderPathBump, bumpName);
//
//						if (CopyFile((const char*)&MatVector.at(y).BumpPath, (const char*)&NewFolderPathBump, false))
//						{
//							std::cout << "Material(" << y + 1 << ") Copied Bump Texture File: Success!" << std::endl << std::endl;
//							for (unsigned int i = 0; i < 150; i++)
//								MatVector.at(y).BumpPath[i] = NewFolderPathBump[i];
//						}
//						else
//						{
//							std::cout << "Material(" << y + 1 << ") Copied Bump Texture File: Failed!" << std::endl;
//						}
//					}
//				}
//
//
//				//Exporting to custom format.
//				for (unsigned int y = 0; y < MatVector.size(); y++)
//				{
//					readable_file << "AmbientFactor: ";
//					outfile.write((const char*)&MatVector.at(y).AmbientFactor, sizeof(float));
//					readable_file << MatVector.at(y).AmbientFactor << std::endl;
//					readable_file << "AmbientColor: ";
//					outfile.write((const char*)&MatVector.at(y).AmbientColor[0], sizeof(float));
//					readable_file << MatVector.at(y).AmbientColor[0] << ", ";
//					outfile.write((const char*)&MatVector.at(y).AmbientColor[1], sizeof(float));
//					readable_file << MatVector.at(y).AmbientColor[1] << ", ";
//					outfile.write((const char*)&MatVector.at(y).AmbientColor[2], sizeof(float));
//					readable_file << MatVector.at(y).AmbientColor[2] << std::endl;
//
//
//					readable_file << "DiffuseFactor: ";
//					outfile.write((const char*)&MatVector.at(y).DiffuseFactor, sizeof(float));
//					readable_file << MatVector.at(y).DiffuseFactor << std::endl;
//					readable_file << "DiffuseColor: ";
//					outfile.write((const char*)&MatVector.at(y).DiffuseColor[0], sizeof(float));
//					readable_file << MatVector.at(y).DiffuseColor[0] << ", ";
//					outfile.write((const char*)&MatVector.at(y).DiffuseColor[1], sizeof(float));
//					readable_file << MatVector.at(y).DiffuseColor[1] << ", ";
//					outfile.write((const char*)&MatVector.at(y).DiffuseColor[2], sizeof(float));
//					readable_file << MatVector.at(y).DiffuseColor[2] << std::endl;
//
//
//					readable_file << "EmissiveFactor: ";
//					outfile.write((const char*)&MatVector.at(y).EmissiveFactor, sizeof(float));
//					readable_file << MatVector.at(y).EmissiveFactor << std::endl;
//					readable_file << "EmissiveColor: ";
//					outfile.write((const char*)&MatVector.at(y).EmissiveColor[0], sizeof(float));
//					readable_file << MatVector.at(y).EmissiveColor[0] << ", ";
//					outfile.write((const char*)&MatVector.at(y).EmissiveColor[1], sizeof(float));
//					readable_file << MatVector.at(y).EmissiveColor[1] << ", ";
//					outfile.write((const char*)&MatVector.at(y).EmissiveColor[2], sizeof(float));
//					readable_file << MatVector.at(y).EmissiveColor[2] << std::endl;
//
//
//					readable_file << "Transparency: ";
//					outfile.write((const char*)&MatVector.at(y).Transparency, sizeof(float));
//					readable_file << MatVector.at(y).Transparency << std::endl;
//
//					//Phong Part.
//
//					readable_file << "SpecularFactor: ";
//					outfile.write((const char*)&MatVector.at(y).SpecularFactor, sizeof(float));
//					readable_file << MatVector.at(y).SpecularFactor << std::endl;
//					readable_file << "SpecularColor: ";
//					outfile.write((const char*)&MatVector.at(y).SpecularColor[0], sizeof(float));
//					readable_file << MatVector.at(y).SpecularColor[0] << ", ";
//					outfile.write((const char*)&MatVector.at(y).SpecularColor[1], sizeof(float));
//					readable_file << MatVector.at(y).SpecularColor[1] << ", ";
//					outfile.write((const char*)&MatVector.at(y).SpecularColor[2], sizeof(float));
//					readable_file << MatVector.at(y).SpecularColor[2] << std::endl;
//
//
//					readable_file << "Shininess: ";
//					outfile.write((const char*)&MatVector.at(y).Shininess, sizeof(float));
//					readable_file << MatVector.at(y).Shininess << std::endl;
//
//
//					readable_file << "Reflection: ";
//					outfile.write((const char*)&MatVector.at(y).Reflection, sizeof(float));
//					readable_file << MatVector.at(y).Reflection << std::endl;
//					readable_file << "ReflectionColor: ";
//					outfile.write((const char*)&MatVector.at(y).ReflectionColor[0], sizeof(float));
//					readable_file << MatVector.at(y).ReflectionColor[0] << ", ";
//					outfile.write((const char*)&MatVector.at(y).ReflectionColor[1], sizeof(float));
//					readable_file << MatVector.at(y).ReflectionColor[1] << ", ";
//					outfile.write((const char*)&MatVector.at(y).ReflectionColor[2], sizeof(float));
//					readable_file << MatVector.at(y).ReflectionColor[2] << std::endl;
//
//					//Texture Paths part.
//					std::cout << "Diffuse Texture Path: " << MatVector.at(y).TexturePath << std::endl;
//					readable_file << "Diffuse Texture Path: " << MatVector.at(y).TexturePath << std::endl;
//					outfile.write((const char*)&MatVector.at(y).TexturePath, sizeof(const char[150]));
//
//					std::cout << "Bump Texture Path: " << MatVector.at(y).BumpPath << std::endl;
//					readable_file << "Bump Texture Path: " << MatVector.at(y).BumpPath << std::endl;
//					outfile.write((const char*)&MatVector.at(y).BumpPath, sizeof(const char[150]));
//				}
//				MatVector.clear();
//				readable_file.close();
//				outfile.close();
//			
//		
//	}
//
//
//}
//void Converter::getGroups(FbxNode * scene_node, std::string par)
//{
//	std::string nodeName = scene_node->GetName(); // used for debugging purposes
//	std::cout << "NODE: " << nodeName << std::endl << std::endl;
//
//	for (size_t i = 0; i < scene_node->GetChildCount(); i++)
//	{
//		FbxNode* child = scene_node->GetChild(i);
//		Group group;
//
//		std::string type = child->GetTypeName();
//		std::string parent = child->GetParent()->GetName();
//
//		if (type == "Null") // has no special attribute like Mesh etc...
//		{
//			if (parent == "RootNode" || parent == par) // and it's rooted to the scene... We've got a group!
//			{
//				if (child->GetChildCount() == 0) continue;
//
//				FbxDouble3 translation = scene_node->LclTranslation.Get();
//				FbxDouble3 rotation = scene_node->LclRotation.Get();
//				FbxDouble3 scaling = scene_node->LclScaling.Get();
//
//				// Translation
//				group.group_transform.transform_position[0] = (float)translation.mData[0];
//				group.group_transform.transform_position[1] = (float)translation.mData[1];
//				group.group_transform.transform_position[2] = (float)translation.mData[2];
//
//				// Rotation
//				group.group_transform.transform_rotation[0] = (float)rotation.mData[0];
//				group.group_transform.transform_rotation[1] = (float)rotation.mData[1];
//				group.group_transform.transform_rotation[2] = (float)rotation.mData[2];
//
//				// Scaling
//				group.group_transform.transform_scale[0] = (float)scaling.mData[0];
//				group.group_transform.transform_scale[1] = (float)scaling.mData[1];
//				group.group_transform.transform_scale[2] = (float)scaling.mData[2];
//
//				for (size_t j = 0; j < child->GetChildCount(); j++)
//				{
//					groupChild tempChild;
//					std::string tempString = child->GetChild(j)->GetName();
//					
//					for (int x = 0; x < tempString.size(); x++)
//						tempChild.name[x] = tempString[x];
//					group.children_names.push_back(tempChild);
//				}
//
//				//group.children.push_back(c_child); // throw in all children (non child in child) into the group
//
//				createGroupFiles(child, &group);
//
//				std::cout << "The group has name: " << child->GetName() << " and has " << child->GetChildCount() << " children!" << std::endl;
//				getGroups(child, child->GetName());
//			}
//		}
//	}
//}
//
//bool Converter::createGroupFiles(FbxNode * scene_node, Group * group)
//{
//	const char * temp = "_Group";
//	const char* node_name = scene_node->GetName();
//	std::string filename = std::string(node_name) + "_Group.bin";
//	std::ofstream outfile(filename, std::ofstream::binary);
//	std::string readable_file_name = std::string(node_name) + "_Group.txt";
//	std::ofstream readable_file(readable_file_name);
//
//
//	std::string groupName = scene_node->GetName();
//	int childCount = group->children_names.size();
//
//
//	std::cout << "Children count: " << group->children_names.size() << std::endl;
//
//	outfile.write((const char*)&childCount, sizeof(int));
//
//	outfile.write((const char*)&group->group_transform.transform_position[0], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_position[1], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_position[2], sizeof(float));
//									
//	outfile.write((const char*)&group->group_transform.transform_rotation[0], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_rotation[1], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_rotation[2], sizeof(float));
//									 
//	outfile.write((const char*)&group->group_transform.transform_scale[0], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_scale[1], sizeof(float));
//	outfile.write((const char*)&group->group_transform.transform_scale[2], sizeof(float));
//
//
//	for (size_t i = 0; i < group->children_names.size(); i++)
//	{
//		readable_file << "Child " << std::to_string(i) << ": " << group->children_names[i].name << std::endl;
//		outfile.write((const char*)group->children_names[i].name, sizeof(char[100]));
//	}
//	readable_file.close();
//	outfile.close();
//
//	return true;
//}