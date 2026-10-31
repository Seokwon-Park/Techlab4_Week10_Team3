#include "EnginePCH.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

void FLevelEditorTab::DrawMenuBar()
{
	////////////////////////////////////////////////////////////
	// File 탭
	////////////////////////////////////////////////////////////

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Scene")) { OnNewScene.Broadcast(); }
		if (ImGui::MenuItem("Open Scene...")) { OnOpenScene.Broadcast(); }

		ImGui::Separator();

		if (ImGui::MenuItem("Save")) { OnSaveScene.Broadcast(); }
		if (ImGui::MenuItem("Save As...")) { OnSaveSceneAs.Broadcast(); }

		ImGui::EndMenu();
	}



	////////////////////////////////////////////////////////////
	// Window 탭
	////////////////////////////////////////////////////////////

	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			bool bOpen = Panel->IsOpen();

			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, bOpen))
			{
				Panel->SetOpen(!bOpen);
			}
		}
		ImGui::EndMenu();
	}



	////////////////////////////////////////////////////////////
	// Build 탭
	////////////////////////////////////////////////////////////

	if (ImGui::BeginMenu("Build"))
	{
		const bool bCanPackage = !IsPackaging || !IsPackaging();
		if (ImGui::MenuItem("Package Game (Release)", nullptr, false, bCanPackage)) { OnPackageGame.Broadcast(); }
		ImGui::EndMenu();
	}
}

void FLevelEditorTab::Tick(float DeltaTime)
{
	for (auto& Panel : Panels)
		Panel->Tick(DeltaTime);
}

void FLevelEditorTab::OnRender(bool bActive)
{
	if (bActive)
		Toolbar.Draw();

	if (bUseDockSpace && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable))
	{
		// 부모 창이 달라져도 기존 패널 도킹 ID는 유지한다.
		const ImGuiID DockSpaceId = ImHashStr("EngineDockingSpace", 0, ImHashStr("DockingSpace"));
		ImGuiDockNodeFlags Flags = bPassthruCentralNode
			? ImGuiDockNodeFlags_PassthruCentralNode : ImGuiDockNodeFlags_None;
		if (!bActive)
			Flags |= ImGuiDockNodeFlags_KeepAliveOnly;

		ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags);
	}

	if (bActive)
	{
		for (auto& Panel : Panels)
		{
			if (Panel->IsOpen())
				Panel->OnRender();
		}
	}
}

void FLevelEditorTab::SetDockSpaceOptions(bool bEnabled, bool bPassthru)
{
	bUseDockSpace = bEnabled;
	bPassthruCentralNode = bPassthru;
}
