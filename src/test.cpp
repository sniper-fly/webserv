#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "signal.hpp"
#include "socket.hpp"
#include "defines.hpp"
#include "3wd.hpp"
#include "config.hpp"
#include "scodes.hpp"
#include "headers.hpp"
#include "http10.hpp"
#include "http11.hpp"
#include "util.hpp"
#include "ftutil.hpp"
#include "cgi.hpp"
#include "test.hpp"


bool TEST(){
  bool ret = false;
  {
    {
      char S[] = "%23";
      DeHexify(S);
      if (ft::stricmp(S, "#")){
        ret = true;
        std::cerr << "[DeHexify]" << std::endl;
        std::cerr << "src=" << "%23" << std::endl;
        std::cerr << "dest=" << S << std::endl;
      }
    }
  }
  {
    char *a = NULL, *b = NULL;
    ft::strdup(&a, "abcde");
    b = ft::strdup("12345");
    ft::strjoin(&a, b);
    if (ft::stricmp(a, "abcde12345")){
      ret = true;
      std::cerr << "a=" << a << std::endl;
      std::cerr << "b=" << b << std::endl;
    }
    delete[] a;
    delete[] b;
  }
  {

  }
  return ret;
}