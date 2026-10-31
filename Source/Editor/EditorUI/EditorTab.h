#pragma once

#include "Editor/EditorUI/EditorPanel.h"

class IEditorTab
{
public:
	virtual ~IEditorTab() = default;
	virtual const char* GetTabName() const = 0;
	virtual bool CanDetach() const { return true; }
	virtual void Tick(float DeltaTime) = 0;
	virtual void OnRender(bool bActive) = 0;
};

// 탭은 각 편집기가 소유하며, 등록한 탭의 수명은 컨테이너보다 길어야 한다.
class FEditorTabContainer
{
public:
	void AddTab(IEditorTab* Tab)
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

	void Tick(float DeltaTime)
	{
		for (IEditorTab* Tab : Tabs)
			Tab->Tick(DeltaTime);
	}

	void OnRender()
	{
		// 일반 패널은 각 편집기의 내부 도킹 영역에서만 배치한다.
		ImGuiWindowClass TabClass;
		TabClass.ClassId = ImGui::GetID("EditorTabClass");
		TabClass.DockingAllowUnclassed = false;
		TabClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoDockingSplit
			| ImGuiDockNodeFlags_NoWindowMenuButton;

		const ImGuiID DockSpaceId = ImGui::GetID("EditorTabs");
		ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f),
			ImGuiDockNodeFlags_NoDockingSplit | ImGuiDockNodeFlags_NoWindowMenuButton, &TabClass);

		for (IEditorTab* Tab : Tabs)
		{
			const bool bCanDetach = Tab->CanDetach();
			ImGui::SetNextWindowClass(&TabClass);
			ImGui::SetNextWindowDockID(DockSpaceId,
				bCanDetach ? ImGuiCond_FirstUseEver : ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);

			// NoUndocking은 같은 노드의 모든 탭에 적용되므로 창별 NoMove로 고정한다.
			ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse;
			if (!bCanDetach)
				Flags |= ImGuiWindowFlags_NoMove;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			const bool bVisible = ImGui::Begin(Tab->GetTabName(), nullptr, Flags);
			ImGui::PopStyleVar();

			// 숨겨진 탭도 내부 도킹 노드를 유지할 수 있도록 호출한다.
			Tab->OnRender(bVisible);
			ImGui::End();
		}
	}

private:
	TArray<IEditorTab*> Tabs;
};
