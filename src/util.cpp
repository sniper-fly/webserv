//
// WWW Server  File: util.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defines.hpp"
#include "config.hpp"
#include "socket.hpp"
#include "3wd.hpp"
#include "http10.hpp"
#include "util.hpp"
#include "base64.hpp"
#include "ftutil.hpp"

// ------------------------------------------------------------------
//
// ConvertDate()
//
// This routine will convert from any of the three recognized
// date formats used in HTTP/1.0 to a time_t value, the number
// of seconds since the epoch date: 00:00:00 1 January 1970 GMT.

time_t ConvertDate(char* szDate) {
  char      szMonth[64]; // Allow extra for bad formats.
  struct tm tmData;

  if (strlen(szDate) > 34) // Catch bad/unknown formatting.
  {
    return ((time_t)0);
  }

  if (szDate[3] == ',') // RFC 822, updated by RFC 1123
  {
    sscanf(szDate, "%*s %d %s %d %d:%d:%d %*s", &(tmData.tm_mday), szMonth,
        &(tmData.tm_year), &(tmData.tm_hour), &(tmData.tm_min),
        &(tmData.tm_sec));
    tmData.tm_year -= 1900;
  } else if (szDate[3] == ' ') // ANSI C's asctime() format
  {
    sscanf(szDate, "%*s %s %d %d:%d:%d %d", szMonth, &(tmData.tm_mday),
        &(tmData.tm_hour), &(tmData.tm_min), &(tmData.tm_sec),
        &(tmData.tm_year));
    tmData.tm_year -= 1900;
  } else if (isascii(szDate[3])) // RFC 850, obsoleted by RFC 1036
  {
    sscanf(szDate, "%*s %d-%3s-%d %d:%d:%d %*s", &(tmData.tm_mday), szMonth,
        &(tmData.tm_year), &(tmData.tm_hour), &(tmData.tm_min),
        &(tmData.tm_sec));
  } else // Unknown time format
  {
    return ((time_t)0);
  }

  // Now calculate the number of seconds since 1970.
  if (ft::stricmp(szMonth, "jan") == 0)
    tmData.tm_mon = 0;
  else if (ft::stricmp(szMonth, "feb") == 0)
    tmData.tm_mon = 1;
  else if (ft::stricmp(szMonth, "mar") == 0)
    tmData.tm_mon = 2;
  else if (ft::stricmp(szMonth, "apr") == 0)
    tmData.tm_mon = 3;
  else if (ft::stricmp(szMonth, "may") == 0)
    tmData.tm_mon = 4;
  else if (ft::stricmp(szMonth, "jun") == 0)
    tmData.tm_mon = 5;
  else if (ft::stricmp(szMonth, "jul") == 0)
    tmData.tm_mon = 6;
  else if (ft::stricmp(szMonth, "aug") == 0)
    tmData.tm_mon = 7;
  else if (ft::stricmp(szMonth, "sep") == 0)
    tmData.tm_mon = 8;
  else if (ft::stricmp(szMonth, "oct") == 0)
    tmData.tm_mon = 9;
  else if (ft::stricmp(szMonth, "nov") == 0)
    tmData.tm_mon = 10;
  else if (ft::stricmp(szMonth, "dec") == 0)
    tmData.tm_mon = 11;

  tmData.tm_isdst = 0; // There should be no daylight savings time factor.

  return (mktime(&tmData));
}

// ------------------------------------------------------------------
//
// CreateDate()
//
// This function accepts a time_t value and converts it to
// a properly formatted string for HTTP/1.0.
//

char* CreateDate(time_t ttTime) {
  struct tm* tmData;
  char*      szDate;

  ttTime += lGmtOffset; // Adjust for GMT offset from local.

  tmData = localtime(&ttTime);

  if (tmData == NULL)
    return NULL; // Check for errors.

  szDate = new char[32];
  sprintf(szDate, "%s, %02d %s %4d %02d:%02d:%02d GMT", szDay[tmData->tm_wday],
      tmData->tm_mday, szMonth[tmData->tm_mon], (tmData->tm_year + 1900),
      tmData->tm_hour, tmData->tm_min, tmData->tm_sec);

  return (szDate);
}

// ------------------------------------------------------------------
//
// CheckAuth()
//
// This function will scan the directory tree for an access file.
// If found it will either verify the authorization if present in the
// Headers variable and return a challenge otherwise.
//

int CheckAuth(char* szPath, Headers* hInfo, int iType) {
  char        *szTmpPath = NULL, *szName = NULL;
  int         l, iRc;
  bool        bNotFound = true;
  struct stat sBuf;

  if (!szPath){
    return ACCESS_OK;
  }
  if (iType == READ_ACCESS) // Check for read or write access.
  {
    szName = szReadAccess;
  } else {
    szName = szWriteAccess;
  }

  // bPersistent == false のときNULL
  ft::strdup(&szTmpPath, szPath);

  l = strlen(szTmpPath) - 1;

  // Look for the access filename.
  while (bNotFound) {
    while ((l > 0) && (szTmpPath[l] != '/')) {
      szTmpPath[l] = '\0';
      l--;
    }

    if (l == 0)
      break; // Stop. No more path left.
    l--;     // Go before the "/" for the next attempt.

    strcat(szTmpPath, szName); // Create filename.
    iRc = stat(szTmpPath, &sBuf);
    if (iRc == 0) // We found the file.
    {
      iRc       = CheckFile(szTmpPath, hInfo);
      bNotFound = false;
      continue;
    }
  }
  delete[] szTmpPath;

  if (bNotFound == true) // No access file found. Return ok.
  {
    return (ACCESS_OK);
  }

  return (iRc);
}

// ------------------------------------------------------------------
//
// CheckFile()
//
// This function reads the access file to determine what
// authorizations the client needs in order to connect.
//

int CheckFile(char* szFile, Headers* hInfo) {
  std::ifstream ifIn;
  int           iRc = ACCESS_FAILED;
  char *szType = NULL, *szRealm = NULL, *szUserFile = NULL, *szBuf = NULL, *szVal = NULL;

  ifIn.open(szFile);
  if (! ifIn) {
    return (ACCESS_FAILED);
  }

  szBuf = new char[SMALLBUF];
  szVal = new char[SMALLBUF];

  // Read the parameters.
  while (! ifIn.eof()) {
    memset(szBuf, 0, SMALLBUF);
    ifIn.getline(szBuf, SMALLBUF, '\n');

    if (strstr(szBuf, "AuthType:") != 0) {
      sscanf(szBuf, "%*s %s", szVal);
      ft::strdup(&szType, szVal);
    } else if (strstr(szBuf, "Realm:") != 0) {
      sscanf(szBuf, "%*s %s", szVal);
      ft::strdup(&szRealm, szVal);
    } else if (strstr(szBuf, "AuthUserFile:") != 0) {
      sscanf(szBuf, "%*s %s", szVal);
      ft::strdup(&szUserFile, szVal);
    }
  }
  ifIn.close();

  if (ft::stricmp(szType, "basic") == 0) // Check basic authentication.
  {
    if (hInfo->szAuth == NULL) // Ask for credentials
    {
      ft::strdup(&hInfo->szRealm, szRealm); // Return this realm.
      iRc            = ACCESS_DENIED;
    } else // Check their credentials
    {
      iRc = BasicCheck(szUserFile, hInfo);
    }
  } else // Refuse request, Error.
  {
    iRc = ACCESS_FAILED;
  }

  delete[] szBuf;
  delete[] szVal;
  if (szType != NULL)
    delete[] szType;
  if (szRealm != NULL)
    delete[] szRealm;
  if (szUserFile != NULL)
    delete[] szUserFile;

  return (iRc);
}

// ------------------------------------------------------------------
//
// BasicCheck()
//
// This function checks the password file for a match
// with the credentials sent by the client.
//

int BasicCheck(char* szFile, Headers* hInfo) {
  char *szUserPass, *szBuf, *szClientUser, *szClientPass, *szUser, *szPass,
      *szCode;
  std::ifstream ifPass;
  bool          bFound = false;

  szCode = new char[strlen(hInfo->szAuth)];
  sscanf(hInfo->szAuth, "%*s %s", szCode);

  szUserPass = FromB64(szCode); // Decode the credentials.
  if (szUserPass == NULL) {
    return (ACCESS_FAILED); // Failure to decode credentials.
  }

  ifPass.open(szFile);
  if (! ifPass) {
    return (ACCESS_FAILED); // Something bad happened, deny access.
  }

  szBuf = new char[SMALLBUF];

  szClientUser = szUserPass;
  // Find the password part.
  szClientPass = (char*)strchr((const char*)szUserPass, ':');
  if (szClientPass == NULL) {
    ifPass.close();
    return (ACCESS_FAILED);
  }
  *szClientPass = '\0';
  szClientPass++;

  // Compare usernames/passwords one by one.
  while ((! ifPass.eof()) && (bFound == false)) {
    ifPass.getline(szBuf, SMALLBUF, '\n');
    szUser = szBuf;
    szPass = (char*)strchr((const char*)szBuf, ':');
    if (szPass == NULL)
      continue;
    *szPass = '\0';
    szPass++;
    if ((ft::stricmp(szClientUser, szUser) == 0) &&
        (strcmp(szClientPass, szPass) == 0))
    {
      ft::strdup(&hInfo->szAuthType, "basic");
      ft::strdup(&hInfo->szRemoteUser, szUser);
      bFound              = true;
    }
    memset(szBuf, 0, SMALLBUF);
  }
  ifPass.close();
  delete[] szBuf;
  delete[] szUserPass;

  if (bFound == true) {
    return (ACCESS_OK);
  }

  return (ACCESS_FAILED);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
