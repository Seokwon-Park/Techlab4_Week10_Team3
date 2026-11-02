#include "EnginePCH.h"
#include "SkeletalMeshObject.h"
#include "RenderCommand.h"
#include "Tasks/Tasks.h"
#include "Tasks/TaskScheduler.h"
#include "Core/Stats/LightweightStats.h"

DECLARE_CYCLE_STAT("CPU Skinning", STAT_CpuSkinning);
DECLARE_CYCLE_STAT("Skin Upload", STAT_SkinUpload);

namespace
{
	// 비균등 스케일에서도 면에 수직이 되도록 3x3 부분의 여인수 행렬로 노멀을 옮긴다.
	// 미러(행렬식 음수)면 여인수 결과가 뒤집히므로 부호를 되돌린다.
	FVector TransformNormal(const FMatrix& M, const FVector& N)
	{
		const FVector R0(M.M[0][0], M.M[0][1], M.M[0][2]);
		const FVector R1(M.M[1][0], M.M[1][1], M.M[1][2]);
		const FVector R2(M.M[2][0], M.M[2][1], M.M[2][2]);
		const FVector C0 = FVector::Cross(R1, R2);
		const FVector Result = C0 * N.X + FVector::Cross(R2, R0) * N.Y + FVector::Cross(R0, R1) * N.Z;
		return (R0.Dot(C0) < 0.0f ? Result * -1.0f : Result).Normalized();
	}
}

FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectCPUSkin(USkeletalMesh* InMesh)
	:FSkeletalMeshObject(InMesh)
{
	const FSkeletalMeshLODRenderData& LOD = InMesh->GetRenderData().LODRenderData[0];
	SkinnedVertices = LOD.BindVertices;
	RefToLocal.SetNum(InMesh->GetNumBones());
	Bounds = SkinnedBounds = LOD.BindAABB;

	const uint32 Size = sizeof(FVertex) * static_cast<uint32>(SkinnedVertices.Num());
	FinalVertexBuffer = RenderCommand::CreateDynamicVertexBuffer(Size, sizeof(FVertex));

	UploadVertices();
}

void FSkeletalMeshObjectCPUSkin::Update(const TArray<FMatrix>& ComponentSpaceTransforms)
{
	UpdateRefToLocalMatrices(ComponentSpaceTransforms);
	SkinVertices();
	UploadVertices();
}

bool FSkeletalMeshObjectCPUSkin::UpdateBounds()
{
	bool bInside = true;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		if (SkinnedBounds.Min[Axis] < Bounds.Min[Axis] || SkinnedBounds.Max[Axis] > Bounds.Max[Axis])
			bInside = false;
	}
	if (bInside) return false;

	const FVector Margin = (SkinnedBounds.Max - SkinnedBounds.Min) * 0.25f;
	Bounds.Min = SkinnedBounds.Min - Margin;
	Bounds.Max = SkinnedBounds.Max + Margin;
	return true;
}

void FSkeletalMeshObjectCPUSkin::UpdateRefToLocalMatrices(const TArray<FMatrix>& ComponentSpaceTransforms)
{
	const TArray<FBoneInfo>& Bones = SkeletalMesh->GetRefSkeleton();
	const int32 Count = std::min(Bones.Num(), ComponentSpaceTransforms.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		RefToLocal[i] = Bones[i].InverseBindPose * ComponentSpaceTransforms[i];
	}
}

void FSkeletalMeshObjectCPUSkin::SkinVertices()
{
	SCOPE_CYCLE_COUNTER(STAT_CpuSkinning);
	const FSkeletalMeshLODRenderData& LOD = SkeletalMesh->GetRenderData().LODRenderData[0];
	const uint32 Count = static_cast<uint32>(LOD.BindVertices.Num());
	const uint32 NumThreads = Tasks::FTaskScheduler::Get().GetNumWorkers() + 1;
	const uint32 ChunkCount = std::clamp(Count / 2048u, 1u, NumThreads * 2);

	TArray<FBox> ChunkBounds;
	ChunkBounds.SetNum(ChunkCount);

	//Liner Blend Skinning LBS
	Tasks::ParallelForChunks(Count, ChunkCount, [&](uint32 Begin, uint32 End, uint32 Chunk)
		{
			FBox ChunkBox{ FVector(FLT_MAX, FLT_MAX, FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX) };
			for (uint32 v = Begin; v < End; ++v)
			{
				const FVertex& Bind = LOD.BindVertices[v];
				const FSkinWeight& W = LOD.SkinWeights[v];

				FMatrix M = RefToLocal[W.BoneIndex[0]] * W.Weight[0];
				for (int32 k = 1; k < MaxBoneInfluences; ++k)
				{
					if (W.Weight[k] > 0.0f)
						M += RefToLocal[W.BoneIndex[k]] * W.Weight[k];
				}

				FVertex& Out = SkinnedVertices[v];
				Out = Bind;
				Out.Position = M.TransformPosition(Bind.Position);
				Out.Normal = TransformNormal(M, Bind.Normal);
				const FVector Tangent = M.TransformVector(FVector(Bind.Tangent.X, Bind.Tangent.Y, Bind.Tangent.Z)).Normalized();
				Out.Tangent = FVector4(Tangent.X, Tangent.Y, Tangent.Z, Bind.Tangent.W);

				for (int Axis = 0; Axis < 3; ++Axis)
				{
					ChunkBox.Min[Axis] = FMath::Min(ChunkBox.Min[Axis], Out.Position[Axis]);
					ChunkBox.Max[Axis] = FMath::Max(ChunkBox.Max[Axis], Out.Position[Axis]);
				}
			}
			ChunkBounds[Chunk] = ChunkBox;
		});
	SkinnedBounds = ChunkBounds[0];
	for (uint32 i = 1; i < ChunkCount; ++i)
		SkinnedBounds.Expand(ChunkBounds[i]);

}

void FSkeletalMeshObjectCPUSkin::UploadVertices()
{
	SCOPE_CYCLE_COUNTER(STAT_SkinUpload);
	if (!FinalVertexBuffer) return;
	void* Dst = RenderCommand::MapWriteDiscard(FinalVertexBuffer.get());
	if (!Dst)return;
	std::memcpy(Dst, SkinnedVertices.GetData(), sizeof(FVertex) * SkinnedVertices.Num());
	RenderCommand::Unmap(FinalVertexBuffer.get());
}
