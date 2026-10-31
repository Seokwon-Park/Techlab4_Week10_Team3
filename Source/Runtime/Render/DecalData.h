#pragma once

struct alignas(16) FDecalData
{
	FMatrix W;
	FMatrix VP;
};

static_assert(sizeof(FDecalData) == 128);

