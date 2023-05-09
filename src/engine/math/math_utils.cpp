#include <engine/math/math_utils.h>

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
// #TODO Implement ULP into this as well based on the source text
bool almost_equals(f32 a, f32 b, f32 max_rel_diff)
{
	// relative epsilon comparison
	f32 diff = fabsf(a - b);
	f32 abs_a = fabsf(a);
	f32 abs_b = fabsf(b);

	// #TODO Implement a GetLargest() template func that takes in variable amount of args and returns biggest
	// compare to larger of two values
	f32 largest = abs_a > abs_b ? abs_a : abs_b;

	if (diff <= largest * max_rel_diff)
		return true;

	return false;
}

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
bool is_zero(f32 s)
{
	return fabs(s) <= FLT_EPSILON;
}
