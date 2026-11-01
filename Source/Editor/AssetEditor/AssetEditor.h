#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include "Editor/AssetEditor/FPreviewScene.h"

class FAssetEditor
{
public:
	FAssetEditor(
		class UEditorEngine& InEditorEngine,
		class FEditorUI& InEditorUI,
		const FString& InAssetPath
	);
	virtual ~FAssetEditor();

	// Editor를 초기화합니다.
	bool Initialize();

	// 틱을 돌립니다.
	virtual void Tick(float DeltaTime);
	
	// 현재 상태를 렌더링합니다.
	virtual void Render();

	// Editor를 포커싱합니다.
	void Activate();

	// Editor를 종료합니다.
	void Close();

	// Editor를 정리합니다.
	void Shutdown();


	////////////////////////////////////////////////////////////
	// Getter
	////////////////////////////////////////////////////////////

	const FString& GetAssetPath() const { return AssetPath; }
	FPreviewScene& GetPreviewScene() { return PreviewScene; }

	bool IsCloseRequested() const { return bCloseRequested; }
	virtual const char* GetEditorTypeName() const = 0;

protected:
	// 리소스를 초기화합니다.
	virtual bool InitializeResources();

	// 이 에디터에 연결된 탭을 생성합니다.
	virtual TUniquePtr<class IEditorTab> CreateTab();
	
	// 리소스를 정리합니다.
	virtual void ShutdownResources();

	UEditorEngine& EditorEngine;
	FEditorUI& EditorUI;
	FPreviewScene PreviewScene;

private:
	FString AssetPath;
	TUniquePtr<class IEditorTab> Tab;
	bool bInitialized = false;
	bool bShutdown = false;
	bool bCloseRequested = false;
};
