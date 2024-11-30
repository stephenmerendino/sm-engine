#include "Engine/Core/Random_old.h"
#include "Engine/Math/MathUtils.h"
#include <cstdlib>
#include <ctime>

void RandomInit()
{
	srand((U32)time(NULL));
}

F32 RandomNumberBetween(F32 low, F32 high)
{
	// generate between 0 and RAND_MAX
	// map [0, RAND_MAX] => [low, high]
	I32 randNum = ::rand();
	return Remap((F32)randNum, 0.0f, RAND_MAX, low, high);
}

I32 RandomNumberBetween(I32 low, I32 high)
{
	I32 range = high - low;
	return low + (rand() % range);
}

Vec3 RandomDirection()
{
	F32 x = RandomNumberBetween(-1.0f, 1.0f);
	F32 y = RandomNumberBetween(-1.0f, 1.0f);
	F32 z = RandomNumberBetween(-1.0f, 1.0f);
	return Vec3(x, y, z).Normalized();
}
