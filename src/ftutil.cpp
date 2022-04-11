#include <cctype>
#include <string>
#include <fstream>

namespace ft
{
  char* strlwr(char* str) {
    int i = 0;
    while (str[i]) {
      str[i] = tolower(str[i]);
      ++i;
    }
    return str;
  }

  int stricmp(const char* s1, const char* s2) {
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2)
      if (*s1 == '\0')
        return 0;
    return ((*(unsigned char*)s1 < *(unsigned char*)s2) ? -1 : +1);
  }

  void copyFile(std::string from, std::string to) {
    std::ifstream src(from, std::ios::binary);
    std::ofstream dst(to, std::ios::binary);
    dst << src.rdbuf();
  }
} // namespace ft
