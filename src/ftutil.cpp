#include <cctype>
#include <string>
#include <fstream>
#include <string.h>
#include "3wd.hpp"
#define MAX(a, b) a > b ? a : b
namespace ft
{
  char* strlwr(char* str) {
    if (!str)
      return NULL;
    size_t i = 0;
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
    ret = new char[MAX(len + 1, PATH_LENGTH)];
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
    *org = new char[MAX(len + 1, PATH_LENGTH)];
    strcpy(*org, str);
    return ;
  }

  void strjoin(char **org, const char *str){
    if (*org == NULL){
      *org = ft::strdup(str);
      return ;
    }
    std::string save = *org;
    delete[] *org;
    *org = NULL;

    size_t sLen = strlen(str);
    *org = new char[save.size() + sLen + 2];
    size_t i = 0;
    for(; i < save.size(); ++i){
      (*org)[i] = save[i];     // *org[i] はSEGF
    }
    size_t j = 0;
    for(; j < sLen; ++j){
      (*org)[i + j] = str[j];
    }
    (*org)[i + j] = '\0';
    return ;
  }

} // namespace ft
