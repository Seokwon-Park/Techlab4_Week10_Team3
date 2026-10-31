#pragma once

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshActor.generated.h"

UCLASS()
class ASkeletalMeshActor : public AActor
{
	GENERATED_BODY()

public:
	ASkeletalMeshActor();
	
	USkinnedMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

private:
	USkinnedMeshComponent* SkeletalMeshComponent;
};

