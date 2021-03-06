#pragma once
#include <fstream>
#include <vector>
#include <fbxsdk.h>


namespace FBXExport
{
#pragma region ExportStructs
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
		Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
		{};

		float& operator[](int i)
		{
			float* ptr = &x;
			return *(ptr += i);
		}
	};

	struct Vec4ui
	{
		unsigned int x, y, z, w;

		Vec4ui(unsigned int _x, unsigned int _y, unsigned int _z, unsigned int _w)
			: x(_x), y(_y), z(_z), w(_w)
		{};
		Vec4ui() : x(0), y(0), z(0), w(0)
		{};

		unsigned int& operator[](int i) 
		{
			unsigned int* ptr = &x;
			return *(ptr += i);
		}
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
		Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z){};
		Vec3() {};
		float& operator[] (int i)
		{
			float* ptr = &x;
			return *(ptr + i);
		}
	};

	struct Vec2
	{
		float x, y;

		Vec2(FbxVector2 vec2)
		{
			x = vec2.mData[0];
			y = vec2.mData[1];
		}
		Vec2() : x(0.0), y(0.0) {};
		float& operator[] (int i)
		{
			float* ptr = &x;
			return *(ptr + i);
		}
	};

	struct AnimatedVertexStefan
	{
		FBXExport::Vec3 position;
		FBXExport::Vec2 UV;
		FBXExport::Vec3 normal;
		FBXExport::Vec4ui influencingJoints;
		FBXExport::Vec4 jointWeights;

		AnimatedVertexStefan(FBXExport::Vec3 pos, FBXExport::Vec2 uv, FBXExport::Vec3 nor)
		{
			position = pos;
			UV = uv;
			normal = nor;
			influencingJoints = FBXExport::Vec4ui();
			jointWeights = FBXExport::Vec4();
		}
		AnimatedVertexStefan() {};
	};
#pragma endregion ExportStructs
#pragma region WriteFunctions
	void appendFloat4(std::ofstream& file, const Vec4 vec)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&vec, sizeof(Vec4));
	}

	//Flips the Z axis before writing
	void appendFloat4AsDirectXVector(std::ofstream& file, const Vec4 vec)
	{
		if (!file.is_open())
			return;

		Vec4 v = vec;
		v.z *= -1.0f;

		file.write((const char*)&v, sizeof(Vec4));
	}

	//Flips X and Y components before writing
	void appendFloat4AsDirectXQuaternion(std::ofstream& file, const Vec4 vec)
	{
		if (!file.is_open())
			return;

		Vec4 v = vec;
		v.x *= -1.0f;
		v.y *= -1.0f;

		file.write((const char*)&v, sizeof(Vec4));
	}

	void appendFloat3(std::ofstream& file, const Vec3 vec)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&vec, sizeof(Vec3));
	}

	void appendFloat3AsDirectXVector(std::ofstream& file, const Vec3 vec)
	{
		if (!file.is_open())
			return;
		Vec3 v = vec;
		v.z *= -1.0f;

		file.write((const char*)&v, sizeof(Vec3));
	}

	void appendFloat2(std::ofstream& file, const Vec2 vec)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&vec, sizeof(Vec2));
	}

	void appendUInt4(std::ofstream& file, const Vec4ui vec)
	{
		if (!file.is_open())
			return;

		file.write((const char*)&vec, sizeof(Vec4ui));
	}

	/// Writes each Vec4 component of the Transform (order is T, R, S)
	void appendTransform(std::ofstream& file, const DecomposedTransform transform)
	{
		if (!file.is_open())
			return;

		/// Write each vector (T, R, S) (R is quaternion)
		appendFloat4AsDirectXVector(file, transform.translation);
		appendFloat4AsDirectXQuaternion(file, transform.rotation);
		appendFloat4(file, transform.scale);
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
		for (size_t i = 0; i < skeleton.joints.size(); i++)
		{
			//if (skeleton.joints[i].parentIndex > i)
			//{
			//	int ji = 0;
			//}
			appendBone(file, skeleton.joints[i]);
		}
	}
#pragma endregion WriteFunctions
}

