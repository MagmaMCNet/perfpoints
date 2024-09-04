#include <iomanip>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <fstream>
#define WIN32_LEAN_AND_MEAN
#pragma region CRC32
static uint32_t crc32_table[256];
static bool crc32_initialized = false;
void initialize_crc32_table() {
    const uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 8; j > 0; --j) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
}
uint32_t crc32(const std::string& data) {
    if (!crc32_initialized) {
        initialize_crc32_table();
        crc32_initialized = true;
    }
    uint32_t crc = 0xFFFFFFFF;
    for (char byte : data) {
        uint8_t table_index = static_cast<uint8_t>(crc ^ byte);
        crc = (crc >> 8) ^ crc32_table[table_index];
    }
    return crc ^ 0xFFFFFFFF;
}
std::string crc32ToHex(uint32_t crc) {
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << crc;
    return ss.str();
}

#pragma endregion


void EnableANSIColors(bool value) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode)) {
        if (value)
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        else
            dwMode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, dwMode);
    }
}
void ClearLastLines(int num_lines) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    COORD coord = { 0, 0 };

    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        int currentY = csbi.dwCursorPosition.Y;
        int x = 0;
        int y = currentY - num_lines + 1;

        for (int i = 0; i < num_lines; ++i) {
            coord.Y = y + i;
            SetConsoleCursorPosition(hConsole, coord);
            FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
            FillConsoleOutputAttribute(hConsole, csbi.wAttributes, csbi.dwSize.X, coord, &written);
        }

        SetConsoleCursorPosition(hConsole, coord);  // Reset the cursor position
    }
}
void MoveCursorDown(int lines) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD coord = { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y + lines };
    SetConsoleCursorPosition(hConsole, coord);
}

std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string getMotherboardHWID() {
    system("wmic baseboard get serialnumber > motherboard_info.txt");

    std::ifstream file("motherboard_info.txt");
    if (!file.is_open())
        return "";

    std::string serialNumber;
    std::string line;

    std::getline(file, line);

    if (std::getline(file, line)) {
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c) {
            return c < 32 || c == 127 || !std::isprint(c);
            }), line.end());
        serialNumber = line;
    }

    file.close();
    std::remove("motherboard_info.txt");
    return serialNumber;
}

std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), [](unsigned char c) {
        return std::tolower(c);
        });
    return lower_str;
}

void HexToRGB(const std::string& hex, int& r, int& g, int& b) {
    std::stringstream ss;
    ss << std::hex << hex.substr(0, 2);
    ss >> r;
    ss.clear();
    ss << std::hex << hex.substr(2, 2);
    ss >> g;
    ss.clear();
    ss << std::hex << hex.substr(4, 2);
    ss >> b;
}
void BlendColor(int r1, int g1, int b1, int r2, int g2, int b2, float ratio, int& r, int& g, int& b) {
    r = static_cast<int>(r1 + ratio * (r2 - r1));
    g = static_cast<int>(g1 + ratio * (g2 - g1));
    b = static_cast<int>(b1 + ratio * (b2 - b1));
}
std::string Color_Text(const std::string& text, const std::string& hexFrom, const std::string& hexTo) {
    int r1, g1, b1, r2, g2, b2;
    HexToRGB(hexFrom, r1, g1, b1);
    HexToRGB(hexTo, r2, g2, b2);

    int length = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if ((text[i] & 0xC0) != 0x80) {
            ++length;
        }
    }

    std::string coloredText;
    int charIndex = 0;
    for (size_t i = 0; i < text.length();) {
        float ratio = static_cast<float>(charIndex) / static_cast<float>(length - 1);
        int r, g, b;
        BlendColor(r1, g1, b1, r2, g2, b2, ratio, r, g, b);

        std::stringstream ss;
        ss << "\033[38;2;" << r << ";" << g << ";" << b << "m";

        if ((text[i] & 0x80) == 0) {
            ss << text[i];
            ++i;
        }
        else if ((text[i] & 0xE0) == 0xC0) {
            ss << text[i] << text[i + 1];
            i += 2;
        }
        else if ((text[i] & 0xF0) == 0xE0) {
            ss << text[i] << text[i + 1] << text[i + 2];
            i += 3;
        }
        else if ((text[i] & 0xF8) == 0xF0) {
            ss << text[i] << text[i + 1] << text[i + 2] << text[i + 3];
            i += 4;
        }
        coloredText += ss.str();
        ++charIndex;
    }

    coloredText += "\033[0m";

    return coloredText;
}


size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}