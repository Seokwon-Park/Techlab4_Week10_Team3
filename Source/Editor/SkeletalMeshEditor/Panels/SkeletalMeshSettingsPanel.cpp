#include "EnginePCH.h"
#include "SkeletalMeshSettingsPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

FSkeletalMeshSettingsPanel::FSkeletalMeshSettingsPanel(FSkeletalMeshEditor& InEditor)
	: FSkeletalMeshEditorPanel(InEditor, "Settings")
{
}

void FSkeletalMeshSettingsPanel::DrawContent()
{
	if (ImGui::Button("Reset All Transform"))
	{
		Editor.ResetPose();
	}

	FVector4 BoneVisualizerColor = Editor.GetBoneVisualizerColor();
	ImGui::TextUnformatted("Bone Visualizer Color");
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::ColorEdit3("##BoneVisualizerColor", &BoneVisualizerColor.X))
	{
		Editor.SetBoneVisualizerColor(BoneVisualizerColor);
	}

	bool bShowBoneVisualizer = Editor.IsBoneVisualizerVisible();
	if (ImGui::Checkbox("Show Bone Visualizer", &bShowBoneVisualizer))
	{
		Editor.SetBoneVisualizerVisible(bShowBoneVisualizer);
	}

	if (FEditorViewportClient* Client = Editor.GetViewportClient())
	{
		bool bWireframe = Client->IsWireframe();
		if (ImGui::Checkbox("Wireframe", &bWireframe))
		{
			Client->SetViewportMode(bWireframe ? EViewportMode::Wireframe : EViewportMode::Solid);
		}
	}
}
