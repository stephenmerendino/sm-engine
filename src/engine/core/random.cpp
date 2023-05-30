#include "engine/core/random.h"
#include <time.h>

void random_init()
{
   srand(time(NULL)); 
}

f32 random_number_between(f32 low, f32 high)
{
   // generate between 0 and RAND_MAX
   // map [0, RAND_MAX] => [low, high]
   i32 rand_num = rand();
   return remap(rand_num, 0, RAND_MAX, low, high);
}

i32 random_number_between(i32 low, i32 high)
{
    i32 range = high - low; 
    return low + (rand() % range);
}

vec3 random_unit_vector()
{
    f32 x = random_number_between(-1.0f, 1.0f); 
    f32 y = random_number_between(-1.0f, 1.0f); 
    f32 z = random_number_between(-1.0f, 1.0f); 
    return get_normalized(make_vec3(x, y, z));
}
