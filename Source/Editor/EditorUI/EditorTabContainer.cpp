#include "EnginePCH.h"
#include "Editor/EditorUI/EditorTabContainer.h"

void FEditorTabContainer::AddTab(IEditorTab* Tab)
{
	if (!Tab)
		return;

	for (IEditorTab* Existing : Tabs)
	{
		if (Existing == Tab)
			return;
	}

	Tabs.Add(Tab);
}

void FEditorTabContainer::RemoveTab(IEditorTab* Tab)
{
	// 소유 객체는 UI 순회와 ImGui 렌더링이 끝난 뒤에만 파괴한다.
	assert(!bRendering);
	for (int32 Index = PendingCloseTabs.Num() - 1; Index >= 0; --Index)
	{
		if (PendingCloseTabs[Index] == Tab)
			PendingCloseTabs.RemoveAt(Index);
	}
	for (int32 Index = Tabs.Num() - 1; Index >= 0; --Index)
	{
		if (Tabs[Index] == Tab)
			Tabs.RemoveAt(Index);
	}
	if (MainSelectedTab == Tab) MainSelectedTab = nullptr;
	if (FocusedTab == Tab) FocusedTab = nullptr;
	if (PendingActivation == Tab) PendingActivation = nullptr;
}

void FEditorTabContainer::ActivateTab(IEditorTab* Tab)
{
	for (IEditorTab* Existing : Tabs)
	{
		if (Existing == Tab)
		{
			PendingActivation = Tab;
			return;
		}
	}
}

void FEditorTabContainer::RequestCloseTab(IEditorTab* Tab)
{
	bool bRegistered = false;
	for (IEditorTab* Existing : Tabs)
		bRegistered |= Existing == Tab;
	if (!bRegistered || !Tab->CanClose())
		return;
	for (IEditorTab* Existing : PendingCloseTabs)
	{
		if (Existing == Tab)
			return;
	}
	PendingCloseTabs.Add(Tab);
}

void FEditorTabContainer::ProcessCloseRequests()
{
	assert(!bRendering);
	TArray<IEditorTab*> Requests = std::move(PendingCloseTabs);
	PendingCloseTabs = {};
	for (IEditorTab* Tab : Requests)
	{
		RemoveTab(Tab);
		Tab->OnCloseRequested();
	}
}

void FEditorTabContainer::DrawMainMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (MainSelectedTab)
			DrawTabMenu(*MainSelectedTab);
		ImGui::EndMenuBar();
	}
}

void FEditorTabContainer::OnRender()
{
	assert(!bRendering);
	bRendering = true;

	// 일반 패널은 각 편집기의 내부 도킹 영역에서만 배치한다.
	ImGuiWindowClass TabClass;
	TabClass.ClassId = ImGui::GetID("EditorTabClass");
	TabClass.DockingAllowUnclassed = false;
	TabClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoDockingSplit
		| ImGuiDockNodeFlags_NoWindowMenuButton;

	const ImGuiID DockSpaceId = ImGui::GetID("EditorTabs");
	ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f),
		ImGuiDockNodeFlags_NoDockingSplit | ImGuiDockNodeFlags_NoWindowMenuButton, &TabClass);

	MainSelectedTab = nullptr;

	for (IEditorTab* Tab : Tabs)
	{
		const bool bCanDetach = Tab->CanDetach();
		ImGui::SetNextWindowClass(&TabClass);
		ImGui::SetNextWindowDockID(DockSpaceId,
			bCanDetach ? ImGuiCond_FirstUseEver : ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);

		// NoUndocking은 같은 노드의 모든 탭에 적용되므로 창별 NoMove로 고정한다.
		const ImGuiWindow* ExistingWindow = ImGui::FindWindowByName(Tab->GetTabName());
		const bool bInMainDockSpace = !bCanDetach || !ExistingWindow
			|| IsInDockSpace(ExistingWindow, DockSpaceId);

		ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse;
		if (!bInMainDockSpace)
			Flags |= ImGuiWindowFlags_MenuBar;
		if (!bCanDetach)
			Flags |= ImGuiWindowFlags_NoMove;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 10.0f));
		const bool bActivate = PendingActivation == Tab;
		if (bActivate)
			ImGui::SetNextWindowFocus();
		bool bOpen = true;
		const bool bVisible = ImGui::Begin(Tab->GetTabName(), Tab->CanClose() ? &bOpen : nullptr, Flags);
		ImGui::PopStyleVar(2);

		if (!bOpen)
		{
			RequestCloseTab(Tab);
			Tab->OnRender(false);
			ImGui::End();
			continue;
		}

		if (bActivate && bVisible)
		{
			ImGuiViewport* Viewport = ImGui::GetWindowViewport();
			if (Viewport->PlatformHandle && ImGui::GetPlatformIO().Platform_SetWindowFocus)
				ImGui::GetPlatformIO().Platform_SetWindowFocus(Viewport);
			PendingActivation = nullptr;
		}

		if (bVisible)
		{
			if (IsInDockSpace(ImGui::GetCurrentWindow(), DockSpaceId))
				MainSelectedTab = Tab;
			else if (ImGui::BeginMenuBar())
			{
				DrawTabMenu(*Tab);
				ImGui::EndMenuBar();
			}
		}

		// 숨겨진 탭도 내부 도킹 노드를 유지할 수 있도록 호출한다.
		Tab->OnRender(bVisible);
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy))
			FocusedTab = Tab;
		ImGui::End();
	}
	bRendering = false;
}

bool FEditorTabContainer::IsInDockSpace(const ImGuiWindow* Window, ImGuiID DockSpaceId)
{
	return Window->DockNode && ImGui::DockNodeGetRootNode(Window->DockNode)->ID == DockSpaceId;
}

void FEditorTabContainer::DrawTabMenu(IEditorTab& Tab)
{
	// 탭 전환 시 이전 탭에서 열었던 메뉴 팝업을 다른 탭이 재사용하지 않는다.
	ImGui::PushID(&Tab);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));
	Tab.DrawMenuBar();
	ImGui::PopStyleVar(2);
	ImGui::PopID();
}
