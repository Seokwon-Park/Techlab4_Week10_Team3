#include "EnginePCH.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/EditorUI/EditorUI.h"
#include "Editor/HitoriEd/EditorEngine.h"

FAssetEditor::FAssetEditor(
	UEditorEngine& InEditorEngine,
	FEditorUI& InEditorUI,
	const FString& InAssetPath
)
	: EditorEngine(InEditorEngine)
	, EditorUI(InEditorUI)
	, AssetPath(InAssetPath)
{
}

FAssetEditor::~FAssetEditor()
{
	// 파생 클래스의 자원 정리는 Manager가 소멸 전에 Shutdown으로 수행한다.
	if (Tab)
	{
		EditorUI.RemoveEditorTab(Tab.get());
	}
}

bool FAssetEditor::Initialize()
{
	if (bInitialized)
	{
		return true;
	}

	if (bCloseRequested)
	{
		return false;
	}

	if (!InitializeResources())
	{
		Shutdown();
		return false;
	}

	Tab = CreateTab();
	if (!Tab)
	{
		Shutdown();
		return false;
	}

	EditorUI.AddEditorTab(Tab.get());
	bInitialized = true;
	Activate();
	return true;
}

void FAssetEditor::Tick(float DeltaTime)
{
	PreviewScene.Tick(DeltaTime);
}

void FAssetEditor::Render()
{
	// 프리뷰 장면과 뷰포트 렌더링은 후속 단계에서 연결한다.
}

void FAssetEditor::Activate()
{
	if (Tab && !bCloseRequested)
	{
		EditorUI.ActivateEditorTab(Tab.get());
	}
}

void FAssetEditor::Close()
{
	if (bCloseRequested)
	{
		return;
	}

	bCloseRequested = true;
	if (Tab)
	{
		EditorUI.RequestCloseEditorTab(Tab.get());
	}
}

void FAssetEditor::Shutdown()
{
	if (bShutdown)
	{
		return;
	}

	bShutdown = true;
	bCloseRequested = true;
	if (Tab)
	{
		EditorUI.RemoveEditorTab(Tab.get());
		Tab.reset();
	}
	ShutdownResources();
	bInitialized = false;
}

bool FAssetEditor::InitializeResources()
{
	if (!PreviewScene.Initialize(&EditorEngine))
	{
		return false;
	}

	// Scene 세팅
	PreviewScene.Setup();

	return true;
}

TUniquePtr<IEditorTab> FAssetEditor::CreateTab()
{
	return MakeUnique<FAssetEditorTab>(*this);
}

void FAssetEditor::ShutdownResources()
{
	PreviewScene.Release();
}
