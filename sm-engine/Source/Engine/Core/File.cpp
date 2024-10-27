#include "Engine/Core/File.h"
#include "Engine/Core/Debug_Old.h"
#include "Engine/Core/Assert.h"

#include <cstdio>
#include <cerrno>

std::vector<Byte> ReadBinaryFile(const char* filename)
{
	std::vector<Byte> fileBytes;

	// open file for read and binary
	FILE* pFile = NULL;
	fopen_s(&pFile, filename, "rb");
	if (NULL == pFile)
	{
		DebugPrintf("Failed to open file [%s] to read bytes\n", filename);
		return fileBytes;
	}

	// measure size
	fseek(pFile, 0, SEEK_END);
	long file_len = ftell(pFile);
	rewind(pFile);

	// alloc buffer
	fileBytes.resize(file_len);

	// read into buffer
	size_t bytes_read = fread(fileBytes.data(), sizeof(Byte), file_len, pFile);
	SM_ASSERT(bytes_read == (size_t)file_len);

	// close file
	fclose(pFile);

	// return buffer
	return fileBytes;
}
