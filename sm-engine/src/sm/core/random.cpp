#include "sm/core/random.h"
#include <cstdlib>
#include <ctime>

using namespace sm;

void sm::init_random()
{
	::srand((u32)time(NULL));
}

f32 sm::gen_random_01()
{
	i32 num = ::rand();
	return (f32)num / (f32)RAND_MAX;
}

f32 sm::gen_random_between(f32 low, f32 high)
{
	f32 range = high - low;
	return low + (gen_random_01() * range);
}

i32 sm::gen_random_between(i32 low, i32 high)
{
	i32 range = high - low;
	return low + (::rand() % range);
}
