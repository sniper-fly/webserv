#ifndef FTUTIL_HPP
#define FTUTIL_HPP

#include <string>

namespace ft
{
  char* strlwr(char* str);
  int   stricmp(const char* s1, const char* s2);
  void  copyFile(const char* from, const char* to);
  char* strdup(const char *);
  // orgをdeleteし、strのnewを入れる
  void strdup(char **org, const char *str);
} // namespace ft
#endif
