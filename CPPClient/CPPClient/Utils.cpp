#include "Utils.h"
bool IsLittleEndian() {
	// Use union to interpret the bytes of a short
	union {
		short s;
		char c[2];
	} test;

	test.s = 1;
	return (test.c[0] == 1);
}

char* ConvertInt16ToEndian(__int16 value) {
	char* result = new char[2];
	if (IsLittleEndian()) {
		// Little-endian platform
		result[0] = static_cast<char>(value & 0xFF);
		result[1] = static_cast<char>((value >> 8) & 0xFF);
	}
	else {
		// Big-endian platform
		result[0] = static_cast<char>((value >> 8) & 0xFF);
		result[1] = static_cast<char>(value & 0xFF);
	}
	return result;
}

char* ConvertInt32ToEndian(__int32 value) {
	char* result = new char[4];
	if (IsLittleEndian()) {
		// Little-endian platform
		result[0] = static_cast<char>(value & 0xFF);
		result[1] = static_cast<char>((value >> 8) & 0xFF);
		result[2] = static_cast<char>((value >> 16) & 0xFF);
		result[3] = static_cast<char>((value >> 24) & 0xFF);
	}
	else {
		// Big-endian platform
		result[0] = static_cast<char>((value >> 24) & 0xFF);
		result[1] = static_cast<char>((value >> 16) & 0xFF);
		result[2] = static_cast<char>((value >> 8) & 0xFF);
		result[3] = static_cast<char>(value & 0xFF);
	}
	return result;
}

//TOOD delete
void PrintPChar(char* arr, int size)
{
	try {
		for (int i = 0; i < size; i++)
		{
			unsigned char currchar = arr[i];
			unsigned int toprint = 0;
			toprint = (currchar);
			std::cerr << "\n" << i + 1 << " : " << toprint << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "overflow in PrintPChar" << std::endl;
	}
}