#ifndef BLIB_H
#define BLIB_H

#include <string>

uint32_t crc32(const std::string& data);
std::string crc32ToHex(uint32_t crc);

void EnableANSIColors(bool value);
void ClearLastLines(int num_lines);
void MoveCursorDown(int lines);

std::wstring stringToWString(const std::string& str);
std::string getMotherboardHWID();
std::string to_lower(const std::string& str);
void HexToRGB(const std::string& hex, int& r, int& g, int& b);
std::string Color_Text(const std::string& text, const std::string& hexFrom, const std::string& hexTo);

size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

#endif // BLIB_H