#pragma once

#include "Editor/EditorUI/EditorPanel.h"

class IEditorTab
{
public:
	virtual ~IEditorTab() = default;
	virtual const char* GetTabName() const = 0;
	virtual bool CanDetach() const { return true; }
	// BeginMenuBar/EndMenuBar는 컨테이너가 담당하고 탭은 메뉴 내용만 그린다.
	virtual void DrawMenuBar() {}
	virtual void Tick(float DeltaTime) = 0;
	virtual void OnRender(bool bActive) = 0;
};
