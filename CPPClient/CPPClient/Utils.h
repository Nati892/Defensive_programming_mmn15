#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
bool IsLittleEndian();
char* ConvertInt16ToEndian(__int16 value);
char* ConvertInt32ToEndian(__int32 value);
void PrintPChar(char* arr, int size);
std::string AsciiToHexStr(char* uuid);
std::string hexStringToAscii( char* hexString,int str_len);
std::string trim(const std::string& str);
std::string charToHex(char c);
