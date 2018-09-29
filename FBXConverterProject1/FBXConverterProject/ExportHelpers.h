#pragma once
#include <fstream>
#include <vector>
#include <fbxsdk.h>

namespace Export
{
	struct Joint
	{
		Transform jointInverseBindPoseTransform;
		Transform jointReferenceTransform;

		int32_t parentIndex;

		Joint(Transform inverseBindPose, Transform referenceTransform, int32_t parent) 
			: jointInverseBindPoseTransform(inverseBindPose), jointReferenceTransform(referenceTransform), parentIndex(parent)
		{}
		Joint() 
		{}
	};

	struct Skeleton 
	{
		std::vector<Joint> joints;
	};

	struct Vec3
	{
		float x, y, z;

		Vec3(FbxVector4 vec)
		{
			x = vec.mData[0];
			y = vec.mData[1];
			z = vec.mData[2];
		}
	};
	
	struct Vec4
	{
		float x, y, z, w;

		Vec4(FbxVector4 vec)
		{
			x = vec.mData[0];
			y = vec.mData[1];
			z = vec.mData[2];
			w = vec.mData[3];
		}
		Vec4(FbxQuaternion quat)
		{
			x = quat.mData[0];
			y = quat.mData[1];
			z = quat.mData[2];
			w = quat.mData[3];
		}
		Vec4(float _x, float _y, float _z, float _w) 
			: x(_x), y(_y), z(_z), w(_w)
		{};
		Vec4() {};
	};
	
	struct Transform
	{
		Vec4 translation;
		Vec4 rotation;
		Vec4 scale;

		Transform(Vec4 t, Vec4 r = { 0.0, 0.0, 0.0, 0.0 }, Vec4 s = { 1.0, 1.0, 1.0, 1.0 }) : translation(t), rotation(r), scale(s)
		{}
		Transform()
		{
			translation = {0.0, 0.0, 0.0, 1.0};
			rotation = {0.0, 0.0, 0.0, 0.0};
			scale = {1.0, 1.0, 1.0, 1.0};
		}
		Transform(FbxAMatrix affineMatrix)
		{
			translation = Vec4(affineMatrix.GetT());
			rotation = Vec4(affineMatrix.GetQ());
			scale = Vec4(affineMatrix.GetS());
		}
	};
	
	void appendFloat4(std::ofstream& file, Vec4 vec)
	{
		if (!file.is_open())
			return;
	
		file.write((const char*)&vec, sizeof(Vec4));
	}
	
	void appendTransform(std::ofstream& file, Export::Transform transform)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&transform, sizeof(Vec4));
	}
}
