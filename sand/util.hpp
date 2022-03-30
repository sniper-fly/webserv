#ifndef UTIL_HPP
#define UTIL_HPP

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include "headers.hpp"

#define PORT           8080
#define ERRSOCK        "Error! Cannot create socket"
#define ERRBIND        "Error! Cannot create socket"
#define ERRLISN        "Error! Listen failed"
#define ERRACPT        "Error! Accept failed"
#define WAIT_QUEUE_LEN 5
#define max(a, b)      a > b ? a : b
#define min(a, b)      a > b ? b : a

// ------------------------------------------------------------------
//
// Authorization codes.
//

#define ACCESS_OK     1 // Allow access.
#define ACCESS_DENIED 2 // Need authorization.
#define ACCESS_FAILED 3 // Credentials failed.

#define WRITE_ACCESS 1 // Check write access
#define READ_ACCESS  2 // Check read access

class Headers;

time_t ConvertDate(char*);
char*  CreateDate(time_t);
int    CheckAuth(char*, Headers*, int);
int    CheckFile(char*, Headers*);
int    BasicCheck(char*, Headers*);

int  RecvLine(int iSocket, char* szBuf, int iLen);
void TalkToClient(int iSocket);

template<typename T>
void puterr(T t) {
  std::cerr << "[" << t << "]" << std::endl;
}

template<typename T, typename S>
void puterr(T t, S s) {
  std::cerr << "[" << t << "]"
            << " " << s << std::endl;
}

template<typename T, typename S, typename U>
void puterr(T t, S s, U u) {
  std::cerr << "[" << t << "]"
            << " " << s << " " << u << std::endl;
}

#endif
