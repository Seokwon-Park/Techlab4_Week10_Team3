#include "EnginePCH.h"
#include "BoneHierarchyPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Render/SkeletalMesh.h"
#include "Runtime/Render/SkeletalMeshData.h"

FBoneHierarchyPanel::FBoneHierarchyPanel(FSkeletalMeshEditor& InEditor)
	: FSkeletalMeshEditorPanel(InEditor, "Bone Hierarchy")
	, BoneData(&InEditor.GetMesh()->GetRefSkeleton())
{
	BuildChildBoneIndices();
}

void FBoneHierarchyPanel::DrawContent()
{
	USkeletalMesh* Mesh = Editor.GetMesh();
	if (!Mesh)
	{
		return;
	}

	if (ImGui::BeginChild("BoneHierarchyChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
	{
		for (int32 RootBoneIndex : RootBoneIndices)
		{
			DrawBoneHierarchy(RootBoneIndex);
		}
	}

	ImGui::EndChild();
}

void FBoneHierarchyPanel::BuildChildBoneIndices()
{
	ChildBoneIndices.SetNum(BoneData->Num());

	for (int32 BoneIndex = 0; BoneIndex < BoneData->Num(); ++BoneIndex)
	{
		const FBoneInfo& Bone = (*BoneData)[BoneIndex];
		if (Bone.ParentIndex == -1)
		{
			RootBoneIndices.Add(BoneIndex);
		}
		else if (Bone.ParentIndex >= 0)
		{
			ChildBoneIndices[Bone.ParentIndex].Add(BoneIndex);
		}
	}
}

void FBoneHierarchyPanel::DrawBoneHierarchy(int32 BoneIndex)
{
	ImGuiTreeNodeFlags Flag =
		ImGuiTreeNodeFlags_DefaultOpen |
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	int32 ChildBoneCount = ChildBoneIndices[BoneIndex].Num();
	if (ChildBoneCount == 0)
	{
		Flag |= ImGuiTreeNodeFlags_Leaf;
	}

	const FString& BoneName = (*BoneData)[BoneIndex].Name;
	if (ImGui::TreeNodeEx(BoneName.c_str(), Flag))
	{
		if (
			ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsItemToggledOpen()
		)
		{
			// TODO: 에디터에 Bone 클릭 함수 호출하고 Gizmo 표시하기..
		}

		for (int32 ChildIndex : ChildBoneIndices[BoneIndex])
		{
			DrawBoneHierarchy(ChildIndex);
		}

		ImGui::TreePop();
	}
}
