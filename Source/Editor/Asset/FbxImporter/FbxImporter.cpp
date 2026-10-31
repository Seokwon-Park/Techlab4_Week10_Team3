#include "EnginePCH.h"
#include "FbxImporter.h"

FFbxImporter::~FFbxImporter()
{
	Close();
}

bool FFbxImporter::Open(const FString& Path, FString& OutError)
{
	Close();
	OutError.clear();

	Manager = FbxManager::Create();
	if (!Manager)
	{
		OutError = "FBX: failed to create SDK manager";
		return false;
	}

	FbxIOSettings* Settings = FbxIOSettings::Create(Manager, IOSROOT);	// IOSRoot라는 이름으로 FbxIOSettings 객체를 생성
	if (!Settings)
	{
		OutError = "FBX: failed to create IO settings";
		Close();
		return false;
	}
	Manager->SetIOSettings(Settings);


	// TODO: 아직은 embedded data에 대한 처리를 하지 않는다.
	// 이번 단계는 Scene 읽기까지다. 파일 안의 Texture를 디스크에 추출하지 않는다.
	Settings->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);

	Scene = FbxScene::Create(Manager, "ImportScene");
	// importer에 이름을 지정하면, 이름을 기반으로 로그를 남길 때 유용하게 사용할 수 있지만
	// 현재는 multiple importer가 필요하지 않아서 이름은 지정하지 않았다.
	FbxImporter* Reader = FbxImporter::Create(Manager, "");
	if (!Scene || !Reader)
	{
		OutError = "FBX: failed to create scene or reader";
		Close();
		return false;
	}

	if (!Reader->Initialize(Path.c_str(), -1, Settings))
	{
		OutError = "FBX: cannot open '" + Path + "': " + Reader->GetStatus().GetErrorString();
		Close();
		return false;
	}
	if (!Reader->IsFBX())
	{
		OutError = "FBX: not an FBX file: '" + Path + "'";
		Close();
		return false;
	}
	if (!Reader->Import(Scene))
	{
		OutError = "FBX: cannot import '" + Path + "': " + Reader->GetStatus().GetErrorString();
		Close();
		return false;
	}

	// 파일 읽기는 끝났으므로 Reader를 닫고 메모리에 있는 Scene만 유지한다.
	Reader->Destroy();
	return true;
}

void FFbxImporter::Close()
{
	// Manager가 아직 남아 있는 Reader, Scene, IOSettings를 함께 해제한다.
	if (Manager)
		Manager->Destroy();
	Manager = nullptr;
	Scene = nullptr;
}
