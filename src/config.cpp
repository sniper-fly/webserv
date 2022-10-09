//
// WWW Server  File: config.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>

#include "defines.hpp"
#include "ftutil.hpp"
#include "config.hpp"

// ------------------------------------------------------------------

char *szServerRoot, // The root directory for serving files.
    *szHostName,    // The hostname given out. Overrides the call to hostname().
    *szWelcome,     // The default file to serve.
    *szAccessLog,   // The access log filename.
    *szErrorLog,    // The error log filename.
    *szReadAccess,  // The read access file name.
    *szWriteAccess, // The write access file name.
    *szDeleteDir;   // The directory to store deleted resources.
short sPort;        // The port number to serve.
bool  bDnsLookup,   // Flag whether to do dns reverse lookups.
    bGmtTime;       // Flag whether to use GMT in access log file.
int iNumPathAliases, // The number of path aliases.
    iNumExecAliases, // The number of exec aliases.
    iNumTypes;       // The number of extension types.
long   lGmtOffset;   // The offset in minutes between local time and GMT.
Paths *pAliasPath,   // The set of root aliases.
    *pAliasExec;     // The set of exec aliases.
Extensions* eExtMap; // The set of extensions and types.

// ------------------------------------------------------------------

void SetDefaults();

// ------------------------------------------------------------------

#define Convert(str)                                                           \
  for (int i = 0; i < (int)strlen(str); i++)                                   \
    if (str[i] == '\\')                                                        \
  str[i] = '/'

// ------------------------------------------------------------------
//
// ReadConfig
//
// Read and store the information in the configuration file.
//

int ReadConfig(char* szConfigName) {
  std::ifstream ifIn; // The config file handle.
  char *        szBuf, *szDirective, *szVal1, *szVal2;
  int           iNum1 = 0, iNum2 = 0, iNum3 = 1, i;
  int ret = 0;

  ifIn.open(szConfigName);
  if (! ifIn) {
    std::cerr << "Error!" << std::endl;
    std::cerr << "Could not read configuration file: " << szConfigName
              << std::endl;
    return 1;
  }

  SetDefaults();

  szBuf       = new char[SMALLBUF];
  szDirective = new char[SMALLBUF];
  szVal1      = new char[SMALLBUF];
  szVal2      = new char[SMALLBUF];

  pAliasPath          = new Paths[MAX_ALIASES];
  pAliasExec          = new Paths[MAX_ALIASES];
  eExtMap             = new Extensions[MAX_EXTENSIONS];
  eExtMap[0].szExt    = new char[1];
  eExtMap[0].szExt[0] = '\0';
  if (eExtMap[0].szType){
    delete[] eExtMap[0].szType;
  }
  ft::strdup(&eExtMap[0].szType, "application/octet-stream");

  while (! ifIn.eof()) // Until the end of the file.
  {
    memset(szBuf, 0, SMALLBUF);
    memset(szDirective, 0, SMALLBUF);
    // 改行==\r\n なので、\rまで読み取る
    ifIn.getline(szBuf, SMALLBUF, '\n');
    if ((szBuf[0] == '#') || (szBuf[0] == '\0') || (szBuf[0] == '\r'))
      continue; // Skip comments.

    sscanf(szBuf, "%s %s %s", szDirective, szVal1, szVal2); // Parse the line.

    if (ft::stricmp(szDirective, "ServerRoot") == 0) {
      if (szServerRoot)
        delete[] szServerRoot;
      Convert(szVal1);
      if (szVal1[strlen(szVal1) - 1] != '/') {
        szServerRoot = new char[strlen(szVal1) + 2];
        sprintf(szServerRoot, "%s/", szVal1);
      } else {
        ft::strdup(&szServerRoot, szVal1);
      }
    } else if (ft::stricmp(szDirective, "HostName") == 0) {
      ft::strdup(&szHostName, szVal1);
    } else if (ft::stricmp(szDirective, "GMTOffset") == 0) {
      lGmtOffset = 60 * (atol(szVal1) / 100); // Number of hours in minutes
      lGmtOffset += (atol(szVal1) % 100);     // Number of minutes specified
      lGmtOffset *= 60;                       // Convert minutes to seconds
    } else if (ft::stricmp(szDirective, "Welcome") == 0) {
      ft::strdup(&szWelcome, szVal1);
    } else if (ft::stricmp(szDirective, "AccessLog") == 0) {
      ft::strdup(&szAccessLog, szVal1);
    } else if (ft::stricmp(szDirective, "ErrorLog") == 0) {
      if (szErrorLog)
        delete[] szErrorLog;
      ft::strdup(&szErrorLog, szVal1);
    } else if (ft::stricmp(szDirective, "DeleteDir") == 0) {
      if (szDeleteDir)
        delete[] szDeleteDir;
      Convert(szVal1);
      if (szVal1[strlen(szVal1) - 1] != '/') {
        szDeleteDir = new char[strlen(szVal1) + 2];
        sprintf(szDeleteDir, "%s/", szVal1);
      } else {
        ft::strdup(&szDeleteDir, szVal1);
      }
    } else if (ft::stricmp(szDirective, "Port") == 0) {
      sPort = (short)atoi(szVal1);
    } else if (ft::stricmp(szDirective, "DNSLookup") == 0) {
      if (ft::stricmp(szVal1, "Off") == 0) {
        bDnsLookup = false;
      }
    } else if (ft::stricmp(szDirective, "LogTime") == 0) {
      if (ft::stricmp(szVal1, "GMT") == 0) {
        bGmtTime = true;
      }
    } else if (ft::stricmp(szDirective, "ReadAccessName") == 0) {
      ft::strdup(&szReadAccess, szVal1);
    } else if (ft::stricmp(szDirective, "WriteAccessName") == 0) {
      ft::strdup(&szWriteAccess, szVal1);
    } else if (ft::stricmp(szDirective, "PathAlias") == 0) {
      if (iNum1 == MAX_ALIASES) {
        std::cerr << "Exceeded maximum path aliases. " << szVal1 << " ignored."
                  << std::endl;
        continue;
      }
      ft::strdup(&pAliasPath[iNum1].szAlias, szVal1);
      pAliasPath[iNum1].szTrue  = new char[strlen(szVal2) + 2];
      strcpy(pAliasPath[iNum1].szTrue, szVal2);
      Convert(pAliasPath[iNum1].szAlias);
      Convert(pAliasPath[iNum1].szTrue);

      i = strlen(pAliasPath[iNum1].szTrue);
      if (pAliasPath[iNum1].szTrue[i - 1] != '/') {
        pAliasPath[iNum1].szTrue[i]     = '/';
        pAliasPath[iNum1].szTrue[i + 1] = '\0';
      }

      iNum1++;
    } else if (ft::stricmp(szDirective, "ExecAlias") == 0) {
      if (iNum2 == MAX_ALIASES) {
        std::cerr << "Exceeded maximum exec aliases. " << szVal1 << " ignored."
                  << std::endl;
        continue;
      }
      ft::strdup(&pAliasExec[iNum2].szAlias, szVal1);
      pAliasExec[iNum2].szTrue  = new char[strlen(szVal2) + 2];
      strcpy(pAliasExec[iNum2].szTrue, szVal2);
      Convert(pAliasExec[iNum2].szAlias);
      Convert(pAliasExec[iNum2].szTrue);

      i = strlen(pAliasExec[iNum2].szTrue);
      if (pAliasExec[iNum2].szTrue[i - 1] != '/') {
        pAliasExec[iNum2].szTrue[i]     = '/';
        pAliasExec[iNum2].szTrue[i + 1] = '\0';
      }

      iNum2++;
    } else if (ft::stricmp(szDirective, "ExtType") == 0) {
      if (iNum3 == MAX_EXTENSIONS) {
        std::cerr << "Exceeded maximum extensions. " << szVal1 << " ignored."
                  << std::endl;
        continue;
      }
      ft::strdup(&eExtMap[iNum3].szExt, szVal1);
      ft::strdup(&eExtMap[iNum3].szType, szVal2);

      iNum3++;
    }
    else{
      std::cerr << "[ReadConfig] szBuf(" << szBuf << ") is invalid config format" << std::endl;
      ret = 1;
    }
  }

  ifIn.close();

  iNumPathAliases = iNum1;
  iNumExecAliases = iNum2;
  iNumTypes       = iNum3;

  delete[] szBuf;
  delete[] szDirective;
  delete[] szVal1;
  delete[] szVal2;

  for (i = 0; i < iNumTypes; i++) {
    if (strstr(eExtMap[i].szType, "text/") == NULL) {
      eExtMap[i].bBinary = true;
    }
  }

  return ret;
}

// ------------------------------------------------------------------
//
// SetDefaults
//
// Set some initial default values in case they're not specified
// in the configuration file.
//

void SetDefaults() {
  sPort         = WWW_PORT;
  bDnsLookup    = true;
  bGmtTime      = false;
  ft::strdup(&szReadAccess, HTREADACCESS);
  ft::strdup(&szWriteAccess, HTWRITEACCESS);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
