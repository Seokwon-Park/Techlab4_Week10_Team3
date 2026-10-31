#include "EnginePCH.h"
#include "SkeletalMeshComponent.h"

const TArray<FBoneInfo>& USkeletalMeshComponent::GetBones() const
{
    return {};
}

int32 USkeletalMeshComponent::FindBoneIndex(const FString& Name) const
{
    return int32();
}

FTransform USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
    return FTransform();
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FTransform& Local)
{
}

FMatrix USkeletalMeshComponent::GetBoneComponentMatrix(int32 BoneIndex) const
{
    return FMatrix();
}

FMatrix USkeletalMeshComponent::GetBoneWorldMatrix(int32 BoneIndex) const
{
    return FMatrix();
}

void USkeletalMeshComponent::ResetPose()
{
}

void USkeletalMeshComponent::RefreshBoneTransformsNow()
{
}
