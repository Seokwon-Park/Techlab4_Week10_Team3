#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "SkeletalMeshEditorTab.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor/HitoriEd/EditorEngine.h"
#include "Engine/World.h"
#include "Math/BoxSphereBounds.h"
#include "Render/SkeletalMesh.h"
#include "Render/LineBatcher.h"

// Bone의 네임스페이스를 제외한 마지막 콜론 뒤의 이름만 반환
FString GetBoneDisplayName(const FString& BoneName)
{
	const auto Separator = BoneName.find_last_of(':');

	if (Separator == FString::npos)
	{
		return BoneName;
	}

	return BoneName.substr(Separator + 1);
}

FSkeletalMeshEditor::FSkeletalMeshEditor(
	UEditorEngine& InEditorEngine,
	FEditorManager& InEditorManager,
	const FString& InAssetPath,
	USkeletalMesh& InMesh
)
	: FAssetEditor(InEditorEngine, InEditorManager, InAssetPath)
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

	// BoneItems를 빌드합니다. (에디터 UI에서 본 계층 구조를 표시하기 위해)
	BuildBoneItems();

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

void FSkeletalMeshEditor::OnRenderPreview()
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
	EditorEngine.RenderAssetEditor(*this);
}

void FSkeletalMeshEditor::SelectBone(int32 BoneIndex)
{
	if (BoneIndex >= -1 && BoneIndex < BoneItems.Num())
	{
		SelectedBoneIndex = BoneIndex;
	}
}

bool FSkeletalMeshEditor::GetSelectedBoneLocalTransform(FTransform& OutTransform) const
{
	if (!MeshComponent || SelectedBoneIndex < 0 || SelectedBoneIndex >= BoneItems.Num())
	{
		return false;
	}

	OutTransform = MeshComponent->GetBoneLocalTransform(SelectedBoneIndex);
	return true;
}

void FSkeletalMeshEditor::SetSelectedBoneLocalTransform(const FTransform& Transform)
{
	if (!MeshComponent || SelectedBoneIndex < 0 || SelectedBoneIndex >= BoneItems.Num())
	{
		return;
	}

	// 원본 애셋 대신 프리뷰 컴포넌트의 포즈만 수정합니다. 본 갱신은 다음 World Tick에서 수행합니다.
	MeshComponent->SetBoneLocalTransform(SelectedBoneIndex, Transform);
}

void FSkeletalMeshEditor::OnRenderLine(FLineBatcher& LineBatcher)
{
	if (!Mesh)
	{
		return;
	}
	
	if (!MeshComponent)
	{
		return;
	}

	const TArray<FBoneInfo>& Bones = Mesh->GetRefSkeleton();

	TArray<FVector> Positions;
	Positions.SetNum(Bones.Num());

	// 각 Bone의 원점을 월드 공간으로 변환하여 Positions에 저장
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		const FVector4 Position = FVector4(0.0f, 0.0f, 0.0f, 1.0f) * MeshComponent->GetBoneWorldMatrix(Index);
		Positions[Index] = FVector(Position.X, Position.Y, Position.Z);
	}

	// 메시 컴포넌트에서 AABB Bound를 계산하고, 그 크기를 기준으로 Bone 원점과 Bone 연결선을 렌더링합니다.
	const FBox Bounds = MeshComponent->CalcLocalBounds().GetWorldAABB(MeshComponent->GetWorldMatrix());
	const float Radius = FMath::Max((Bounds.Max - Bounds.Min).Size() * 0.0025f, 0.001f);

	// Bone 시각화 그리기
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		const FVector4 Color = Index == SelectedBoneIndex
			? FVector4(1.0f, 1.0f, 0.0f, 1.0f)
			: FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		
		// Joint 먼저 그리기
		LineBatcher.AddSphere(Positions[Index], Radius, Color);
		
		const int32 ParentIndex = Bones[Index].ParentIndex;

		// 부모가 있다면, 부모에서 자식으로 이어지는 Bone을 그리기
		if (ParentIndex >= 0 && ParentIndex < Bones.Num())
		{
			FVector4 BoneColor(0.0f, 0.0f, 0.0f, 1.0f);
			if (Index == SelectedBoneIndex)
			{
				// 현재 본이 선택한 본이면 부모 본을 노란색으로 표시
				BoneColor = FVector4(1.0f, 0.5f, 0.0f, 1.0f);
			}
			else if (ParentIndex == SelectedBoneIndex)
			{
				// 부모 Index가 선택된 본이면 부모 본을 주황색으로 표시
				BoneColor = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
			}

			const float Length = (Positions[Index] - Positions[ParentIndex]).Size();
			const float HalfWidth = FMath::Min(Radius * 1.5f, Length * 0.1f);
			LineBatcher.AddPyramid(Positions[ParentIndex], Positions[Index], HalfWidth, BoneColor);
		}
	}
}

void FSkeletalMeshEditor::ShutdownResources()
{
	MeshComponent = nullptr;
	Mesh = nullptr;
	FAssetEditor::ShutdownResources();
}

void FSkeletalMeshEditor::BuildBoneItems()
{
	if (!Mesh)
	{
		return;
	}

	const TArray<FBoneInfo>& BoneData = Mesh->GetRefSkeleton();
	BoneItems.Empty();
	RootBoneIndices.Empty();
	SelectedBoneIndex = -1;

	for (int32 BoneIndex = 0; BoneIndex < BoneData.Num(); ++BoneIndex)
	{
		const FBoneInfo& Bone = BoneData[BoneIndex];

		BoneItems.Add(FEditorBoneItem{
			.BoneIndex = BoneIndex,
			.Name = GetBoneDisplayName(Bone.Name),
			.ChildBoneIndices = {},
			.InverseBindPose = Bone.InverseBindPose
		});

		FEditorBoneItem& BoneItem = BoneItems[BoneIndex];

		if (Bone.ParentIndex == -1)
		{
			RootBoneIndices.Add(BoneIndex);
		}
		else if (Bone.ParentIndex >= 0)
		{
			BoneItems[Bone.ParentIndex].ChildBoneIndices.Add(BoneIndex);
		}
	}
}
