//
// WWW Server  File: 3wd.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// Function and Variable declarations for 3wd.cpp
//

#ifndef _3WD_HPP_
#define _3WD_HPP_

#include <semaphore.h>
#include "socket.hpp"

// ------------------------------------------------------------------
// Function Prototypes.
//

void  WriteToLog(Socket*, char*, int, long);
void  W3Conn(void*);
void  Server();
char* DeHexify(char*);
char  Hex2Char(char);

// ------------------------------------------------------------------
//
// Variable declarations.
//

extern volatile int iAccessLock;
extern volatile int iErrorLock;
extern const char   szMonth[12][4];
extern const char   szDay[7][4];

extern sem_t* g_logSem;
extern sem_t* g_cgiSem;

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
