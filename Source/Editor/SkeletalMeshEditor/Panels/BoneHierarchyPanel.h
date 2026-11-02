#pragma once

#include "SkeletalMeshEditorPanel.h"
#include "Containers/Array.h"

// 스켈레탈 메시의 본 계층을 표시하는 패널입니다.
class FBoneHierarchyPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FBoneHierarchyPanel(FSkeletalMeshEditor& InEditor);

protected:
	void DrawContent() override;

private:
	void BuildChildBoneIndices();
	void DrawBoneHierarchy(int32 BoneIndex);

	const TArray<struct FBoneInfo>* BoneData;

	// 부모 본 인덱스를 키로, 자식 본 인덱스 배열을 값으로 매핑하는 자료구조
	TArray<TArray<int32>> ChildBoneIndices; 

	// 루트 본의 인덱스들. 여러개의 RootBone이 있을 수 있으므로 배열로 관리합니다.
	TArray<int32> RootBoneIndices; 
};
