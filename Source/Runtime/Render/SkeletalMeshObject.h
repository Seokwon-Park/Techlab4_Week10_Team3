#pragma once

#include "Render/SkeletalMesh.h"

class FSkeletalMeshObject
{
public:
	explicit FSkeletalMeshObject(USkeletalMesh* InMesh) : SkeletalMesh(InMesh) {}
	virtual ~FSkeletalMeshObject() = default;

	virtual void Update(const TArray<FMatrix>& ComponentSpaceTransforms) = 0;
	virtual FVertexBuffer* GetVertexBuffer(uint32 LOD) const = 0;

	FIndexBuffer* GetIndexBuffer(uint32 LOD) const
	{
		return SkeletalMesh->GetRenderData().LODRenderData[LOD].IndexBuffer.get();
	}

	const FBox& GetBounds() const { return Bounds; }
	virtual const TArray<FVertex>* GetCPUSkinnedVertices() const { return nullptr; }
	virtual bool UpdateBounds() { return false; }

protected:
	USkeletalMesh* SkeletalMesh;
	FBox Bounds;
};

class FSkeletalMeshObjectCPUSkin : public FSkeletalMeshObject
{
public:
	explicit FSkeletalMeshObjectCPUSkin(USkeletalMesh* InMesh);

	void Update(const TArray<FMatrix>& ComponentSpaceTransforms) override;
	FVertexBuffer* GetVertexBuffer(uint32 LOD) const override { return FinalVertexBuffer.get(); }
	const TArray<FVertex>* GetCPUSkinnedVertices() const override { return &SkinnedVertices; }
	bool UpdateBounds() override;

private:
	void UpdateRefToLocalMatrices(const TArray<FMatrix>& ComponentSpaceTransforms);
	void SkinVertices();
	void UploadVertices();

	TUniquePtr<FVertexBuffer> FinalVertexBuffer;
	TArray<FVertex> SkinnedVertices;
	TArray<FMatrix> RefToLocal;
	FBox SkinnedBounds;
};
