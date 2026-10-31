#include "EnginePCH.h"
#include "Editor/AssetEditor/EditorManager.h"
#include "Editor/EditorUI/EditorUI.h"
#include "Editor/StaticMeshEditor/StaticMeshEditor.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetUtils.h"
#include "Render/Mesh.h"
#include "Render/SkeletalMesh.h"
#include <filesystem>

FEditorManager::FEditorManager(FEditorUI& InEditorUI)
	: EditorUI(InEditorUI)
{
}

FEditorManager::~FEditorManager()
{
	Shutdown();
}

void FEditorManager::OpenEditor(const FString& AssetPath)
{
	if (bShuttingDown)
	{
		return;
	}

	const FString Path = FAssetUtils::GetAssetKey(AssetPath);
	if (Path.empty())
	{
		HTR_LOG(Warning, "Cannot open an editor for an empty or invalid asset path.");
		return;
	}

	PendingOpens.Add(Path);
}

FAssetEditor* FEditorManager::FindEditor(const FString& Path) const
{
	for (const auto& Editor : Editors)
	{
		if (_stricmp(Editor->GetAssetPath().c_str(), Path.c_str()) == 0)
		{
			return Editor.get();
		}
	}

	return nullptr;
}

void FEditorManager::OpenEditorNow(const FString& AssetPath)
{
	if (FAssetEditor* Existing = FindEditor(AssetPath))
	{
		Existing->Activate();
		return;
	}

	URenderAsset* Asset = UAssetManager::GetAssetByPath<URenderAsset>(AssetPath);
	const FString FilePath = FAssetUtils::ResolveAssetFilePath(AssetPath);
	if (!Asset && !FilePath.empty())
	{
		// 기존 AssetManager는 애셋에 따라 프로젝트 상대 경로로도 등록한다.
		const FString ProjectPath = std::filesystem::path(FilePath)
			.lexically_relative(std::filesystem::current_path()).generic_string();
		Asset = UAssetManager::GetAssetByPath<URenderAsset>(ProjectPath);
		
		if (!Asset)
		{
			Asset = UAssetManager::GetAssetByPath<URenderAsset>(FilePath);
		}
	}

	if (!Asset && std::filesystem::path(AssetPath).extension() == ".obj")
	{
		Asset = UAssetManager::LoadObjStaticMesh(FilePath);
	}

	if (!Asset)
	{
		HTR_LOG(Warning, "Asset is not loaded or could not be loaded: {}", AssetPath.c_str());
		return;
	}


	// Editor 불러오기
	TUniquePtr<FAssetEditor> Editor = nullptr;
	if (Asset->IsA<UStaticMesh>())
	{
		Editor = MakeUnique<FStaticMeshEditor>(EditorUI, AssetPath);
	}

	if (Asset->IsA<USkeletalMesh>())
	{
		Editor = MakeUnique<FSkeletalMeshEditor>(EditorUI, AssetPath);
	}

	// 지원하지 않는 타입
	if (!Editor)
	{
		HTR_LOG(Warning, "Unsupported asset editor type: {}", AssetPath.c_str());
		return;
	}

	// 초기화 실패시 로깅 후 return
	if (!Editor->Initialize())
	{
		Editor->Shutdown();
		HTR_LOG(Warning, "Asset editor initialization failed: {}", AssetPath.c_str());
		return;
	}

	Editors.Add(std::move(Editor));
	HTR_LOG(Info, "Opened asset editor: {}", AssetPath.c_str());
}

void FEditorManager::Tick(float DeltaTime)
{
	for (auto& Editor : Editors)
	{
		if (!Editor->IsCloseRequested())
		{
			Editor->Tick(DeltaTime);
		}
	}
}

void FEditorManager::Render()
{
	for (auto& Editor : Editors)
	{
		if (!Editor->IsCloseRequested())
		{
			Editor->Render();
		}
	}
}

void FEditorManager::ProcessPendingRequests()
{
	// ImGui의 분리 창까지 렌더링한 뒤 호출한다. 이번 프레임 텍스처 참조도 끝난 시점이다.
	for (int32 Index = Editors.Num() - 1; Index >= 0; --Index)
	{
		if (Editors[Index]->IsCloseRequested())
		{
			HTR_LOG(Info, "Closed asset editor: {}", Editors[Index]->GetAssetPath().c_str());
			Editors[Index]->Shutdown();
			Editors.RemoveAt(Index);
		}
	}

	TArray<FString> Requests = std::move(PendingOpens);
	PendingOpens = {};

	for (const FString& Path : Requests)
	{
		OpenEditorNow(Path);
	}
}

void FEditorManager::Shutdown()
{
	if (bShuttingDown)
	{
		return;
	}

	bShuttingDown = true;
	
	PendingOpens.Empty();

	for (auto& Editor : Editors)
	{
		Editor->Shutdown();
	}
	Editors.Empty();
}
