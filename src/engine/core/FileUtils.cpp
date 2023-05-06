#include "engine/core/FileUtils.h"
#include "engine/core/Debug.h"
#include "engine/core/Assert.h"

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
	long fileLen = ftell(pFile);
	rewind(pFile);

	// alloc buffer
	fileBytes.resize(fileLen);

	// read into buffer
	size_t bytesRead = fread(fileBytes.data(), sizeof(Byte), fileLen, pFile);
	ASSERT(bytesRead == (size_t)fileLen);

	// close file
	fclose(pFile);

	// return buffer
	return fileBytes;
}
