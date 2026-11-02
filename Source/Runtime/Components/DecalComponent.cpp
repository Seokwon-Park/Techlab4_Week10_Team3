#include "EnginePCH.h"
#include "DecalComponent.h"

#include "Engine/Scene.h"
#include "Engine/World.h"

void UDecalComponent::TickComponent(float DeltaTime)
{
	const float FadeInEnd = Delay + FadeIn;
	const float VisibleEnd = FadeInEnd + Visible;
	const float FadeOutEnd = VisibleEnd + FadeOut;

	if (CurrentTime > FadeOutEnd)
	{
		return;
	}

	CurrentTime += DeltaTime;

	float t = 0.0f;
	if (CurrentTime < Delay) {
		t = 0.0f;
	}
	else if (CurrentTime < FadeInEnd) {
		t = (CurrentTime - Delay) / FadeIn;
	}
	else if (CurrentTime < VisibleEnd) {
		t = 1.0f;
	}
	else if (CurrentTime < FadeOutEnd) {
		t = 1 - (CurrentTime - VisibleEnd) / FadeOut;
	}
	else {
		t = 0.0f;
	}
	Fade = Ease(t);
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

float UDecalComponent::Ease(float t) const
{
	return FMath::CubicInterp(0.0f, 0.0f, 1.0f, 0.0f, t);
}
