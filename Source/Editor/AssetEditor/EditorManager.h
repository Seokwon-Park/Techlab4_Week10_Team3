#pragma once

#include "Containers/Array.h"
#include "Editor/AssetEditor/AssetEditor.h"

class FEditorManager
{
public:
	explicit FEditorManager(FEditorUI& InEditorUI);
	~FEditorManager();

	// 요청은 프레임 종료 시 처리하여 UI/편집기 배열 순회와 충돌하지 않는다.
	void OpenEditor(const FString& AssetPath);
	void Tick(float DeltaTime);
	void Render();
	void ProcessPendingRequests();
	void Shutdown();

	int32 GetEditorCount() const { return Editors.Num(); }

private:
	void OpenEditorNow(const FString& AssetPath);
	FAssetEditor* FindEditor(const FString& Path) const;

	FEditorUI& EditorUI;
	TArray<TUniquePtr<FAssetEditor>> Editors;
	TArray<FString> PendingOpens;
	bool bShuttingDown = false;
};
