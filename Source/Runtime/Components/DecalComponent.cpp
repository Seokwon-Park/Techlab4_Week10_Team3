#include "EnginePCH.h"
#include "DecalComponent.h"

#include "Engine/Scene.h"
#include "Engine/World.h"

void UDecalComponent::TickComponent(float DeltaTime)
{
	if (CurrentTime > Delay + FadeIn + Visible + FadeOut)
	{
		return;
	}
	CurrentTime += DeltaTime;

	const float FadeInEnd = Delay + FadeIn;
	const float VisibleEnd = FadeInEnd + Visible;
	const float FadeOutEnd = VisibleEnd + FadeOut;

	if (CurrentTime < Delay) {
		Fade = 0.0f;
	}
	else if (CurrentTime < FadeInEnd) {
		Fade = (CurrentTime - Delay) / FadeIn;
	}
	else if (CurrentTime < VisibleEnd) {
		Fade = 1.0f;
	}
	else if (CurrentTime < FadeOutEnd) {
		Fade = 1 - (CurrentTime - VisibleEnd) / FadeOut;
	}
	else
		Fade = 0.0f;
	
}

void UDecalComponent::SetDecalViewProj()
{
	FVector Forward, Up;
	const FMatrix World = this->GetWorldMatrix();
	Forward = FVector(World[0][0], World[0][1], World[0][2]);
	Up = FVector(World[2][0], World[2][1], World[2][2]);
	Forward = Forward.Normalized();
	Up=  Up.Normalized();

	FMatrix View = LookAt(this->GetWorldLocation(), this->GetWorldLocation() + Forward, Up);
	FMatrix Proj = Ortho(Extent.Y * 2, Extent.Y / Extent.Z, -Extent.X, Extent.X);
	DecalViewProj = View * Proj;
}

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
