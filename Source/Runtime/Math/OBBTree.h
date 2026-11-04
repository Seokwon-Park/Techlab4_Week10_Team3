#pragma once

#include "Box.h"

struct FOBBTreeNode {
	FBox LocalBounds;
	int32 Left = -1;
	int32 Right = -1;
	uint32 FirstTriangle = 0;
	uint32 TriangleCount = 0;

	bool IsLeaf() const { return Left < 0; }
};

class FOBBTree {
public:
	void Build(const TArray <FVector>& Positions, const TArray<uint32>& Indices);

private:
	TArray <FOBBTreeNode> Nodes;
	TArray <uint32> Triangles;

	static constexpr uint32 MaxDepth = 8;
	static constexpr uint32 MinTriangles = 8;

	TArray<FBox> TriangleBounds;
	TArray<FVector> Centroids;	// 삼각형 무게중심 모음
};