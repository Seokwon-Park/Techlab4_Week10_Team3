#pragma once

#include "Editor/AssetEditor/AssetEditor.h"

// TODO: 일단 뼈대만 구현
class FStaticMeshEditor : public FAssetEditor
{
public:
	FStaticMeshEditor(
		UEditorEngine& InEditorEngine,
		FEditorUI& InEditorUI,
		const FString& InAssetPath
	);
	const char* GetEditorTypeName() const override { return "StaticMesh"; }
};
