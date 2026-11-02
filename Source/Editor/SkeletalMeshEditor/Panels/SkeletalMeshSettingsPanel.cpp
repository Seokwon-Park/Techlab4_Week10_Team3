#include "EnginePCH.h"
#include "SkeletalMeshSettingsPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Math/Transform.h"

FSkeletalMeshSettingsPanel::FSkeletalMeshSettingsPanel(FSkeletalMeshEditor& InEditor)
	: FSkeletalMeshEditorPanel(InEditor, "Settings")
{
}

void FSkeletalMeshSettingsPanel::DrawContent()
{
	if (FEditorViewportClient* Client = Editor.GetViewportClient())
	{
		bool bWireframe = Client->IsWireframe();
		if (ImGui::Checkbox("Wireframe", &bWireframe))
		{
			Client->SetViewportMode(bWireframe ? EViewportMode::Wireframe : EViewportMode::Solid);
		}
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Local Bone Transform");
	FTransform Transform;
	if (!Editor.GetSelectedBoneLocalTransform(Transform))
	{
		ImGui::TextDisabled("Select a bone in Bone Hierarchy.");
		return;
	}

	const int32 BoneIndex = Editor.GetSelectedBoneIndex();
	ImGui::TextWrapped("Bone: %s", Editor.GetBoneItems()[BoneIndex].Name.c_str());
	ImGui::TextDisabled("Relative to parent | Rotation: Pitch / Yaw / Roll (deg)");

	// 선택이 바뀌면 이전 본의 편집 상태가 새 본에 이어지지 않도록 ID를 구분합니다.
	ImGui::PushID(BoneIndex);
	bool bChanged = ImGui::DragFloat3("Location", Transform.Location.V, 0.1f);
	bChanged |= ImGui::DragFloat3("Rotation", Transform.Rotation.V, 0.5f);
	bChanged |= ImGui::DragFloat3("Scale", Transform.Scale.V, 0.01f);
	ImGui::PopID();

	if (bChanged)
	{
		Editor.SetSelectedBoneLocalTransform(Transform);
	}
}
