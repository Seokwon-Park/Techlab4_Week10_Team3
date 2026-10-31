#include "EnginePCH.h"
#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include <filesystem>

FAssetEditorTab::FAssetEditorTab(FAssetEditor& InEditor) : Editor(InEditor)
{
	const FString Filename = std::filesystem::path(Editor.GetAssetPath()).filename().string();
	TabName = Filename + " (" + Editor.GetEditorTypeName() + ")###AssetEditor:" + Editor.GetAssetPath();
}

void FAssetEditorTab::OnCloseRequested()
{
	Editor.RequestClose();
}

void FAssetEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("Editor"))
	{
		if (ImGui::MenuItem("Close Editor"))
		{
			Editor.RequestClose();
		}

		ImGui::EndMenu();
	}
}

void FAssetEditorTab::OnRender(bool bActive)
{
}
