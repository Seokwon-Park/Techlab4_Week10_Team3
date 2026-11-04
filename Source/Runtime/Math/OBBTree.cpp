#include "EnginePCH.h"
#include "OBBTree.h"

void FOBBTree::Build(const TArray<FVector>& Positions, const TArray<uint32>& Indices)
{
	TriangleBounds.Reset();
	Centroids.Reset();
	Triangles.Reset();
	Nodes.Reset();

	if (Indices.Num() == 0) return;

	for (uint32 index = 0; index < Indices.Num() / 3; index++) {
		FVector V[3];
		V[0] = Positions[Indices[3 * index]];
		V[1] = Positions[Indices[3 * index + 1]];
		V[2] = Positions[Indices[3 * index + 2]];

		// 상자 리스트 추가
		FBox Box = { V[0], V[0] };
		Box.Expand({ V[1], V[1] });
		Box.Expand({ V[2], V[2] });
		TriangleBounds.Add(Box);

		// 무게중심 리스트 추가 
		FVector Centroid = (V[0] + V[1] + V[2]) / 3.0f;
		Centroids.Add(Centroid);

		// 삼각형 번호 추가
		Triangles.Add(index);
	}

	BuildNode(0, static_cast<uint32> (Triangles.Num()), 0);
}

int32 FOBBTree::BuildNode(uint32 First, uint32 Count, uint32 Depth)
{
	FBox Bounds = TriangleBounds[Triangles[First]];
	FOBBTreeNode Node;
	// 범위 삼각형 전부 감싸는 상자 제작
	for (uint32 i = First + 1; i < First + Count; i++) {
		Bounds.Expand(TriangleBounds[Triangles[i]]);
	}
	Node.LocalBounds = Bounds;

	Node.FirstTriangle = First;
	Node.TriangleCount = Count;

	const int32 NodeIndex = static_cast <int32>(Nodes.Num());
	Nodes.Add(Node);

	if (Depth >= MaxDepth || Count <= MinTriangles) return NodeIndex;

	return NodeIndex;
}
