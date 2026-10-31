#include "EnginePCH.h"
#include "SkeletalMeshObject.h"
#include "RenderCommand.h"

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
	UploadVertices();
}

bool FSkeletalMeshObjectCPUSkin::UpdateBounds()
{
	return false;
}

void FSkeletalMeshObjectCPUSkin::UpdateRefToLocalMatrices(const TArray<FMatrix>& ComponentSpaceTransforms)
{
}

void FSkeletalMeshObjectCPUSkin::SkinVertices()
{
}

void FSkeletalMeshObjectCPUSkin::UploadVertices()
{
	if (!FinalVertexBuffer) return;
	void* Dst = RenderCommand::MapWriteDiscard(FinalVertexBuffer.get());
	if (!Dst)return;
	std::memcpy(Dst, SkinnedVertices.GetData(), sizeof(FVertex) * SkinnedVertices.Num());
	RenderCommand::Unmap(FinalVertexBuffer.get());
}
