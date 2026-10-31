#pragma once

#include <fbxsdk.h>
#include "Core/EngineString.h"

// Manager와 Scene의 수명을 관리하며 한 번에 FBX 파일 하나를 보관한다.
class FFbxImporter
{
public:
	FFbxImporter() = default;
	~FFbxImporter();
	FFbxImporter(const FFbxImporter&) = delete;
	FFbxImporter& operator=(const FFbxImporter&) = delete;

	// 이전 Scene을 닫고 새 파일을 읽는다. 실패하면 닫힌 상태와 오류를 반환한다.
	bool Open(const FString& Path, FString& OutError);
	void Close();
	// 비소유 참조. 다음 Open/Close 또는 이 객체의 소멸 전까지만 유효하다.
	FbxScene* GetScene() const { return Scene; }

private:
	FbxManager* Manager = nullptr;
	FbxScene* Scene = nullptr;
};
