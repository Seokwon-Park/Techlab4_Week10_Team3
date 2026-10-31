#include "EnginePCH.h"
#include "FbxImporter.h"

#include <array>

namespace
{
	bool IsBoneNode(FbxNode* Node)
	{
		const FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
		if (!Attribute) return false;
		const auto Type = Attribute->GetAttributeType();
		return Type == FbxNodeAttribute::eSkeleton || Type == FbxNodeAttribute::eNull || Type == FbxNodeAttribute::eMesh;
	}

	FbxNode* FindSkeletonRoot(FbxNode* Node, FbxScene* Scene, bool bIsBlender)
	{
		while (FbxNode* Parent = Node->GetParent())
		{
			if (Parent == Scene->GetRootNode() || !IsBoneNode(Parent)) break;
			const auto Type = Parent->GetNodeAttribute()->GetAttributeType();
			// Blender의 Armature 컨테이너는 Bone에서 제외한다. 그 Transform은 Root의 Global에 남는다.
			if (bIsBlender && Type == FbxNodeAttribute::eNull && Parent->GetParent() == Scene->GetRootNode() && _stricmp(Parent->GetName(), "armature") == 0) break;
			// UE GetRootSkeleton처럼 Skin Mesh가 Bone의 조상인 경우 그 Mesh까지 올라가지 않는다.
			if (Type == FbxNodeAttribute::eMesh && Parent->GetMesh()->GetDeformerCount(FbxDeformer::eSkin) > 0) break;
			Node = Parent;
		}
		return Node;
	}

	void CollectBoneNodes(FbxNode* Node, TArray<FbxNode*>& Nodes)
	{
		if (!IsBoneNode(Node)) return;
		Nodes.Add(Node);
		for (int Child = 0; Child < Node->GetChildCount(); ++Child) CollectBoneNodes(Node->GetChild(Child), Nodes);
	}

	// Bind Pose의 행렬은 SDK 계약상 Global이다. 없으면 Cluster Link, 현재 Global 순으로 사용한다.
	FbxAMatrix ReadBindGlobal(FbxScene* Scene, FbxNode* Node, const TArray<FbxCluster*>& Clusters, TArray<FString>& Warnings)
	{
		FbxAMatrix Global;
		for (int I = 0; I < Scene->GetPoseCount(); ++I)
		{
			FbxPose* Pose = Scene->GetPose(I);
			if (!Pose->IsBindPose()) continue;
			const int Index = Pose->Find(Node);
			if (Index < 0) continue;
			const FbxMatrix& Matrix = Pose->GetMatrix(Index);
			std::copy_n(static_cast<const double*>(Matrix), 16, static_cast<double*>(Global));
			return Global;
		}
		Warnings.Add("Bone missing from Bind Pose; using Cluster Link or current Global: " + FString(Node->GetName()));
		for (FbxCluster* Cluster : Clusters)
		{
			if (Cluster->GetLink() != Node) continue;
			Cluster->GetTransformLinkMatrix(Global);
			return Global;
		}
		return Node->EvaluateGlobalTransform();
	}

	bool IsFiniteMatrix(const FbxAMatrix& Matrix)
	{
		const double* Values = Matrix;
		return std::all_of(Values, Values + 16, [](double Value) { return std::isfinite(Value); });
	}

	using FVertexKey = std::pair<int32, std::array<float, 9>>;

	// OBJ Importer와 같은 방식으로 정점 조합의 해시를 만든다.
	struct FVertexKeyHash
	{
		size_t operator()(const FVertexKey& Key) const
		{
			size_t Hash = std::hash<int32>{}(Key.first);
			for (float Value : Key.second)
				Hash = Hash * 31 + std::hash<float>{}(Value);
			return Hash;
		}
	};

	std::filesystem::path PathOf(const FString& Text)
	{
		return std::filesystem::path(std::u8string(Text.begin(), Text.end()));
	}

	void Warn(TArray<FString>& Warnings, const FString& Text)
	{
		if (std::find(Warnings.begin(), Warnings.end(), Text) == Warnings.end()) Warnings.Add(Text);
	}

	FVector MirrorY(const FbxVector4& Value)
	{
		return FVector(static_cast<float>(Value[0]), static_cast<float>(-Value[1]), static_cast<float>(Value[2]));
	}

	// Mapping은 대상 칸을, Reference는 그 칸이 값인지 값 배열의 Index인지를 결정한다.
	template <typename Element, typename Value>
	bool ReadElement(const Element* Layer, int ControlPoint, int PolygonVertex, int Polygon, Value& OutValue, FString& Error)
	{
		int Index;
		switch (Layer->GetMappingMode())
		{
		case FbxLayerElement::eByControlPoint: Index = ControlPoint; break;
		case FbxLayerElement::eByPolygonVertex: Index = PolygonVertex; break;
		case FbxLayerElement::eByPolygon: Index = Polygon; break;
		case FbxLayerElement::eAllSame: Index = 0; break;
		default: Error = "unsupported layer mapping"; return false;
		}
		if (Layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
		{
			if (Index < 0 || Index >= Layer->GetIndexArray().GetCount()) { Error = "layer index array out of range"; return false; }
			Index = Layer->GetIndexArray().GetAt(Index);
		}
		else if (Layer->GetReferenceMode() != FbxLayerElement::eDirect)
		{
			Error = "unsupported layer reference mode";
			return false;
		}
		if (Index < 0 || Index >= Layer->GetDirectArray().GetCount()) { Error = "layer direct array out of range"; return false; }
		OutValue = Layer->GetDirectArray().GetAt(Index);
		return true;
	}

	// Material의 지정 속성에 연결된 첫 File Texture를 찾아 원본 파일의 절대 경로를 반환한다.
	// 절대 경로 → FBX 폴더+상대 경로 → FBX 폴더+파일명 순으로 탐색하며, 파일 복사/이미지 로딩은 하지 않는다.
	// 연결이 없으면 빈 경로를 반환하고, 파일 누락이나 지원하지 않는 Texture 방식은 경고로 알린다.
	FString ReadTexture(FbxSurfaceMaterial* Material, const char* PropertyName, const std::filesystem::path& Folder, const char* UVName, TArray<FString>& Warnings)
	{
		const FbxProperty Property = Material->FindProperty(PropertyName);
		if (!Property.IsValid()) return {};
		if (Property.GetSrcObjectCount<FbxLayeredTexture>() > 0)
		{
			Warn(Warnings, "Layered texture is unsupported: " + FString(Material->GetName()));
			return {};
		}
		const int Count = Property.GetSrcObjectCount<FbxFileTexture>();
		if (!Count) return {};
		if (Count > 1) Warn(Warnings, "Only the first file texture is used: " + FString(Material->GetName()));
		FbxFileTexture* Texture = Property.GetSrcObject<FbxFileTexture>(0);
		const FString TextureUV = Texture->UVSet.Get().Buffer();
		if (!TextureUV.empty() && TextureUV != "default" && TextureUV != UVName) Warn(Warnings, "Texture requests an unused UV set: " + TextureUV);
		const auto FileName = PathOf(Texture->GetFileName());
		const auto RelativeName = PathOf(Texture->GetRelativeFileName());
		const auto Relative = RelativeName.empty() ? FileName : RelativeName;
		const auto Leaf = FileName.empty() ? RelativeName.filename() : FileName.filename();
		const std::array<std::filesystem::path, 3> Candidates = { FileName.is_absolute() ? FileName : std::filesystem::path(), !Relative.empty() && !Relative.is_absolute() ? Folder / Relative : std::filesystem::path(), Leaf.empty() ? std::filesystem::path() : Folder / Leaf };
		for (const auto& Candidate : Candidates)
		{
			std::error_code EC;
			if (Candidate.empty() || !std::filesystem::is_regular_file(Candidate, EC)) continue;
			const auto Absolute = std::filesystem::absolute(Candidate, EC);
			if (EC) continue;
			const auto Text = Absolute.lexically_normal().generic_u8string();
			return FString(Text.begin(), Text.end());
		}
		// TODO: Embedded Texture 추출 지원. 원본 파일만 찾는다.
		Warn(Warnings, "Texture file not found (embedded-only texture is unsupported): " + FString(Material->GetName()) + "/" + PropertyName + " : " + Texture->GetFileName());
		return {};
	}

	// Node의 Material 순서를 유지하여 Slot 표를 만들고 이름, Diffuse 색상, Diffuse/Normal Texture 경로를 채운다.
	// NormalMap 경로를 찾지 못하면 Bump를 대신 조회하고, Material이 없는 Node에는 기본 Slot 하나를 만든다.
	// 여기서는 Slot 표만 만들며, 삼각형별 Slot 지정과 Section 구성은 ReadMeshNode에서 처리한다.
	void ReadMaterials(FbxNode* Node, const std::filesystem::path& Folder, const char* UVName, TArray<FStaticMaterialSlot>& Slots, TArray<FString>& Warnings)
	{
		for (int I = 0; I < std::max(1, Node->GetMaterialCount()); ++I)
		{
			FStaticMaterialSlot Slot;
			Slot.Name = "Default";
			FbxSurfaceMaterial* Material = I < Node->GetMaterialCount() ? Node->GetMaterial(I) : nullptr;
			if (Material)
			{
				Slot.Name = Material->GetName();
				const FbxProperty Diffuse = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
				if (Diffuse.IsValid())
				{
					const FbxDouble3 Color = Diffuse.Get<FbxDouble3>();
					Slot.BaseColor = FVector4(static_cast<float>(Color[0]), static_cast<float>(Color[1]), static_cast<float>(Color[2]), 1.0f);
				}
				Slot.DiffuseTexturePath = ReadTexture(Material, FbxSurfaceMaterial::sDiffuse, Folder, UVName, Warnings);
				Slot.NormalTexturePath = ReadTexture(Material, FbxSurfaceMaterial::sNormalMap, Folder, UVName, Warnings);
				if (Slot.NormalTexturePath.empty()) Slot.NormalTexturePath = ReadTexture(Material, FbxSurfaceMaterial::sBump, Folder, UVName, Warnings);
			}
			Slots.Add(std::move(Slot));
		}
	}

	// 삼각형화가 끝난 Mesh 하나에서 엔진 기준 Geometry/Material과 정점별 Control Point 번호를 추출한다.
	// Node와 Skin 유무도 보관하여 후속 Skeleton 분류와 Weight 연결에 사용한다. 여기서는 Bone/Weight를 읽지 않는다.
	bool ReadMeshNode(FbxNode* Node, FbxMesh* Mesh, const std::filesystem::path& Folder, FFbxMeshNodeData& Result, TArray<FString>& Warnings, FString& Error)
	{
		Result.Node = Node;
		Result.bHasSkin = Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
		FStaticMeshData& Geometry = Result.Geometry;
		if (!Mesh->GetPolygonCount()) { Warn(Warnings, "Empty mesh: " + FString(Node->GetName())); return true; }

		// multiple layer에 대한 처리는 하지 않는다.
		FbxLayer* Layer = Mesh->GetLayer(0);
		if (!Layer || !Layer->GetNormals())
		{
			if (!Mesh->GenerateNormals(true, false)) { Error = "normal generation failed"; return false; }
			Layer = Mesh->GetLayer(0);
			Warn(Warnings, "Missing normals generated: " + FString(Node->GetName()));
		}
		if (!Layer || !Layer->GetNormals()) { Error = "no normal layer"; return false; }

		const auto* Normals = Layer->GetNormals();
		const auto* UVs = Layer->GetUVs();
		const auto* Colors = Layer->GetVertexColors();
		const auto* Materials = Layer->GetMaterials();
		if (!UVs) Warn(Warnings, "Missing UVs use (0,0): " + FString(Node->GetName()));
		if (Mesh->GetElementUVCount() > 1) Warn(Warnings, "Only the first layer-0 UV set is used: " + FString(Node->GetName()));
		if (Mesh->GetDeformerCount(FbxDeformer::eBlendShape)) Warn(Warnings, "Blend shapes are not imported: " + FString(Node->GetName()));
		if (Mesh->GetDeformerCount(FbxDeformer::eVertexCache)) Warn(Warnings, "Vertex cache deformation is not imported: " + FString(Node->GetName()));

		// Mesh 자체에 적용되는 Geometric Translation/Rotation/Scale을 G 행렬로 만든다.
		// Node의 Global Transform W와 결합해 정점 위치에 굽고, Normal에는 그 결합 행렬의 역전치를 적용한다.
		FbxAMatrix Geometric;
		Geometric.SetT(Node->GetGeometricTranslation(FbxNode::eSourcePivot));
		Geometric.SetR(Node->GetGeometricRotation(FbxNode::eSourcePivot));
		Geometric.SetS(Node->GetGeometricScaling(FbxNode::eSourcePivot));
		
		// SDK는 열벡터이므로 엔진의 p*G*W에 해당하는 행렬은 W*G다.
		const FbxAMatrix Total = Node->EvaluateGlobalTransform() * Geometric;
		// 출력 때 적용하는 Y 반전 C의 determinant는 -1이다. 최종 G*W*C 기준으로 검사한다.
		const double FinalDeterminant = -Total.Determinant();
		if (!std::isfinite(FinalDeterminant) || FinalDeterminant == 0) { Error = "singular/non-finite mesh transform"; return false; }
		
		const FbxAMatrix NormalMatrix = Total.Inverse().Transpose();
		
		ReadMaterials(Node, Folder, UVs ? UVs->GetName() : "", Geometry.MaterialSlots, Warnings);
		
		std::vector<TArray<uint32>> SlotIndices(Geometry.MaterialSlots.Num());
		TMap<FVertexKey, uint32, FVertexKeyHash> VerticesByValue;
		// 삼각형마다 Material Slot을 정하고, 세 Polygon Vertex를 엔진 정점 번호로 변환한다.
		// 정점 조합은 Node 전체에서 공유하고, 삼각형 Index는 Slot별로 모아 나중에 Section으로 묶는다.
		for (int Polygon = 0; Polygon < Mesh->GetPolygonCount(); ++Polygon)
		{
			if (Mesh->GetPolygonSize(Polygon) != 3) { Error = "triangulation left a non-triangle"; return false; }
			int Slot = 0;
			// ByPolygon이면 현재 Polygon 칸, AllSame이면 0번 칸을 읽는다.
			// Material IndexArray의 값은 Node의 Material Slot 번호다. 누락되거나 범위를 벗어나면 Slot 0을 사용한다.
			if (Materials)
			{
				const auto Mapping = Materials->GetMappingMode();
				if (Mapping != FbxLayerElement::eByPolygon && Mapping != FbxLayerElement::eAllSame) { Error = "unsupported material mapping"; return false; }
				if (Materials->GetReferenceMode() != FbxLayerElement::eIndexToDirect) { Error = "unsupported material reference mode"; return false; }
				const int Index = Mapping == FbxLayerElement::eAllSame ? 0 : Polygon;
				Slot = Index < Materials->GetIndexArray().GetCount() ? Materials->GetIndexArray().GetAt(Index) : -1;
				if (Slot < 0 || Slot >= Geometry.MaterialSlots.Num()) { Slot = 0; Warn(Warnings, "Invalid material assignment uses slot 0: " + FString(Node->GetName())); }
			}
			else Warn(Warnings, "Missing material layer uses slot 0: " + FString(Node->GetName()));
			uint32 Triangle[3];
			// 각 꼭짓점의 CP(위치 번호)와 PV(Polygon Vertex 칸 번호)로 Normal/UV/Color를 조회한다.
			// Position/Normal을 변환하고 Y 반전, Normal 정규화, UV의 V 반전을 적용한다.
			// 같은 (CP, Normal, UV, Color) 조합은 기존 정점을 재사용하고, 새 정점이면 CP 대응표와 AABB도 갱신한다.
			// Triangle에 넣는 값은 CP 번호가 아니라 최종 Geometry.Vertices의 정점 번호다.
			for (int Corner = 0; Corner < 3; ++Corner)
			{
				const int CP = Mesh->GetPolygonVertex(Polygon, Corner);
				if (CP < 0 || CP >= Mesh->GetControlPointsCount()) { Error = "control point index out of range"; return false; }
				const int PV = Mesh->GetPolygonVertexIndex(Polygon) + Corner;
				FbxVector4 Normal;
				FbxVector2 UV(0, 0);
				FbxColor Color(1, 1, 1, 1);
				if (!ReadElement(Normals, CP, PV, Polygon, Normal, Error)) return false;
				if (UVs && !ReadElement(UVs, CP, PV, Polygon, UV, Error)) return false;
				if (Colors && !ReadElement(Colors, CP, PV, Polygon, Color, Error)) return false;
				// 방향에는 3x3 성분만 곱하여 평행이동을 제외한다.
				FbxVector4 TransformedNormal(0, 0, 0, 0);
				for (int Row = 0; Row < 3; ++Row)
					for (int Column = 0; Column < 3; ++Column) TransformedNormal[Row] += NormalMatrix.Get(Column, Row) * Normal[Column];
				const double Length = std::sqrt(TransformedNormal[0] * TransformedNormal[0] + TransformedNormal[1] * TransformedNormal[1] + TransformedNormal[2] * TransformedNormal[2]);
				if (!std::isfinite(Length) || Length == 0) { Error = "zero/non-finite normal"; return false; }
				FVertex Vertex;
				Vertex.Position = MirrorY(Total.MultT(Mesh->GetControlPointAt(CP)));
				Vertex.Normal = MirrorY(TransformedNormal / Length);
				Vertex.UV = FVector2(static_cast<float>(UV[0]), UVs ? static_cast<float>(1.0 - UV[1]) : 0.0f);
				Vertex.Color = FVector4(static_cast<float>(Color.mRed), static_cast<float>(Color.mGreen), static_cast<float>(Color.mBlue), static_cast<float>(Color.mAlpha));
				const FVertexKey Key{ CP, { Vertex.Normal.X, Vertex.Normal.Y, Vertex.Normal.Z, Vertex.UV.X, Vertex.UV.Y, Vertex.Color.X, Vertex.Color.Y, Vertex.Color.Z, Vertex.Color.W } };
				if (!std::isfinite(Vertex.Position.X) || !std::isfinite(Vertex.Position.Y) || !std::isfinite(Vertex.Position.Z) || !std::all_of(Key.second.begin(), Key.second.end(), [](float V) { return std::isfinite(V); })) { Error = "non-finite vertex attribute"; return false; }
				if (const uint32* ExistingIndex = VerticesByValue.Find(Key))
				{
					Triangle[Corner] = *ExistingIndex;
				}
				else
				{
					const uint32 NewIndex = static_cast<uint32>(Geometry.Vertices.Num());
					VerticesByValue.Add(Key, NewIndex);
					if (Geometry.Vertices.IsEmpty()) Geometry.AABB = { Vertex.Position, Vertex.Position };
					else Geometry.AABB.Expand({ Vertex.Position, Vertex.Position });
					Geometry.Vertices.Add(Vertex);
					Result.ControlPointIndices.Add(CP);
					Triangle[Corner] = NewIndex;
				}
			}
			if (FinalDeterminant < 0) std::swap(Triangle[1], Triangle[2]);
			for (uint32 Index : Triangle) SlotIndices[Slot].Add(Index);
		}
		// Slot별로 모아 둔 Index를 최종 배열에 이어 붙여 같은 Material을 쓰는 삼각형이 연속하도록 한다.
		// 각 Section에는 이어 붙이기 전 시작 위치, Index 개수, Material Slot 번호를 기록한다. 사용하지 않은 Slot은 Section을 만들지 않는다.
		for (int Slot = 0; Slot < Geometry.MaterialSlots.Num(); ++Slot)
		{
			if (SlotIndices[Slot].IsEmpty()) continue;
			Geometry.Sections.Add({ static_cast<uint32>(Geometry.Indices.Num()), static_cast<uint32>(SlotIndices[Slot].Num()), static_cast<uint32>(Slot) });
			Geometry.Indices.Append(SlotIndices[Slot]);
		}
		return true;
	}
}

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
	// Open은 Scene 읽기까지 담당하며, 파일 안의 Texture를 디스크에 추출하지 않는다.
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

	// Creator는 Reader가 살아 있을 때 읽어 두고 Blender Armature 예외 판별에 사용한다.
	const FbxIOFileHeaderInfo* Header = Reader->GetFileHeaderInfo();
	bIsBlender = Header && FString(Header->mCreator.Buffer()).starts_with("Blender");
	// 파일 읽기는 끝났으므로 Reader를 닫고 메모리에 있는 Scene만 유지한다.
	Reader->Destroy();
	SourcePath = Path;
	return true;
}

void FFbxImporter::Close()
{
	ClearSkeletonData();
	MeshNodes.Empty();
	SourcePath.clear();
	bIsBlender = false;
	// Manager가 아직 남아 있는 Reader, Scene, IOSettings를 함께 해제한다.
	if (Manager)
		Manager->Destroy();
	Manager = nullptr;
	Scene = nullptr;
}

// 열린 Scene의 축/단위를 변환한 뒤, Mesh Node별 Geometry/Material과 Control Point 대응표를 만든다.
// 모든 Node 추출이 성공했을 때만 Results를 MeshNodes에 보관한다.
bool FFbxImporter::ReadMeshData(TArray<FString>& OutWarnings, FString& OutError)
{
	ClearSkeletonData();
	MeshNodes.Empty();
	OutWarnings.Empty();
	OutError.clear();
	if (!Scene) 
	{ 
		OutError = "FBX: no scene is open"; 
		return false; 
	}

	FbxRootNodeUtility::RemoveAllFbxRoots(Scene);
	// 엔진의 최종 기준은 UE처럼 왼손 좌표계(+Z 위, +X 앞)다.
	// Target은 SDK 축 변환에 사용할 중간 기준(+Z 위, +X 앞, 오른손)이며, 생성만으로 Scene은 바뀌지 않는다.
	// ConvertScene은 Scene 바로 아래 Node의 회전으로 축을 맞추므로 오른손/왼손 전환은 처리하지 못한다.
	// 따라서 eLeftHanded를 지정하는 것만으로는 엔진 좌표계로 변환되지 않는다. (SDK fbxaxissystem.h의 ConvertScene 설명)
	// 여기서는 Node Transform과 Scene의 축 정보를 갱신하고, 자식은 부모의 변환을 상속한다.
	// 이후 Position/Normal을 엔진 데이터로 옮길 때 Y를 반전하여 왼손 기준을 완성한다.
	// Index 교환도 이 Y 반전까지 포함한 최종 변환의 determinant를 기준으로 처리한다.
	const FbxAxisSystem Target(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eRightHanded);
	if (Scene->GetGlobalSettings().GetAxisSystem() != Target) Target.ConvertScene(Scene);
	// 단위도 설정값만 바꾸는 것이 아니라 Scene 데이터를 cm 기준으로 실제 변환한다.
	if (Scene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::cm) FbxSystemUnit::cm.ConvertScene(Scene);
	// 축/단위 변환 전 자식 Node의 Global 캐시가 남지 않도록 비운다. UE ConvertScene 경로와 같은 처리다.
	Scene->GetAnimationEvaluator()->Reset();
	
	FbxGeometryConverter Converter(Manager);
	TArray<FFbxMeshNodeData> Results;
	// Scene의 Node 목록에서 Mesh를 찾는다. Mesh가 없는 Node는 건너뛴다.
	// 지원하지 않는 Geometry/복수 Mesh Attribute는 오류로 처리하고,
	// 각 Mesh를 삼각형화한 뒤 새 Mesh에서 Node별 데이터를 추출한다.
	for (int I = 0; I < Scene->GetNodeCount(); ++I)
	{
		FbxNode* Node = Scene->GetNode(I);
		int MeshCount = 0;
		for (int A = 0; A < Node->GetNodeAttributeCount(); ++A)
		{
			const auto Type = Node->GetNodeAttributeByIndex(A)->GetAttributeType();
			if (Type == FbxNodeAttribute::eMesh) 
				++MeshCount;
			if (Type == FbxNodeAttribute::eNurbs || Type == FbxNodeAttribute::eNurbsSurface || Type == FbxNodeAttribute::ePatch) 
			{ 
				OutError = "FBX: unsupported Patch/NURBS: " + FString(Node->GetName()); 
				return false; 
			}
		}
		if (!MeshCount) continue;
		if (MeshCount != 1 || !Node->GetMesh()) 
		{ 
			OutError = "FBX: unsupported multiple/non-default mesh attributes: " + FString(Node->GetName()); 
			return false; 
		}

		// replace를 true로 하였기때문에, 기존 데이터는 사라지고, 삼각형으로 변환된 데이터만 남게 된다.
		if (!Converter.Triangulate(Node->GetMesh(), true)) 
		{ 
			OutError = "FBX: triangulation failed: " + FString(Node->GetName()); 
			return false; 
		}
		// Triangulate가 기존 Mesh를 교체할 수 있으므로 Node에서 다시 얻는다.
		FFbxMeshNodeData Result;
		if (!ReadMeshNode(Node, Node->GetMesh(), PathOf(SourcePath).parent_path(), Result, OutWarnings, OutError))
		{
			OutError = "FBX: " + FString(Node->GetName()) + ": " + OutError;
			return false;
		}
		Results.Add(std::move(Result));
	}
	if (Results.IsEmpty()) { OutError = "FBX: no mesh nodes"; return false; }
	MeshNodes = std::move(Results);
	return true;
}

// Node별로 추출한 Geometry를 하나의 Static 출력 데이터로 합친다.
// Skin이 있는 Node도 포함하며, 이미 구워진 Position/Normal에 Transform을 다시 적용하지 않는다.
void FFbxImporter::BuildStaticMesh(FStaticMeshData& OutData) const
{
	OutData = {};
	// 각 Node의 배열을 뒤에 붙이므로 기존 Vertex/Index/Slot 개수만큼 참조 번호를 보정한다.
	// Index 값에는 VertexBase, Section의 시작에는 IndexBase, Material 번호에는 SlotBase를 더한다.
	// AABB는 모든 Node의 경계가 들어가도록 합친다.
	for (const FFbxMeshNodeData& Node : MeshNodes)
	{
		const FStaticMeshData& Mesh = Node.Geometry;
		if (Mesh.Vertices.IsEmpty()) continue;
		const uint32 VertexBase = static_cast<uint32>(OutData.Vertices.Num());
		const uint32 IndexBase = static_cast<uint32>(OutData.Indices.Num());
		const uint32 SlotBase = static_cast<uint32>(OutData.MaterialSlots.Num());
		
		if (OutData.Vertices.IsEmpty()) 
			OutData.AABB = Mesh.AABB;
		else 
			OutData.AABB.Expand(Mesh.AABB);
		
		OutData.Vertices.Append(Mesh.Vertices);
		for (uint32 Index : Mesh.Indices) 
			OutData.Indices.Add(VertexBase + Index);

		OutData.MaterialSlots.Append(Mesh.MaterialSlots);
		for (const FStaticMeshSection& Section : Mesh.Sections) 
			OutData.Sections.Add({ IndexBase + Section.StartIndex, Section.IndexCount, SlotBase + Section.MaterialSlotIndex });
	}
}


void FFbxImporter::ClearSkeletonData()
{
	Bones.Empty();
	BoneIndices.Empty();
	for (FFbxMeshNodeData& Mesh : MeshNodes) Mesh.RigidBoneIndex = -1;
}

// ReadMeshData로 축과 단위를 변환한 Scene에서 Bone 계층과 Bind를 읽는다. 실패 시 Skeleton 결과는 비워 둔다.
bool FFbxImporter::ReadSkeletonData(TArray<FString>& OutWarnings, FString& OutError)
{
	ClearSkeletonData();
	OutWarnings.Empty();
	OutError.clear();
	if (!Scene || MeshNodes.IsEmpty()) { OutError = "FBX: read mesh data before skeleton data"; return false; }

	// 여러 Mesh의 Cluster를 모아 Bone 계층과 Bind를 조회한다. Weight 값은 읽지 않는다.
	TArray<FbxCluster*> Clusters;
	FbxNode* Root = nullptr;
	for (const FFbxMeshNodeData& Mesh : MeshNodes)
	{
		if (!Mesh.bHasSkin) continue;
		FbxMesh* Geometry = Mesh.Node->GetMesh();
		for (int S = 0; S < Geometry->GetDeformerCount(FbxDeformer::eSkin); ++S)
		{
			auto* Skin = static_cast<FbxSkin*>(Geometry->GetDeformer(S, FbxDeformer::eSkin));
			if (!Skin->GetClusterCount()) { OutError = "FBX: Skin has no clusters: " + FString(Mesh.Node->GetName()); return false; }
			for (int C = 0; C < Skin->GetClusterCount(); ++C)
			{
				FbxCluster* Cluster = Skin->GetCluster(C);
				FbxNode* Link = Cluster->GetLink();
				if (!Link) { OutError = "FBX: Cluster has no Link: " + FString(Mesh.Node->GetName()); return false; }
				FbxNode* Candidate = FindSkeletonRoot(Link, Scene, bIsBlender);
				if (Root && Root != Candidate) { OutError = "FBX: multiple skeleton roots"; return false; }
				Root = Candidate;
				Clusters.Add(Cluster);
			}
		}
	}
	if (!Root) { OutError = "FBX: no skeleton bones"; return false; }
	TArray<FbxNode*> Nodes;
	CollectBoneNodes(Root, Nodes);
	if (Nodes.IsEmpty()) { OutError = "FBX: no skeleton bones"; return false; }
	// 기존 FSkinWeight::BoneIndex가 uint8이므로 최종 출력으로 표현할 수 있는 Bone 수에 맞춘다.
	if (Nodes.Num() > 256) { OutError = "FBX: bone count exceeds 256"; return false; }

	TMap<FbxNode*, int32> ResultIndices;
	TSet<FString> Names;
	TArray<FBoneInfo> ResultBones;
	TArray<FbxAMatrix> BindGlobals;
	TArray<FMatrix> BindWorlds;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		FbxNode* Node = Nodes[I];
		FBoneInfo Bone;
		Bone.Name = Node->GetName();
		if (Names.Contains(Bone.Name)) { OutError = "FBX: duplicate bone name: " + Bone.Name; return false; }
		Names.Add(Bone.Name);
		if (I > 0)
		{
			const int32* Parent = ResultIndices.Find(Node->GetParent());
			if (!Parent) { OutError = "FBX: bone parent is not in skeleton: " + Bone.Name; return false; }
			Bone.ParentIndex = *Parent;
		}

		const FbxAMatrix Global = ReadBindGlobal(Scene, Node, Clusters, OutWarnings);
		const double GlobalDeterminant = Global.Determinant();
		if (!IsFiniteMatrix(Global) || !std::isfinite(GlobalDeterminant) || GlobalDeterminant == 0) { OutError = "FBX: invalid Bind Global: " + Bone.Name; return false; }
		const FbxAMatrix Local = Bone.ParentIndex < 0 ? Global : BindGlobals[Bone.ParentIndex].Inverse() * Global;
		if (!IsFiniteMatrix(Local)) { OutError = "FBX: non-finite Bind Local: " + Bone.Name; return false; }
		const FbxVector4 Translation = Local.GetT();
		const FbxQuaternion Rotation = Local.GetQ();
		const FbxVector4 Scale = Local.GetS();
		// Position과 같은 Y 반전을 Transform에도 적용한다. Quaternion은 (x,-y,z,-w), Scale은 그대로다.
		Bone.LocalBindPose.Location = MirrorY(Translation);
		Bone.LocalBindPose.Scale = FVector(static_cast<float>(Scale[0]), static_cast<float>(Scale[1]), static_cast<float>(Scale[2]));
		// SDK에서 Euler로 변환하여 정확히 +/-90도인 회전도 보존한다. FRotator 저장 순서는 Pitch(Y), Yaw(Z), Roll(X)다.
		FbxAMatrix EngineRotation;
		EngineRotation.SetQ(FbxQuaternion(Rotation[0], -Rotation[1], Rotation[2], -Rotation[3]));
		const FbxVector4 Euler = EngineRotation.GetR();
		Bone.LocalBindPose.Rotation = FRotator(static_cast<float>(Euler[1]), static_cast<float>(Euler[2]), static_cast<float>(Euler[0]));
		const FMatrix LocalMatrix = Bone.LocalBindPose.GetLocalMatrix();
		const double LocalDeterminant = LocalMatrix.Determinant();
		if (!std::isfinite(LocalDeterminant) || LocalDeterminant == 0) { OutError = "FBX: non-finite/zero-scale Bind Local: " + Bone.Name; return false; }
		// 런타임과 같은 저장 Local 누적에서 역행렬을 만들어 TRS 분해 후의 자세와 일치시킨다.
		const FMatrix World = Bone.ParentIndex < 0 ? LocalMatrix : LocalMatrix * BindWorlds[Bone.ParentIndex];
		Bone.InverseBindPose = World.Inverse();
		for (int R = 0; R < 4; ++R)
		{
			for (int C = 0; C < 4; ++C)
			{
				if (!std::isfinite(World.M[R][C]) || !std::isfinite(Bone.InverseBindPose.M[R][C])) { OutError = "FBX: non-finite accumulated Bind: " + Bone.Name; return false; }
			}
		}
		ResultIndices.Add(Node, I);
		ResultBones.Add(std::move(Bone));
		BindGlobals.Add(Global);
		BindWorlds.Add(World);
	}
	for (FbxCluster* Cluster : Clusters)
	{
		if (!ResultIndices.Contains(Cluster->GetLink())) { OutError = "FBX: Cluster Link is outside the selected bone hierarchy"; return false; }
	}

	Bones = std::move(ResultBones);
	BoneIndices = std::move(ResultIndices);
	// Bone 계층의 Skin 없는 Mesh에 자기 Node의 Bone 번호를 연결한다.
	for (FFbxMeshNodeData& Mesh : MeshNodes)
	{
		if (Mesh.bHasSkin) continue;
		if (const int32* BoneIndex = BoneIndices.Find(Mesh.Node)) Mesh.RigidBoneIndex = *BoneIndex;
	}
	return true;
}
