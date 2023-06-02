#include "engine/core/file.h"
#include "engine/core/debug.h"
#include "engine/core/assert.h"

#include <cstdio>
#include <cerrno>

std::vector<byte_t> read_binary_file(const char* filename)
{
	std::vector<byte_t> file_bytes;

	// open file for read and binary
	FILE* p_file = NULL;
	fopen_s(&p_file, filename, "rb");
	if (NULL == p_file)
	{
		debug_printf("Failed to open file [%s] to read bytes\n", filename);
		return file_bytes;
	}

	// measure size
	fseek(p_file, 0, SEEK_END);
	long file_len = ftell(p_file);
	rewind(p_file);

	// alloc buffer
	file_bytes.resize(file_len);

	// read into buffer
	size_t bytes_read = fread(file_bytes.data(), sizeof(byte_t), file_len, p_file);
	SM_ASSERT(bytes_read == (size_t)file_len);

	// close file
	fclose(p_file);

	// return buffer
	return file_bytes;
}
