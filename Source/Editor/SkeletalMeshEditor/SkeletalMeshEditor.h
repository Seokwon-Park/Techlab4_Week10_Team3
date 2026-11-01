#pragma once

#include "Editor/AssetEditor/AssetEditor.h"

// TODO: 일단 뼈대만 구현
class FSkeletalMeshEditor : public FAssetEditor
{
public:
	FSkeletalMeshEditor(
		UEditorEngine& InEditorEngine,
		FEditorUI& InEditorUI,
		const FString& InAssetPath
	);
	const char* GetEditorTypeName() const override { return "SkeletalMesh"; }
};
