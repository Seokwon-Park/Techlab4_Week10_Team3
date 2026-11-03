#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Camera/ViewportCameraMovement.h"

struct FEditorBoneItem
{
	int32 BoneIndex;
	FString Name;
	
	// 자식 Bone Index 배열. 계층 구조 그릴 때 사용
	TArray<int32> ChildBoneIndices;

	// BindPose 행렬의 역행렬. Gizmo 그릴 때 사용
	FMatrix InverseBindPose;
};

class FSkeletalMeshEditor : public FAssetEditor
{
public:
	FSkeletalMeshEditor(
		class UEditorEngine& InEditorEngine,
		class FEditorManager& InEditorManager,
		const FString& InAssetPath,
		class USkeletalMesh& InMesh
	);

	////////////////////////////////////////////////////////////
	// 에디터 공통 인터페이스
	////////////////////////////////////////////////////////////

	// 틱을 돌립니다.
	void Tick(float DeltaTime) override;

	// 현재 상태를 렌더링합니다.
	void OnRenderPreview() override;

	// 현재 상태에서 라인을 렌더링합니다. (Gizmo, BoneHierarchy 등)
	void OnRenderLine(class FLineBatcher& LineBatcher) override;

	////////////////////////////////////////////////////////////
	// 에디터 전용 함수
	////////////////////////////////////////////////////////////

	// 카메라를 메시에 포커싱합니다.
	void FocusMesh();

	// 본을 선택합니다. -1은 선택을 해제합니다.
	void SelectBone(int32 BoneIndex);

	// 선택한 본의 프리뷰 포즈를 부모 기준 로컬 변환으로 조회·수정합니다.
	bool GetSelectedBoneLocalTransform(FTransform& OutTransform) const;
	void SetSelectedBoneLocalTransform(const FTransform& Transform);

	// 스켈레톤의 상태를 초기화합니다.
	void ResetPose();

	// 본의 상태를 초기화합니다.
	void ResetBone(int32 BoneIndex);

	// 뷰포트 상태를 설정합니다. (뷰포트가 보이지 않으면 입력을 무시합니다)
	void SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input);


	////////////////////////////////////////////////////////////
	// Getter
	////////////////////////////////////////////////////////////

	const char* GetEditorTypeName() const override { return "SkeletalMesh"; }
	USkeletalMesh* GetMesh() const { return Mesh; }

	const TArray<int32>& GetRootBoneIndices() const { return RootBoneIndices; }
	const TArray<FEditorBoneItem>& GetBoneItems() const { return BoneItems; }
	int32 GetSelectedBoneIndex() const { return SelectedBoneIndex; }

protected:
	// 이 에디터에 연결된 탭을 생성합니다.
	TUniquePtr<class IEditorTab> CreateTab() override;

	// 리소스를 초기화합니다.
	bool InitializeResources() override;

	// 리소스를 정리합니다.
	void ShutdownResources() override;

	// UI, 에디터에서 사용할 수 있도록 BoneItems를 빌드합니다.
	void BuildBoneItems();

private:
	class USkeletalMesh* Mesh = nullptr;
	class USkeletalMeshComponent* MeshComponent = nullptr;

	FViewportCameraInput CameraInput;

	uint32 ViewportWidth = 0;
	uint32 ViewportHeight = 0;

	TArray<FEditorBoneItem> BoneItems;
	TArray<int32> RootBoneIndices;
	int32 SelectedBoneIndex = -1;

	bool bViewportVisible = false;
	bool bInitialFocusPending = true;
};
