#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "SkeletalMeshEditorTab.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor/HitoriEd/EditorEngine.h"
#include "Engine/World.h"
#include "Math/BoxSphereBounds.h"
#include "Render/SkeletalMesh.h"

FSkeletalMeshEditor::FSkeletalMeshEditor(
	UEditorEngine& InEditorEngine,
	FEditorUI& InEditorUI,
	const FString& InAssetPath,
	USkeletalMesh& InMesh
)
	: FAssetEditor(InEditorEngine, InEditorUI, InAssetPath)
	, Mesh(&InMesh)
{
}

bool FSkeletalMeshEditor::InitializeResources()
{
	// 메시가 없음
	if (!Mesh)
	{
		return false;
	}

	// 기본 포즈를 표시할 LOD0 렌더 데이터가 필요합니다.
	const auto& LODs = Mesh->GetRenderData().LODRenderData;
	if (LODs.IsEmpty())
	{
		return false;
	}

	if (LODs[0].BindVertices.IsEmpty() || LODs[0].Indices.IsEmpty())
	{
		return false;
	}

	// 부모 InitializeResources 호출 (FPreviewScene 초기화 및 세팅)
	if (!FAssetEditor::InitializeResources())
	{
		return false;
	}

	// PreviewScene에 SkeletalMeshComponent 추가
	auto Component = PreviewScene.AddComponent(USkeletalMeshComponent::StaticClass());
	MeshComponent = Cast<USkeletalMeshComponent>(Component);
	if (!MeshComponent)
	{
		return false;
	}

	// SkeletalMeshComponent에 메시를 설정
	MeshComponent->SetSkeletalMesh(Mesh);
	// 애니메이션 재생 없이 바인드 포즈의 본 변환과 스키닝 결과를 준비합니다.
	MeshComponent->RefreshBoneTransformsNow();

	// 새로운 ViewportClient 생성
	ViewportClient = MakeUnique<FEditorViewportClient>(PreviewScene);
	ViewportClient->SetViewRotation(FRotator(30.0f, 210.0f, 0.0f));

	// 카메라를 메시에 포커싱
	FocusMesh();

	return true;
}

TUniquePtr<IEditorTab> FSkeletalMeshEditor::CreateTab()
{
	return MakeUnique<FSkeletalMeshEditorTab>(*this);
}

void FSkeletalMeshEditor::FocusMesh()
{
	if (!ViewportClient)
	{
		return;
	}

	if (!Mesh)
	{
		return;
	}

	// 메시의 AABB를 가져와서 BoxSphereBounds로 변환
	const FBox& Box = Mesh->GetRenderData().LODRenderData[0].BindAABB;
	const FVector Extent = (Box.Max - Box.Min) * 0.5f;
	FBoxSphereBounds Bound
	{
		.Origin = (Box.Min + Box.Max) * 0.5f,
		.BoxExtent = Extent,
		.SphereRadius = Extent.Size()
	};

	// AABB로 ViewportClient의 카메라를 메시에 포커싱
	ViewportClient->FocusOnBounds(Bound);
}

void FSkeletalMeshEditor::SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input)
{
	ViewportWidth = Width;
	ViewportHeight = Height;
	bViewportVisible = Width > 0 && Height > 0;
	CameraInput = Input;
}

void FSkeletalMeshEditor::Tick(float DeltaTime)
{
	if (!ViewportClient)
	{
		return;
	}

	if (!ViewportClient->GetWorld())
	{
		return;
	}

	// 뷰포트가 보이면 카메라 이동을 적용
	if (bViewportVisible)
	{
		FViewportCameraState Camera{
			.Location = ViewportClient->GetViewLocation(),
			.Rotation = ViewportClient->GetViewRotation(),
			.OrthoWidth = ViewportClient->GetOrthoWidth(),
			.ViewWidth = static_cast<float>(ViewportWidth),
			.bPerspective = true
		};
		ApplyViewportCameraMovement(Camera, CameraInput, DeltaTime, 5.0f, 0.2f);
		ViewportClient->SetViewLocation(Camera.Location);
		ViewportClient->SetViewRotation(Camera.Rotation);
	}

	// 카메라 입력 리셋
	CameraInput = {};

	// 부모 Tick 호출 (PreviewScene Tick)
	FAssetEditor::Tick(DeltaTime);
}

void FSkeletalMeshEditor::Render()
{
	// Viewport가 보이지 않으면 렌더링하지 않음
	const bool bRender = bViewportVisible;
	bViewportVisible = false;
	if (!bRender)
	{
		return;
	}

	if (!ViewportClient)
	{
		return;
	}

	if (!ViewportClient->GetWorld())
	{
		return;
	}

	// ViewportClient 크기 변경
	ViewportClient->Resize(ViewportWidth, ViewportHeight);

	// ViewportClient의 RTV가 없다면 렌더링하지 않음
	if (!ViewportClient->GetViewTargets().IsValidFor(ViewportWidth, ViewportHeight))
	{
		return;
	}

	// FocusMesh가 아직 호출되지 않았다면 호출
	if (bInitialFocusPending)
	{
		FocusMesh();
		bInitialFocusPending = false;
	}

	// EditorEngine에 ViewportClient를 전달하여 렌더링
	EditorEngine.RenderPreviewViewport(*ViewportClient);
}

void FSkeletalMeshEditor::ShutdownResources()
{
	ViewportClient.reset();
	MeshComponent = nullptr;
	Mesh = nullptr;
	FAssetEditor::ShutdownResources();
}
