#pragma once

#include "Containers/Array.h"
#include "Editor/AssetEditor/AssetEditor.h"

class FEditorManager
{
public:
	explicit FEditorManager(class UEditorEngine& InEditorEngine, class FEditorUI& InEditorUI);

	// 애셋을 살펴볼 수 있는 새로운 에디터를 요청합니다. 
	void OpenEditor(const FString& AssetPath);
	
	// 에디터 전체의 Tick을 수행합니다.
	void Tick(float DeltaTime);

	// 에디터 전체의 렌더링을 수행합니다.
	void Render();

	// 지연된 요청을 처리합니다. UI 렌더링이 종료된 이후에 실행합니다.
	void LateTick();

	// 매니저를 정리합니다.
	void Shutdown();

	int32 GetEditorCount() const { return Editors.Num(); }

private:
	// 에디터를 생성합니다. 이미 열려 있는 에디터가 있으면 활성화합니다.
	void CreateEditor(const FString& AssetPath);

	FAssetEditor* FindEditor(const FString& Path) const;

	UEditorEngine& EditorEngine;
	FEditorUI& EditorUI;
	
	TArray<TUniquePtr<FAssetEditor>> Editors;
	TArray<FString> PendingOpens;
	bool bShuttingDown = false;
};
