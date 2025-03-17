#include "sm/io/file.h"
#include "sm/core/debug.h"

#include <cstdio>
#include <cerrno>

using namespace sm;

array_t<byte_t> sm::file_load_bytes(arena_t* arena, const char* filename)
{
	array_t<byte_t> data;

	// open file for read and binary
	FILE* p_file = NULL;
	fopen_s(&p_file, filename, "rb");
	if (NULL == p_file)
	{
		debug_printf("Failed to open file [%s] to read bytes\n", filename);
		return data;
	}

	// measure size
	fseek(p_file, 0, SEEK_END);
	long file_len = ftell(p_file);
	rewind(p_file);

	// alloc buffer
	data = array_init_sized<byte_t>(arena, file_len);

	// read into buffer
	size_t bytes_read = fread(data.data, sizeof(byte_t), file_len, p_file);
	SM_ASSERT(bytes_read == (size_t)file_len);

	// close file
	fclose(p_file);

	// return buffer
	return data;
}
