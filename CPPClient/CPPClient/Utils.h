#pragma once
#include <iostream>
bool IsLittleEndian();
char* ConvertInt16ToEndian(__int16 value);
char* ConvertInt32ToEndian(__int32 value);
void PrintPChar(char* arr, int size);