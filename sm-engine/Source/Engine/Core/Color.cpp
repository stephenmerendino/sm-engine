#include "Engine/Core/Color.h"
#include "Engine/Core/Assert.h"

ColorF32::ColorF32(F32 inR, F32 inG, F32 inB, F32 inA)
	:r(inR)
	,g(inG)
	,b(inB)
	,a(inA)
{
}

ColorU8 ColorF32::ToColorU8()
{
	return ColorU8((U8)(r * 255),
				   (U8)(g * 255),
				   (U8)(b * 255),
				   (U8)(a * 255));
}

const ColorF32 ColorF32::WHITE	= { 1.0f, 1.0f, 1.0f, 1.0f };
const ColorF32 ColorF32::RED	= { 1.0f, 0.0f, 0.0f, 1.0f };
const ColorF32 ColorF32::GREEN	= { 0.0f, 1.0f, 0.0f, 1.0f };
const ColorF32 ColorF32::BLUE	= { 0.0f, 0.0f, 1.0f, 1.0f };

ColorU8::ColorU8(U8 inR, U8 inG, U8 inB, U8 inA)
	:r(inR)
	,g(inG)
	,b(inB)
	,a(inA)
{
}

ColorF32 ColorU8::ToColorF32()
{
	return ColorF32(((F32)r / 255.0f),
				    ((F32)g / 255.0f),
				    ((F32)b / 255.0f),
				    ((F32)a / 255.0f));
}

const ColorU8 ColorU8::WHITE	= { 255, 255, 255, 255 };
const ColorU8 ColorU8::RED		= { 255, 0, 0, 255 };
const ColorU8 ColorU8::GREEN	= { 0, 255, 0, 255 };
const ColorU8 ColorU8::BLUE		= { 0, 0, 255, 255 };
