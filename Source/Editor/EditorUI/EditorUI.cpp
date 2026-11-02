#include "EnginePCH.h"
#include "Editor/EditorUI/EditorUI.h"

bool FEditorUI::Init(bool bInUseDockSpace, bool bInPassthruCentralNode)
{
	bUseDockSpace = bInUseDockSpace;
	bPassthruCentralNode = bInPassthruCentralNode;

	LevelEditorTab.SetDockSpaceOptions(bUseDockSpace, bPassthruCentralNode);
	LevelEditorTab.OnNewScene.Add([this]() { OnNewScene.Broadcast(); });
	LevelEditorTab.OnOpenScene.Add([this]() { OnOpenScene.Broadcast(); });
	LevelEditorTab.OnSaveScene.Add([this]() { OnSaveScene.Broadcast(); });
	LevelEditorTab.OnSaveSceneAs.Add([this]() { OnSaveSceneAs.Broadcast(); });
	LevelEditorTab.OnImportFbx.Add([this](EFbxImportMode Mode) { OnImportFbx.Broadcast(Mode); });
	LevelEditorTab.OnPackageGame.Add([this]() { OnPackageGame.Broadcast(); });

	TabContainer.AddTab(&LevelEditorTab);

	return true;
}

void FEditorUI::Tick(float DeltaTime)
{
	LevelEditorTab.Tick(DeltaTime);
}

void FEditorUI::OnRender()
{
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);

	ImGuiWindowFlags Flags = ImGuiWindowFlags_MenuBar
		| ImGuiWindowFlags_NoDocking
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
	// 메뉴 높이를 먼저 확보하고 탭 선택이 확정된 후 내용을 그린다.
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 10.0f));
	ImGui::Begin("DockingSpace", nullptr, Flags);
	ImGui::PopStyleVar(4);

	TabContainer.OnRender();
	DrawMainMenuBar();
	ImGui::End();
	TabContainer.ProcessCloseRequests();
}

// 메인 도킹 영역에서 선택된 탭의 메뉴를 공통 상단에 표시한다.
void FEditorUI::DrawMainMenuBar()
{
	TabContainer.DrawMainMenuBar();
}
