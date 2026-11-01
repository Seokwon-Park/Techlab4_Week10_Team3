#include "EnginePCH.h"
#include "StaticMeshEditorTab.h"
#include "StaticMeshEditor.h"
#include "Panels/StaticMeshViewportPanel.h"
#include "Panels/StaticMeshInfoPanel.h"
#include "Panels/StaticMeshSettingsPanel.h"

FStaticMeshEditorTab::FStaticMeshEditorTab(FStaticMeshEditor& InEditor)
	: FAssetEditorTab(InEditor)
	, MeshEditor(InEditor)
{
	// 탭의 부모 창이 달라져도 애셋별 도킹 ID를 유지합니다.
	const ImGuiID AssetId = ImHashStr(InEditor.GetAssetPath().c_str());
	DockSpaceId = ImHashStr("StaticMeshDockSpace", 0, AssetId);
	PanelClassId = ImHashStr("StaticMeshPanelClass", 0, AssetId);

	Panels.Add(MakeUnique<FStaticMeshViewportPanel>(InEditor));
	Panels.Add(MakeUnique<FStaticMeshInfoPanel>(InEditor));
	Panels.Add(MakeUnique<FStaticMeshSettingsPanel>(InEditor));
	for (auto& Panel : Panels)
	{
		Panel->SetDockClass(PanelClassId);
		Panel->Init();
	}
}

void FStaticMeshEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("Viewer"))
	{
		if (ImGui::MenuItem("Focus Mesh", "F"))
		{
			MeshEditor.FocusMesh();
		}

		if (ImGui::MenuItem("Close Viewer"))
		{
			MeshEditor.Close();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, Panel->IsOpen()))
			{
				Panel->SetOpen(!Panel->IsOpen());
			}
		}
		ImGui::EndMenu();
	}
}

void FStaticMeshEditorTab::BuildDefaultLayout(ImGuiID InDockSpaceId, const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(InDockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(InDockSpaceId, Size);
	ImGuiID ViewportId = InDockSpaceId;
	ImGuiID RightId = ImGui::DockBuilderSplitNode(ViewportId, ImGuiDir_Right, 0.28f, nullptr, &ViewportId);
	ImGuiID InfoId = RightId;
	const ImGuiID SettingsId = ImGui::DockBuilderSplitNode(InfoId, ImGuiDir_Down, 0.4f, nullptr, &InfoId);

	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), ViewportId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), InfoId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), SettingsId);
	ImGui::DockBuilderFinish(InDockSpaceId);
}

void FStaticMeshEditorTab::OnRender(bool bActive)
{
	MeshEditor.SetViewportState(0, 0, {});
	if (MeshEditor.IsCloseRequested())
	{
		return;
	}

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		const ImVec2 Size = ImGui::GetContentRegionAvail();
		if (bActive && Size.x > 0.0f && Size.y > 0.0f && !ImGui::DockBuilderGetNode(DockSpaceId))
		{
			BuildDefaultLayout(DockSpaceId, Size);
		}

		ImGuiWindowClass WindowClass;
		WindowClass.ClassId = PanelClassId;
		WindowClass.DockingAllowUnclassed = false;
		const ImGuiDockNodeFlags Flags = bActive ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly;
		if (bActive || ImGui::DockBuilderGetNode(DockSpaceId))
		{
			ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags, &WindowClass);
		}
	}

	for (auto& Panel : Panels)
	{
		// 분리된 패널은 부모 탭이 가려져도 독립적으로 표시합니다.
		if (Panel->IsOpen() && (bActive || Panel->IsDetached(DockSpaceId)))
		{
			Panel->OnRender();
		}
	}
}
