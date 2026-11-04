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
