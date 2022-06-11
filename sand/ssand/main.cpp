#include <unistd.h>
#include <fcntl.h>           /* AT_* 定数の定義 */
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;

int main(){
  struct stat   sBuf;
  std::ofstream ofTmp;
  char* szTmp;
  int           iRc;

  szTmp = strdup("tmp/tmpFile/aaaa.html");
  ofTmp.open(szTmp);
  // Write the temp file with the info.
  ofTmp << "<!doctype html public \"-//IETF//DTD HTML 2.0//EN\">" << std::endl;
  ofTmp.close();
  int uu = unlink(szTmp); // Get rid of it.
  std::cerr << "HHHHHHHHHHHHHHHHHHH" << " " << uu << " " << szTmp << " " << std::endl;

  return 0;
}