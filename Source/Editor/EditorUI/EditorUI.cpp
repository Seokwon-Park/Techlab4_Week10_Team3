#include "EnginePCH.h"
#include "Editor/EditorUI/EditorUI.h"


bool FEditorUI::Init(bool bInUseDockSpace, bool bInPassthruCentralNode)
{
	bUseDockSpace = bInUseDockSpace;
	bPassthruCentralNode = bInPassthruCentralNode;

	LevelEditorTab.SetDockSpaceOptions(bUseDockSpace, bPassthruCentralNode);
	TabContainer.AddTab(&LevelEditorTab);
	return true;
}

void FEditorUI::Tick(float DeltaTime)
{
	TabContainer.Tick(DeltaTime);
}

void FEditorUI::OnRender()
{
	DrawMainMenuBar();

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);

	ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus;

	if (bUseDockSpace && bPassthruCentralNode)
		Flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockingSpace", nullptr, Flags);
	ImGui::PopStyleVar(3);

	TabContainer.OnRender();
	ImGui::End();
}

// 파일 메뉴의 요청을 애플리케이션 델리게이트로 전달한다.
void FEditorUI::DrawMainMenuBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene")) { OnNewScene.Broadcast(); }
			if (ImGui::MenuItem("Open Scene...")) { OnOpenScene.Broadcast(); }

			ImGui::Separator();

			if (ImGui::MenuItem("Save")) { OnSaveScene.Broadcast(); }
			if (ImGui::MenuItem("Save As...")) { OnSaveSceneAs.Broadcast(); }

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			for (auto& Panel : LevelEditorTab.GetPanels())
			{
				bool bOpen = Panel->IsOpen();

				if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, bOpen))
				{
					Panel->SetOpen(!bOpen);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Build"))
		{
			const bool bCanPackage = !IsPackaging || !IsPackaging();
			if (ImGui::MenuItem("Package Game (Release)", nullptr, false, bCanPackage)) { OnPackageGame.Broadcast(); }
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar(2);
}
