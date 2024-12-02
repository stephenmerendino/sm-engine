#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Math/Vec4_old.h"

class ColorF32
{
public:
	ColorF32(F32 inR, F32 inG, F32 inB, F32 inA);

	Vec4 ToVec4() const;

	F32 r;
	F32 g;
	F32 b;
	F32 a;

	static const ColorF32 WHITE;
	static const ColorF32 RED;
	static const ColorF32 GREEN;
	static const ColorF32 BLUE;
};

class ColorU8
{
public:
	ColorU8(U8 inR, U8 inG, U8 inB, U8 inA);

	U8 r;
	U8 g;
	U8 b;
	U8 a;

	static const ColorU8 WHITE;
	static const ColorU8 RED;
	static const ColorU8 GREEN;
	static const ColorU8 BLUE;
};
