#include <cctype>
#include <string>
#include <fstream>
#include <string.h>
#include "3wd.hpp"
namespace ft
{
  char* strlwr(char* str) {
    if (!str)
      return NULL;
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

  void copyFile(const char* from, const char* to) {
    std::ifstream src(from, std::ios::binary);
    std::ofstream dst(to, std::ios::binary);
    dst << src.rdbuf();
  }

  char* strdup(const char *str){
    if (str == NULL)
      return NULL;
    size_t len = strlen(str);
    char *ret;
    ret = new char[len + 1];
    strcpy(ret, str);
    return ret;
  }

  void strdup(char **org, const char *str){
    if (*org){
      delete[] *org;
      *org = NULL;
    }
    if (str == NULL)
      return ;
    size_t len = strlen(str);
    *org = new char[len + 1];
    strcpy(*org, str);
    return ;
  }

} // namespace ft
