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
private:
	TArray <FOBBTreeNode> Nodes;
	TArray <uint32> Triangles;

	static constexpr uint32 MaxDepth = 8;
	static constexpr uint32 MinTriangles = 8;
};