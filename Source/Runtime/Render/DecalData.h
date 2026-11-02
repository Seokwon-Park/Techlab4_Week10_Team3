#pragma once

struct alignas(16) FDecalData
{
	FMatrix VP;
	float Fade;
	float Padding[3];
};

static_assert(sizeof(FDecalData) == 80);

