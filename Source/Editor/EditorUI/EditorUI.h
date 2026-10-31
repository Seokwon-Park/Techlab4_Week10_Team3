#pragma once

#include "Core/Delegate/Delegate.h"
#include "Editor/EditorUI/EditorTabContainer.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

#include <functional>

class FEditorUI
{
public:
	bool Init(bool bInUseDockSpace = true, bool bInPassthruCentralNode = false);
	void Tick(float DeltaTime);
	void OnRender();

	// 같은 애셋의 중복 생성 여부와 탭의 수명은 호출자가 관리한다.
	void AddEditorTab(IEditorTab* Tab) { TabContainer.AddTab(Tab); }

	IEditorTab* GetMainSelectedTab() const { return TabContainer.GetMainSelectedTab(); }
	IEditorTab* GetFocusedTab() const { return TabContainer.GetFocusedTab(); }

	FLevelEditorTab& GetLevelEditorTab() { return LevelEditorTab; }

	TDelegate<> OnNewScene;
	TDelegate<> OnOpenScene;
	TDelegate<> OnSaveScene;
	TDelegate<> OnSaveSceneAs;
	TDelegate<> OnPackageGame;
	// 패키징 중이면 true. Build 메뉴 항목을 비활성화하는 데 쓴다.
	void SetIsPackagingQuery(std::function<bool()> InQuery) { LevelEditorTab.SetIsPackagingQuery(std::move(InQuery)); }

private:
	bool bUseDockSpace = true;
	bool bPassthruCentralNode = false;

	void DrawMainMenuBar();

	FLevelEditorTab LevelEditorTab;
	FEditorTabContainer TabContainer;
};
