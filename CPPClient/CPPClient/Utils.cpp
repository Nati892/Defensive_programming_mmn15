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

/// <summary>
/// this function is unsafe in the matter that the uuid param should be at least 16 bytes in length
/// any less then that and the thing will throw an exception
/// </summary>
/// <param name="uuid"> a char* uuid that should be at least 16 bytes in length!</param>
/// <returns>a string representing the 16 bytes in a double char hex for each char</returns>
std::string AsciiToHexStr(char* uuid) {
	if (uuid == nullptr) {
		// Handle null pointer case
		return "Invalid UUID (null pointer)";
	}
	std::string hex_str = "";
	for (int i = 0; i < 16; i++)
	{
		hex_str = hex_str + charToHex(uuid[i]);
	}

	return hex_str;
}



std::string charToHex(char c) {
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(2) << std::hex << (int)(unsigned char)c;
	return stream.str();
}


char hexPairToChar(const std::string& hexPair) {
	if (hexPair.size() != 2) {
		throw std::invalid_argument("Input must be a 2-character hex string.");
	}

	try {
		size_t pos;
		int charValue = std::stoi(hexPair, &pos, 16);

		if (pos != hexPair.size()) {
			throw std::invalid_argument("Invalid hex string: " + hexPair);
		}

		return static_cast<char>(charValue);
	}
	catch (const std::invalid_argument& e) {
		throw std::invalid_argument("Invalid hex string: " + hexPair);
	}
}
/// <summary>
///  Converts a hex-encoded string to its corresponding ASCII representation.
/// </summary>
/// <param name="hexString">hexString Null-terminated hex-encoded string with even length.</param>
/// <returns>ASCII representation of the hex string</returns>
std::string hexStringToAscii( char* constHexString, int str_len) {
	char* hexString = new char[str_len];
	std::memcpy(hexString, constHexString, str_len);
	if (hexString == nullptr) {
		throw std::invalid_argument("Input hex string is null.");
	}

	if (std::strlen(hexString) % 2 != 0) {
		throw std::invalid_argument("Input hex string length must be even.");
	}

	char ascii[16];
	for (size_t i = 0; i < str_len; i += 2) {
		std::string hexPair = { hexString[i], hexString[i + 1] };
		try {
			ascii[i/2] =hexPairToChar(hexPair);
		}
		catch (std::exception& e) {}
	}

	return std::string(ascii);
}

//custom trim func
std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\n\r");
	size_t last = str.find_last_not_of(" \t\n\r");

	if (first == std::string::npos || last == std::string::npos) {
		return ""; // Empty or whitespace-only string
	}

	return str.substr(first, last - first + 1);
}