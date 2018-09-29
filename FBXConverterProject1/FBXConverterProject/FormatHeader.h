#pragma once
#include <windows.h>
#include <DirectXMath.h>
struct Vertex
{
	float vertex_position[3];
	float vertex_UVCoord[2];
	float vertex_normal[3];
};
struct AnimatedVertex
{
	float vertex_position[3];
	float vertex_UVCoord[2];
	float vertex_normal[3];
	unsigned int influencing_joint[4];
	float joint_weights[4];
};
struct Transform {
	float transform_position[3];
	float transform_rotation[3];
	float transform_scale[3];
};
struct MeshHeader {

	unsigned int mesh_nrOfVertices;
	unsigned int mesh_materialID;
};
struct CameraHeader
{
	unsigned int camera_nrOfCameras;
};
struct SkeletonHeader
{
	unsigned int skeleton_nrOfJoints;
	const char skeletonID[100];


};
struct Joint
{
	const char joint_name[100];
	Transform joint_transform;
	unsigned int parentIndex;
};

struct AnimationHeader
{
	unsigned int anim_nrOfKeys;

};
struct AnimKey
{
	Transform jointTransform;
};
