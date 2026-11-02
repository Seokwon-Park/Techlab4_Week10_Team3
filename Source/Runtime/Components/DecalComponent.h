#pragma once

#include "SceneComponent.h"
#include "Render/Texture2D.h"
#include "DecalComponent.generated.h"

UCLASS()
class UDecalComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	void TickComponent(float DeltaTime) override;

	UDecalComponent() { PrimaryComponentTick.bCanEverTick = true; }
	UTexture2D* GetDecalTexture() const { return DecalTexture; }
	void SetDecalTexture(UTexture2D* InTexture) { DecalTexture = InTexture; }
	FVector GetExtent() const { return Extent; }
	void SetExtent(FVector InExtent) { Extent = InExtent; }
	int32 GetSortOrder() const { return SortOrder; }
	void SetSortOrder(int32 InSortOrder) { SortOrder = InSortOrder; }

	void SetDecalViewProj();
	FMatrix GetDecalVP() { return DecalViewProj; }

	float GetFade() const { return Fade; }

protected:
	virtual bool ShouldCreateRenderState() const override
	{
		return true;
	}

	virtual void CreateRenderState() override;
	virtual void DestroyRenderState() override;

private:
	UPROPERTY(EditAnywhere) UTexture2D* DecalTexture = nullptr;
	UPROPERTY(EditAnywhere) FVector Extent = FVector(50, 50, 50);
	UPROPERTY(EditAnywhere) int32 SortOrder = 0;

	FMatrix DecalViewProj;

	float Fade = 0.0f;
	float Delay = 1.0f;
	float FadeIn = 1.0f;
	float Visible = 1.0f;
	float FadeOut = 1.0f;
	float CurrentTime = 0.0f;
};