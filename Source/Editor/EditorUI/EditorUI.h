#pragma once

#include "Core/Delegate/Delegate.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

#include <functional>

class FEditorUI
{
public:
	bool Init(bool bInUseDockSpace = true, bool bInPassthruCentralNode = false);
	void Tick(float DeltaTime);
	void OnRender();

	template <typename T>
	T* AddEditorPanel()
	{
		return LevelEditorTab.AddEditorPanel<T>();
	}

	FLevelEditorToolbar* GetToolbar() { return LevelEditorTab.GetToolbar(); }

	TDelegate<> OnNewScene;
	TDelegate<> OnOpenScene;
	TDelegate<> OnSaveScene;
	TDelegate<> OnSaveSceneAs;
	TDelegate<> OnPackageGame;
	// 패키징 중이면 true. Build 메뉴 항목을 비활성화하는 데 쓴다.
	void SetIsPackagingQuery(std::function<bool()> InQuery) { IsPackaging = InQuery; }

private:
	bool bUseDockSpace = true;
	bool bPassthruCentralNode = false;

	void DrawMainMenuBar();

	FLevelEditorTab LevelEditorTab;
	FEditorTabContainer TabContainer;

	std::function<bool()> IsPackaging;
};
