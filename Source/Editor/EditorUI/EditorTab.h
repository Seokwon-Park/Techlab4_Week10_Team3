#pragma once

#include "Editor/EditorUI/EditorPanel.h"

class IEditorTab
{
public:
	virtual ~IEditorTab() = default;
	virtual const char* GetTabName() const = 0;
	virtual void Tick(float DeltaTime) = 0;
	virtual void OnRender(bool bActive) = 0;
};

// 탭은 각 편집기가 소유하며, 컨테이너는 선택과 렌더링 순서를 관리한다.
class FEditorTabContainer
{
public:
	void AddTab(IEditorTab* Tab) { Tabs.Add(Tab); }

	void Tick(float DeltaTime)
	{
		for (IEditorTab* Tab : Tabs)
			Tab->Tick(DeltaTime);
	}

	void OnRender()
	{
		if (ImGui::BeginTabBar("EditorTabs", ImGuiTabBarFlags_Reorderable))
		{
			for (IEditorTab* Tab : Tabs)
			{
				// 닫기 버튼을 넘기지 않아 LevelEditor 탭을 항상 유지한다.
				if (ImGui::BeginTabItem(Tab->GetTabName()))
				{
					ActiveTab = Tab;
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		for (IEditorTab* Tab : Tabs)
			Tab->OnRender(Tab == ActiveTab);
	}

private:
	TArray<IEditorTab*> Tabs;
	IEditorTab* ActiveTab = nullptr;
};
