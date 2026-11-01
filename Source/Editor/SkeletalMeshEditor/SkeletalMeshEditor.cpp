#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

FSkeletalMeshEditor::FSkeletalMeshEditor(
	UEditorEngine& InEditorEngine,
	FEditorUI& InEditorUI,
	const FString& InAssetPath
)
	: FAssetEditor(InEditorEngine, InEditorUI, InAssetPath)
{
}
