//
// WWW Server  File: http10.cpp
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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "socket.hpp"
#include "defines.hpp"
#include "3wd.hpp"
#include "config.hpp"
#include "scodes.hpp"
#include "http10.hpp"
#include "http11.hpp"
#include "util.hpp"
#include "cgi.hpp"

// ------------------------------------------------------------------
//
// FindType()
//
// This function compares the file extension of the document
// being sent to determine the proper MIME type to return
// to the client.
//

int FindType(char* szPath) {
  char* szTmp;
  int   i;

  szTmp = szPath + strlen(szPath) - 1;

  while ((*szTmp != '.') && (szTmp != szPath) && (*szTmp != '/')) {
    szTmp--;
  }

  szTmp++; // Advance past the '.' or '/'.

  for (i = 0; i < iNumTypes; i++) {
    if (ft::stricmp(eExtMap[i].szExt, szTmp) == 0) {
      return i;
    }
  }
  return 0;
}

// ------------------------------------------------------------------
//
// ResolvePath()
//
// Resolve the path given by the client to the real path
// by checking against the aliases.
//

char* ResolvePath(char* szUri) {
  int   i;
  char *szRest, *szRoot;
  bool  bFound = false;

  if (strcmp(szUri, "/") == 0) // They asked for the root directory doc.
  {
    szRoot = strdup(szServerRoot);
    return szRoot;
  }

  // Now isolate the first component of the requested path.
  szRest = szUri;
  szRoot = new char[PATH_LENGTH];

  szRoot[0] = *szRest;
  szRest++; // Advance past the initial '/'.
  i = 1;
  while ((*szRest != '/') && (*szRest != '\\') && (*szRest)) {
    szRoot[i] = *szRest;
    i++;
    szRest++;
  }

  // Now we have the first component.
  szRoot[i] = '\0';
  if (*szRest != NULL)
    szRest++; // Advance past the '/'.

  // Compare it to our list of aliases.
  for (i = 0; i < iNumPathAliases; i++) {
    // Case insensitive comparison.
    if (ft::stricmp(szRoot, pAliasPath[i].szAlias) == 0) {
      memset(szRoot, 0, PATH_LENGTH);
      sprintf(szRoot, "%s%s", pAliasPath[i].szTrue, szRest);
      bFound = true;
      break;
    }
  }

  if (bFound == true)
    return (szRoot); // Found.

  // Give them a path based on the default root.
  if (*szRest == NULL) {
    if (*szUri == '/') {
      szRest = szUri + 1;
    } else {
      szRest = szUri;
    }
    memset(szRoot, 0, PATH_LENGTH);
    sprintf(szRoot, "%s%s", szServerRoot, szRest);
    return (szRoot);
  }

  return (NULL);
}

// ------------------------------------------------------------------
//
// ResolveExec()
//
// Resolve the given exec path to the real path.
//

char* ResolveExec(char* szUri) {
  int   i;
  char *szRest, *szRoot;
  bool  bFound = false;

  szRest = szUri;
  szRoot = new char[PATH_LENGTH];

  szRoot[0] = *szRest;
  szRest++; // Advance past the initial '/'.
  i = 1;
  while ((*szRest != '/') && (*szRest != '\\') && (*szRest)) {
    szRoot[i] = *szRest;
    i++;
    szRest++;
  }

  szRoot[i] = '\0';
  if (*szRest != NULL)
    szRest++; // Advance past the '/'.

  // Compare to the list of exec path aliases.
  for (i = 0; i < iNumExecAliases; i++) {
    if (ft::stricmp(szRoot, pAliasExec[i].szAlias) == 0) {
      memset(szRoot, 0, PATH_LENGTH);
      sprintf(szRoot, "%s%s", pAliasExec[i].szTrue, szRest);
      bFound = true;
      break;
    }
  }

  // Return true if found. NULL otherwise.
  if (bFound == true)
    return (szRoot);
  delete[] szRoot;
  return (NULL);
}

// ------------------------------------------------------------------
//
// SendError()
//
// Send an error message back to the client.
//

int SendError(
    Socket* sClient, char* szReason, int iCode, char* szVer, Headers* hInfo) {
  struct stat   sBuf;
  std::ofstream ofTmp;
  char *        szTmp, szBuf[PATH_LENGTH];
  int           iRc;

  szTmp = tmpnam(NULL);
  ofTmp.open(szTmp);
  if (! ofTmp) {
    sClient->Send(sz500); // Unable to get temp file, fail.
    return 500;
  }
  // Write the temp file with the info.
  ofTmp << "<!doctype html public \"-//IETF//DTD HTML 2.0//EN\">" << std::endl;
  ofTmp << "<html><head>" << std::endl;
  ofTmp << "<title>Error</title></head>" << std::endl;
  ofTmp << "<body><h2>Error...</h2>Your request could not be honored.<hr><b>"
        << std::endl;
  ofTmp << szReason << std::endl;
  ofTmp << "</b><hr><em>HTTP Response Code:</em> " << iCode << "<br>"
        << std::endl;
  ofTmp << "<em>From server at:</em> " << szHostName << "<br>" << std::endl;
  ofTmp << "<em>Running:</em> " << szServerVer << "</body></html>" << std::endl;
  ofTmp.close();

  sprintf(szBuf, "%s %d\r\n", szVer, iCode);
  sClient->Send(szBuf);
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\nContent-Type: text/html\r\n");
  iRc = stat(szTmp, &sBuf);
  if (iRc == 0) {
    hInfo->ulContentLength = sBuf.st_size;
    sprintf(szBuf, "Content-Length: %d\r\n", sBuf.st_size);
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->SendText(szTmp); // Send the file.
  } else {
    hInfo->ulContentLength = 0;
    strcpy(szBuf, "Content-Length: 0\r\n");
    sClient->Send(szBuf);
    sClient->Send("\r\n");
  }
  unlink(szTmp); // Get rid of it.
  return iCode;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
