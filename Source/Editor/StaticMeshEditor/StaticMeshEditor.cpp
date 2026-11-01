#include "EnginePCH.h"
#include "Editor/StaticMeshEditor/StaticMeshEditor.h"

FStaticMeshEditor::FStaticMeshEditor(
	UEditorEngine& InEditorEngine,
	FEditorUI& InEditorUI,
	const FString& InAssetPath
)
	: FAssetEditor(InEditorEngine, InEditorUI, InAssetPath)
{
}
