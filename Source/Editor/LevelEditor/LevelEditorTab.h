#pragma once

#include "Core/Delegate/Delegate.h"
#include "Editor/EditorUI/EditorTab.h"
#include "Editor/LevelEditor/LevelEditorToolbar.h"

class FLevelEditorTab : public IEditorTab
{
public:
	const char* GetTabName() const override { return "LevelEditor"; }
	bool CanDetach() const override { return false; }
	void DrawMenuBar() override;

	TDelegate<> OnNewScene;
	TDelegate<> OnOpenScene;
	TDelegate<> OnSaveScene;
	TDelegate<> OnSaveSceneAs;
	TDelegate<> OnPackageGame;
	void SetIsPackagingQuery(std::function<bool()> InQuery) { IsPackaging = std::move(InQuery); }

	FLevelEditorToolbar* GetToolbar() { return &Toolbar; }
	TArray<TUniquePtr<IEditorPanel>>& GetPanels() { return Panels; }

	template <typename T>
	T* AddPanel()
	{
		TUniquePtr<T> Panel = MakeUnique<T>();
		T* Result = Panel.get();
		Result->Init();
		Panels.Add(std::move(Panel));
		return Result;
	}

	void Tick(float DeltaTime);

	void OnRender(bool bActive) override;

	void SetDockSpaceOptions(bool bEnabled, bool bPassthru);

private:
	std::function<bool()> IsPackaging;
	FLevelEditorToolbar Toolbar;
	TArray<TUniquePtr<IEditorPanel>> Panels;
	bool bUseDockSpace = true;
	bool bPassthruCentralNode = false;
};