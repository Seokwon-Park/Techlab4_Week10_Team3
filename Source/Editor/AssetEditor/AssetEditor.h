#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"

class FEditorUI;
class IEditorTab;

class FAssetEditor
{
public:
	FAssetEditor(FEditorUI& InEditorUI, const FString& InAssetPath);
	virtual ~FAssetEditor();

	bool Initialize();
	virtual void Tick(float DeltaTime);
	virtual void Render();
	void Activate();
	void RequestClose();
	void Shutdown();

	const FString& GetAssetPath() const { return AssetPath; }
	bool IsCloseRequested() const { return bCloseRequested; }
	virtual const char* GetEditorTypeName() const = 0;

protected:
	virtual bool InitializeResources();
	virtual TUniquePtr<IEditorTab> CreateTab();
	virtual void ShutdownResources();
	FEditorUI& EditorUI;

private:
	FString AssetPath;
	TUniquePtr<IEditorTab> Tab;
	bool bInitialized = false;
	bool bShutdown = false;
	bool bCloseRequested = false;
};
