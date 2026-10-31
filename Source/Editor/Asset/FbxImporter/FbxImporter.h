#pragma once

#include <fbxsdk.h>
#include "Core/EngineString.h"
#include "Render/StaticMeshData.h"

// Import 중에만 사용하는 Node별 자료. SDK 포인터는 최종 엔진 출력에 넘기지 않는다.
struct FFbxMeshNodeData
{
	FbxNode* Node = nullptr;
	bool bHasSkin = false;
	FStaticMeshData Geometry;
	TArray<int32> ControlPointIndices; // Geometry.Vertices와 같은 순서/길이
};

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
	// 열린 Scene의 축/단위를 맞추고 Node별 자료를 생성한다. 실패하면 Node 결과는 비운다.
	bool ReadMeshData(TArray<FString>& OutWarnings, FString& OutError);
	// 참조는 다음 ReadMeshData/Open/Close/소멸 전까지만 유효하다.
	const TArray<FFbxMeshNodeData>& GetMeshNodes() const { return MeshNodes; }
	// Import Static용: Skin 유무와 관계없이 모든 Node를 하나로 합친다.
	void BuildStaticMesh(FStaticMeshData& OutData) const;

private:
	FbxManager* Manager = nullptr;
	FbxScene* Scene = nullptr;
	FString SourcePath;
	TArray<FFbxMeshNodeData> MeshNodes;
};
