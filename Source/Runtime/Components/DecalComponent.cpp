#include "EnginePCH.h"
#include "DecalComponent.h"

#include "Engine/Scene.h"
#include "Engine/World.h"

void UDecalComponent::CreateRenderState()
{
	Super::CreateRenderState();
	GetWorld()->GetScene().AddDecal(this);
}

void UDecalComponent::DestroyRenderState()
{
	GetWorld()->GetScene().RemoveDecal(this);
	Super::DestroyRenderState();
}
