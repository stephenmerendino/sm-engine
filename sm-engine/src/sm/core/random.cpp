#include "sm/core/random.h"
#include <cstdlib>
#include <ctime>

using namespace sm;

void sm::random_init()
{
	::srand((u32)time(NULL));
}

f32 sm::random_gen_zero_to_one()
{
	i32 num = ::rand();
	return (f32)num / (f32)RAND_MAX;
}

f32 sm::random_gen_between(f32 low, f32 high)
{
	f32 range = high - low;
	return low + (random_gen_zero_to_one() * range);
}

i32 sm::random_gen_between(i32 low, i32 high)
{
	i32 range = high - low;
	return low + (::rand() % range);
}
