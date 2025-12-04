#include "SM/Math.h"

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
// #TODO(smerendino): Implement ULP into this as well based on the source text
bool SM::CloseEnough(F32 a, F32 b, F32 acceptableError)
{
	// relative epsilon comparison
	F32 diff = fabsf(a - b);
	F32 absA = fabsf(a);
	F32 absB = fabsf(b);

	// #TODO(smerendino): Implement a get_largest() template func that takes in variable amount of args and returns biggest
	// compare to larger of two values
	F32 largest = absA > absB ? absA : absB;

	if (diff <= largest * acceptableError)
		return true;

	return false;
};

bool SM::IsCloseEnoughToZero(F32 value, F32 acceptableError)
{
	return fabs(value) <= acceptableError;
};
