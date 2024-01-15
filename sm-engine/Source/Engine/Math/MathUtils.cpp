#include <Engine/Math/MathUtils.h>

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
// #TODO(smerendino): Implement ULP into this as well based on the source text
bool AlmostEquals(F32 a, F32 b, F32 maxRelDiff)
{
	// relative epsilon comparison
	F32 diff = fabsf(a - b);
	F32 absA = fabsf(a);
	F32 absB = fabsf(b);

	// #TODO(smerendino): Implement a get_largest() template func that takes in variable amount of args and returns biggest
	// compare to larger of two values
	F32 largest = absA > absB ? absA : absB;

	if (diff <= largest * maxRelDiff)
		return true;

	return false;
}

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
bool IsZero(F32 value)
{
	return fabs(value) <= FLT_EPSILON;
}
