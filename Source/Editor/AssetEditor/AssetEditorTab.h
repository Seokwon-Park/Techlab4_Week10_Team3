#pragma once

#include "Editor/EditorUI/EditorTab.h"
#include "Core/EngineString.h"

class FAssetEditor;

class FAssetEditorTab : public IEditorTab
{
public:
	explicit FAssetEditorTab(FAssetEditor& InEditor);
	const char* GetTabName() const override { return TabName.c_str(); }
	bool CanClose() const override { return true; }
	void OnCloseRequested() override;
	void DrawMenuBar() override;
	void OnRender(bool bActive) override;

private:
	FAssetEditor& Editor;
	FString TabName;
};
