#include "sm/math/helpers.h"

using namespace sm;

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
// #TODO(smerendino): Implement ULP into this as well based on the source text
bool sm::almost_equals(f32 a, f32 b, f32 acceptable_error)
{
	// relative epsilon comparison
	f32 diff = fabsf(a - b);
	f32 abs_a = fabsf(a);
	f32 abs_b = fabsf(b);

	// #TODO(smerendino): Implement a get_largest() template func that takes in variable amount of args and returns biggest
	// compare to larger of two values
	f32 largest = abs_a > abs_b ? abs_a : abs_b;

	if (diff <= largest * acceptable_error)
		return true;

	return false;

}

bool sm::is_zero(f32 value, f32 acceptable_error)
{
	return fabs(value) <= acceptable_error;
}