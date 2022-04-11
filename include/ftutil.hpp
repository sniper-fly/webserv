#ifndef FTUTIL_HPP
#define FTUTIL_HPP

#include <string>

namespace ft
{
  char* strlwr(char* str);
  int   stricmp(const char* s1, const char* s2);
  void  copyFile(std::string from, std::string to);
} // namespace ft
#endif
