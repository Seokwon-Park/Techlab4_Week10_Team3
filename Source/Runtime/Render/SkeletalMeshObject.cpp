#include "EnginePCH.h"
#include "SkeletalMeshObject.h"
#include "RenderCommand.h"
#include "Tasks/Tasks.h"
#include "Tasks/TaskScheduler.h"

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
	const FSkeletalMeshLODRenderData& LOD = SkeletalMesh->GetRenderData().LODRenderData[0];
	const uint32 Count = static_cast<uint32>(LOD.BindVertices.Num());
	const uint32 NumThreads = Tasks::FTaskScheduler::Get().GetNumWorkers();
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
				Out.Normal = M.TransformVector(Bind.Normal).Normalized();
				const FVector Tangent = M.TransformVector(FVector(Bind.Tangent.X, Bind.Tangent.Y, Bind.Tangent.Z));
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
	if (!FinalVertexBuffer) return;
	void* Dst = RenderCommand::MapWriteDiscard(FinalVertexBuffer.get());
	if (!Dst)return;
	std::memcpy(Dst, SkinnedVertices.GetData(), sizeof(FVertex) * SkinnedVertices.Num());
	RenderCommand::Unmap(FinalVertexBuffer.get());
}
