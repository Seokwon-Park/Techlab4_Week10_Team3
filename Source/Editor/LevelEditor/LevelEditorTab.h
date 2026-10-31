#pragma once

#include "Editor/EditorUI/EditorTab.h"
#include "Editor/LevelEditor/LevelEditorToolbar.h"

class FLevelEditorTab : public IEditorTab
{
public:
	const char* GetTabName() const override { return "LevelEditor"; }
	bool CanDetach() const override { return false; }
	FLevelEditorToolbar* GetToolbar() { return &Toolbar; }
	TArray<TUniquePtr<IEditorPanel>>& GetPanels() { return Panels; }

	template <typename T>
	T* AddEditorPanel()
	{
		TUniquePtr<T> Panel = MakeUnique<T>();
		T* Result = Panel.get();
		Result->Init();
		Panels.Add(std::move(Panel));
		return Result;
	}

	void Tick(float DeltaTime) override
	{
		for (auto& Panel : Panels)
			Panel->Tick(DeltaTime);
	}

	void OnRender(bool bActive) override
	{
		if (bActive)
			Toolbar.Draw();

		if (bUseDockSpace && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable))
		{
			// 부모 창이 달라져도 기존 패널 도킹 ID는 유지한다.
			const ImGuiID DockSpaceId = ImHashStr("EngineDockingSpace", 0, ImHashStr("DockingSpace"));
			ImGuiDockNodeFlags Flags = bPassthruCentralNode
				? ImGuiDockNodeFlags_PassthruCentralNode : ImGuiDockNodeFlags_None;
			if (!bActive)
				Flags |= ImGuiDockNodeFlags_KeepAliveOnly;

			ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags);
		}

		if (bActive)
		{
			for (auto& Panel : Panels)
			{
				if (Panel->IsOpen())
					Panel->OnRender();
			}
		}
	}

	void SetDockSpaceOptions(bool bEnabled, bool bPassthru)
	{
		bUseDockSpace = bEnabled;
		bPassthruCentralNode = bPassthru;
	}

private:
	FLevelEditorToolbar Toolbar;
	TArray<TUniquePtr<IEditorPanel>> Panels;
	bool bUseDockSpace = true;
	bool bPassthruCentralNode = false;
};