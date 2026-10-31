#pragma once

#include "Render/StaticMeshData.h"
#include "Math/Transform.h"

inline constexpr int32 MaxBoneInfluences = 4;

struct FBoneInfo
{
	FString Name;
	int32 ParentIndex = -1; // 부모가 항상 앞 인덱스
	FTransform LocalBindPose; // 부모기준?
	FMatrix InverseBindPose; //컴포넌트 공간 바인드 역행렬
};

struct FSkinWeight
{
	uint8 BoneIndex[MaxBoneInfluences] = {};
	float Weight[MaxBoneInfluences] = {};
};

struct FSkeletalMeshData
{
	TArray<FVertex> BindVertices;
	TArray<FSkinWeight> SkinWeight;
	TArray<uint32> Indices;
	TArray<FStaticMeshSection> Sections;
	TArray<FStaticMaterialSlot> MaterialSlots;
	TArray<FBoneInfo> Bones;
	FBox BindAABB;
	FString SourcePath;

	bool Validate(FString& OutError) const;
};

// B 전용 테스트 데이터: Z축으로 Segments칸, 칸마다 본 하나. 관절 ±20%는 두 본에 선형 가중치
FSkeletalMeshData MakeTestBoneStick(int32 Segments = 3, float SegmentLength = 100.0f);

