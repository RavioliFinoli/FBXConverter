#pragma once
#include <fstream>
#include <vector>
#include <fbxsdk.h>


namespace FBXExport
{
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
	
	struct DecomposedTransform
	{
		Vec4 translation;
		Vec4 rotation;
		Vec4 scale;
		DecomposedTransform(Vec4 t, Vec4 r = { 0.0, 0.0, 0.0, 0.0 }, Vec4 s = { 1.0, 1.0, 1.0, 1.0 }) : translation(t), rotation(r), scale(s)
		{}
		DecomposedTransform()
		{
			translation = { 0.0, 0.0, 0.0, 1.0 };
			rotation = { 0.0, 0.0, 0.0, 0.0 };
			scale = { 1.0, 1.0, 1.0, 1.0 };
		}
		DecomposedTransform(FbxAMatrix affineMatrix)
		{
			translation = Vec4(affineMatrix.GetT());
			rotation = Vec4(affineMatrix.GetQ());
			scale = Vec4(affineMatrix.GetS());
		}
		DecomposedTransform& operator=(const FbxAMatrix& affineMatrix)
		{
			translation = Vec4(affineMatrix.GetT());
			rotation = Vec4(affineMatrix.GetQ());
			scale = Vec4(affineMatrix.GetS());
			return *this;
		}
	};
	
	struct Bone
	{
		DecomposedTransform jointInverseBindPoseTransform;
		DecomposedTransform jointReferenceTransform;

		int32_t parentIndex;

		FbxString name;
		Bone(DecomposedTransform inverseBindPose, DecomposedTransform referenceTransform, int32_t parent)
			: jointInverseBindPoseTransform(inverseBindPose), jointReferenceTransform(referenceTransform), parentIndex(parent)
		{}
		Bone()
		{}
	};

	struct Skeleton
	{
		std::vector<FBXExport::Bone> joints;
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

	void appendFloat4(std::ofstream& file, const Vec4 vec)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&vec, sizeof(Vec4));
	}

	/// Writes each Vec4 component of the Transform (order is T, R, S)
	void appendTransform(std::ofstream& file, const DecomposedTransform transform)
	{
		if (!file.is_open())
			return;

		/// Write each vector (T, R, S) (R is quaternion)
		file.write((const char*)&transform.translation, sizeof(Vec4));
		file.write((const char*)&transform.rotation, sizeof(Vec4));
		file.write((const char*)&transform.scale, sizeof(Vec4));
	}

	void appendInt32(std::ofstream& file, const int32_t value)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&value, sizeof(int32_t));
	}

	void appendBone(std::ofstream& file, const FBXExport::Bone& bone)
	{
		if (!file.is_open())
			return;

		/// Write each transform and parentIndex
		appendTransform(file, bone.jointInverseBindPoseTransform);
		appendTransform(file, bone.jointReferenceTransform);
		appendInt32(file, bone.parentIndex);
	}

	/// Writes the bone count and each bone (order is inverseBindPose, referencePose, parentIndex)
	void appendSkeleton(std::ofstream& file, const FBXExport::Skeleton& skeleton)
	{
		if (!file.is_open())
			return;

		/// Write bone count (needed for loading)
		int32_t jointCount = skeleton.joints.size();
		file.write((char*)&jointCount, sizeof(jointCount));

		/// Write each joint
		for (const auto& bone : skeleton.joints)
		{
			appendBone(file, bone);
		}
	}

	///// Reads a Vec4 (four floats in x, y, z, w order)
	//Vec4 loadVec4(std::ifstream& file)
	//{
	//	if (!file.is_open())
	//		return FBXExport::Vec4();

	//	Vec4 vec;
	//	file.read((char*)&vec, sizeof(Vec4));

	//	return vec;
	//}

	//int32_t loadInt32(std::ifstream& file)
	//{
	//	if (!file.is_open())
	//		return 0;

	//	int32_t int32;
	//	file.read((char*)&int32, sizeof(int32_t));
	//	return int32;
	//}

	//DecomposedTransform loadTransform(std::ifstream& file)
	//{
	//	if (!file.is_open())
	//		return FBXExport::DecomposedTransform();

	//	DecomposedTransform transform;

	//	transform.translation = (loadVec4(file));
	//	transform.rotation    = (loadVec4(file));
	//	transform.scale       = (loadVec4(file));

	//	return transform;
	//}

	//FBXExport::Bone loadBone(std::ifstream& file)
	//{
	//	if (!file.is_open())
	//		return FBXExport::Bone();

	//	FBXExport::Bone bone;
	//	bone.jointInverseBindPoseTransform = loadTransform(file);
	//	bone.jointReferenceTransform       = loadTransform(file);
	//	bone.parentIndex                   = loadInt32(file);

	//	return bone;
	//}

	//Skeleton loadSkeleton(std::ifstream& file, int32_t boneCount)
	//{
	//	if (!file.is_open())
	//		return FBXExport::Skeleton();

	//	/// Read and add each bone to vector
	//	Skeleton skeleton;
	//	for (int i = 0; i < boneCount; i++)
	//		skeleton.joints.push_back(loadBone(file));
	//	
	//	return skeleton;
	//}

	//// If no boneCount is supplied, we assume we read that data first
	//Skeleton loadSkeleton(std::ifstream& file)
	//{
	//	if (!file.is_open())
	//		return FBXExport::Skeleton();

	//	/// Read bone count
	//	int32_t boneCount = loadInt32(file);

	//	/// Read and add each bone to vector
	//	Skeleton skeleton;
	//	for (int i = 0; i < boneCount; i++)
	//		skeleton.joints.push_back(loadBone(file));

	//	return skeleton;
	//}
}

