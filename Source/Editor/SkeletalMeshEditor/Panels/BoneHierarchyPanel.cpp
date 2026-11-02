#include "EnginePCH.h"
#include "BoneHierarchyPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Render/SkeletalMesh.h"
#include "Runtime/Render/SkeletalMeshData.h"

FBoneHierarchyPanel::FBoneHierarchyPanel(FSkeletalMeshEditor& InEditor)
	: FSkeletalMeshEditorPanel(InEditor, "Bone Hierarchy")
{
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
		for (int32 RootBoneIndex : Editor.GetRootBoneIndices())
		{
			DrawBoneHierarchy(RootBoneIndex);
		}
	}

	ImGui::EndChild();
}

void FBoneHierarchyPanel::DrawBoneHierarchy(int32 BoneIndex)
{
	ImGuiTreeNodeFlags Flag =
		ImGuiTreeNodeFlags_DefaultOpen |
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	const TArray<FEditorBoneItem>& BoneItems = Editor.GetBoneItems();

	int32 ChildBoneCount = BoneItems[BoneIndex].ChildBoneIndices.Num();
	if (ChildBoneCount == 0)
	{
		Flag |= ImGuiTreeNodeFlags_Leaf;
	}

	const FString& BoneName = BoneItems[BoneIndex].Name;
	if (ImGui::TreeNodeEx(BoneName.c_str(), Flag))
	{
		if (
			ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsItemToggledOpen()
		)
		{
			// TODO: 에디터에 Bone 클릭 함수 호출하고 Gizmo 표시하기..
		}

		for (int32 ChildIndex : BoneItems[BoneIndex].ChildBoneIndices)
		{
			DrawBoneHierarchy(ChildIndex);
		}

		ImGui::TreePop();
	}
}
