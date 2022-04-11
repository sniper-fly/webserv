//
// WWW Server  File: http11.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <ctype.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "3wd.hpp"
#include "cgi.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "headers.hpp"
#include "http10.hpp"
#include "http11.hpp"
#include "scodes.hpp"
#include "socket.hpp"
#include "util.hpp"
#include "ftutil.hpp"


// ------------------------------------------------------------------

// The alphabet used for MIME boundaries.
const char szMime[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST"
                      "UVWXYZ'()=_,-./:=?";
// The number of characters in the alphabet.
const int iNumMime = strlen(szMime);

// ------------------------------------------------------------------
//
// DoHttp11()
//
// This function handles our HTTP/1.0 requests.
//

int DoHttp11(Socket* sClient, char* szMethod, char* szUri) {
  int      iRc, iRsp, iMethod;
  char *   szReq, *szPath, *szCgi, *szTmp, *szSearch;
  Headers* hInfo;
  bool     bCgi = false, bPersistent;

  szReq    = strdup(sClient->szOutBuf); // Save the request line.
  iRsp     = 200;
  szSearch = NULL;
  szPath   = NULL;
  szCgi    = NULL;
  hInfo    = new Headers();
  iMethod  = CheckMethod(szMethod); // The request method.

  // First, check for TRACE method.
  if (iMethod == TRACE) {
    // Do a trace, saving connection.
    bPersistent = DoTrace(sClient, hInfo);
    DeHexify(szReq);
    WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
    delete[] szReq;
    delete hInfo;
    return bPersistent;
  }

  hInfo->RcvHeaders(sClient);          // Grab the request headers.
  bPersistent = hInfo->bPersistent;    // Find out if close was requested.
  iRc         = hInfo->CheckHeaders(); // Make sure none are inconsistent.
  if (iRc == false)                    // Bad request.
  {
    iRsp = SendError(sClient,
        (char*)"Missing Host header or incompatible headers detected.", 400,
        (char*)HTTP_1_1, hInfo);
    DeHexify(szReq);
    WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
    delete[] szReq;
    delete hInfo;
    return bPersistent;
  }

  // Check for a query in the URI.
  if ((szTmp = strchr(szUri, '?')) != NULL) {
    // Break up the URI into document and and search parameters.
    *szTmp = '\0'; // Append NULL to shorter URI.
    szTmp++;       // Let szTmp point to the query terms.
    szSearch       = strdup(szTmp);
    hInfo->szQuery = strdup(szSearch);
    if (strchr(szSearch, '=') != NULL) {
      bCgi = true; // Only a cgi request can contain an equal sign.
    }
  }

  DeHexify(szUri);                    // Remove any escape sequences.
  hInfo->szMethod = strdup(szMethod); // Save a few items.
  hInfo->szUri    = strdup(szUri);
  hInfo->szVer    = strdup(HTTP_1_1);
  szPath          = ResolvePath(szUri); // Check for path match.
  szCgi           = ResolveExec(szUri); // Check for exec match.

  // Now key on the request method and URI given.
  // Any POST request.
  if (iMethod == POST) {
    iRsp = DoExec11(sClient, iMethod, szCgi, szSearch, hInfo);
  }
  // A GET or HEAD to process as a CGI request.
  else if ((bCgi == true) && (iMethod == GET))
  {
    iRsp = DoExec11(sClient, iMethod, szCgi, szSearch, hInfo);
  }
  // Any valid DELETE request.
  else if (iMethod == DELETE)
  {
    iRsp = DoDelete(sClient, szPath, szCgi, hInfo);
  }
  // A simple GET or HEAD request.
  else if ((iMethod == GET) && (szPath != NULL))
  {
    iRsp = DoPath11(sClient, iMethod, szPath, szSearch, hInfo);
  }
  // Unknown method used.
  else if (iMethod == UNKNOWN)
  {
    iRsp = SendError(sClient, (char*)"Request method not implemented.", 501,
        (char*)HTTP_1_1, hInfo);
  }
  // Error Condition.
  else
  {
    iRsp = SendError(
        sClient, (char*)"Resource not found.", 404, (char*)HTTP_1_1, hInfo);
  }

  // This request now finished. Log the results.
  DeHexify(szReq);
  WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
  delete[] szReq;
  delete hInfo;
  if ((szSearch != NULL) && (bCgi == false)) {
    unlink(szPath); // The temporary search file.
    delete[] szSearch;
  }
  if (szPath)
    delete[] szPath;
  if (szCgi)
    delete[] szCgi;

  return bPersistent;
}

// ------------------------------------------------------------------
//
// DoPath11()
//
// This function checks to see if it can return the requested
// document back to the client.
//

int DoPath11(Socket* sClient, int iMethod, char* szPath, char* szSearch,
    Headers* hInfo) {
  struct stat   sBuf;
  char *        szTmp, szBuf[PATH_LENGTH], szFile[PATH_LENGTH];
  std::ofstream ofTmp;
  int iRsp = 200, iRc, iType, iIfMod, iIfUnmod, iIfMatch, iIfNone, iIfRange,
      iRangeErr;

  if (szPath[strlen(szPath) - 1] == '/') {
    strcat(szPath, szWelcome); // Append default welcome file.
  }

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS); // Check for authorization.
  if (iRc == ACCESS_DENIED)                    // Send request for credentials.
  {
    sClient->Send("HTTP/1.1 401 \r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
    sClient->Send(szBuf);
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz401));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz401);
    return 401;
  } else if (iRc == ACCESS_FAILED) // Send forbidden response.
  {
    sClient->Send("HTTP/1.1 403 Access Denied\r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz403));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz403);
    return 403;
  }

  if (szSearch != NULL) // Do an index search.
  {
    iRc = Index(szPath, szSearch, szFile, hInfo->szUri);
    if (iRc != 0) {
      iRc = SendError(
          sClient, (char*)"Resource not found.", 404, (char*)HTTP_1_1, hInfo);
      return iRc;
    }
    strcpy(szPath, szFile);
  }

  iRc = stat(szPath, &sBuf);
  if (iRc < 0) {
    iRsp = SendError(
        sClient, (char*)"Resource not found.", 404, (char*)HTTP_1_1, hInfo);
    return iRsp;
  }

  // Check If headers.
  iIfMod    = IfModSince(hInfo, sBuf.st_mtime);
  iIfUnmod  = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch  = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone   = IfNone(hInfo, sBuf.st_mtime);
  iIfRange  = IfRange(hInfo, sBuf.st_mtime);
  iRangeErr = hInfo->FindRanges(sBuf.st_size);

  // Check to make sure any If headers are false.
  // Either not-modified or no etags matched.
  if ((iIfMod == false) || (iIfNone == false)) {
    sClient->Send("HTTP/1.1 304 Not Modified\r\n");
    iRsp = 304;
  }
  // No matching etags or it's been modified.
  else if ((iIfMatch == false) || (iIfUnmod == false))
  {
    sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
    iRsp = 412;
  }
  // Resource matched so send just the bytes requested.
  else if ((iIfRange == true) && (iRangeErr == 0))
  {
    sClient->Send("HTTP/1.1 206 Partial Content\r\n");
    iRsp = 206;
  }
  // Resource didn't match, so send the entire entity.
  else if ((hInfo->szIfRange != NULL) && (iIfRange == false))
  {
    sClient->Send("HTTP/1.1 200 OK\r\n");
    iRsp = 200;
  }
  // Only asked for a byte range.
  else if (iRangeErr == 0)
  {
    sClient->Send("HTTP/1.1 206 Partial Content\r\n");
    iRsp = 206;
  }
  // Must be a plain jane request.
  else
  {
    sClient->Send("HTTP/1.1 200 OK\r\n");
    iRsp = 200;
  }

  sClient->Send("Server: "); // Standard server header.
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL)); // Create a date header.
  if (szTmp != NULL) {
    sClient->Send("Date: ");
    sClient->Send(szTmp);
    sClient->Send("\r\n");
    delete[] szTmp;
  }
  szTmp = CreateDate(sBuf.st_mtime); // The last modified time header.
  if (szTmp != NULL) {
    sClient->Send("Last-Modified: ");
    sClient->Send(szTmp);
    sClient->Send("\r\n");
    delete[] szTmp;
  }
  sprintf(szBuf, "ETag: \"%ld\"\r\n", sBuf.st_mtime); // Entity tag.
  sClient->Send(szBuf);

  if ((iRsp == 304) || (iRsp == 412)) {
    sClient->Send("\r\n");
    return iRsp; // Don't send anything else.
  }

  if (szSearch != NULL) // Force search results to text/html type.
  {
    iType = FindType((char*)"x.html");
  } else {
    iType = FindType(szPath); // Figure out the MIME type to return.
  }

  if (iRsp == 206) // Sending partial content.
  {
    // Send byte range to client.
    SendByteRange(sClient, hInfo, szPath, &sBuf, iType, iMethod);
    return iRsp;
  }

  // Send full entity.
  sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
  sClient->Send(szBuf);
  sprintf(szBuf, "Content-Length: %ld\r\n", sBuf.st_size);
  sClient->Send(szBuf);
  sClient->Send("\r\n");

  if (iMethod == GET) // Don't send unless GET.
  {
    if (eExtMap[iType].bBinary == true) {
      iRc = sClient->SendBinary(szPath);
    } else {
      iRc = sClient->SendText(szPath);
    }
  }

  return iRsp;
}

// ------------------------------------------------------------------
//
// DoExec11()
//
// This function executes our CGI scripts.
//

int DoExec11(Socket* sClient, int iMethod, char* szPath, char* szSearch,
    Headers* hInfo) {
  struct stat   sBuf;
  char *        szTmp, *szVal, *szPtr, szBuf[SMALLBUF], szFile[PATH_LENGTH];
  int           iRsp = 200, iRc, iIfUnmod, iIfMatch, iIfNone, i, iCount;
  Cgi*          cParms;
  std::ofstream ofOut;
  std::ifstream ifIn;

  (void)szSearch;

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS); // Check for authorization.
  if (iRc == ACCESS_DENIED)                    // Send request for credentials.
  {
    sClient->Send("HTTP/1.1 401 \r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
    sClient->Send(szBuf);
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz401));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz401);
    return 401;
  } else if (iRc == ACCESS_FAILED) // Send forbidden response.
  {
    sClient->Send("HTTP/1.1 403 Access Denied\r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz403));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz403);
    return 403;
  }

  iRc = stat(szPath, &sBuf);
  if (iRc < 0) {
    iRsp = SendError(
        sClient, (char*)"Resource not found.", 404, (char*)HTTP_1_1, hInfo);
    return iRsp;
  }

  // Check If headers.
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone  = IfNone(hInfo, sBuf.st_mtime);

  // Check to make sure any If headers are false.
  // No match on etags or it's been modified or an etag did match.
  if ((iIfMatch == false) || (iIfUnmod == false) || (iIfNone == false)) {
    sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
    iRsp = 412;
  }
  // Go ahead and do the CGI.
  else
  {
    sClient->Send("HTTP/1.1 200 OK\r\n");
    iRsp = 200;
  }

  sClient->Send("Server: "); // Standard server response.
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL)); // Create a date header.
  if (szTmp != NULL) {
    sClient->Send("Date: ");
    sClient->Send(szTmp);
    sClient->Send("\r\n");
    delete[] szTmp;
  }

  if (iRsp == 412) {
    hInfo->ulContentLength = 0; // For the logfile.
    return iRsp;                // Don't send anything else.
  }

  // Execute the cgi program here.
  cParms          = new Cgi();
  cParms->hInfo   = hInfo;
  cParms->sClient = sClient;
  cParms->szProg  = szPath;
  if (iMethod == POST) {
    // Grab the posted data.
    cParms->szOutput = NULL;
    tmpnam(szFile);
    ft::strlwr(hInfo->szContentType);
    szPtr = strstr(hInfo->szContentType, "text/");
    if (szPtr != NULL) // Receiving text data.
    {
      ofOut.open(szFile);
      iCount = 0;
      // Get the specified number of bytes.
      while ((unsigned long)iCount < hInfo->ulContentLength) {
        i = sClient->RecvTeol(); // Keep eol for proper byte count.
        iCount += i;
        // Remove the end of line.
        while ((sClient->szOutBuf[i] == '\r') || (sClient->szOutBuf[i] == '\n'))
        {
          sClient->szOutBuf[i] = '\0';
          i--;
        }
        ofOut << sClient->szOutBuf << std::endl; // Write to temp file.
      }
    } else // Binary data.
    {
      ofOut.open(szFile, std::ios::binary); // Open in binary mode.
      iCount = 0;
      while ((unsigned long)iCount < hInfo->ulContentLength) {
        i = sClient->Recv(hInfo->ulContentLength - iCount);
        iCount += i;
        ofOut.write(sClient->szOutBuf, i);
      }
    }
    ofOut.close();
    cParms->szPost = szFile;
  }

  ExecCgi(cParms); // Run the cgi program.

  stat(cParms->szOutput, &sBuf);
  ifIn.open(cParms->szOutput); // Open the output file.
  iCount = 0;
  ifIn.getline(szBuf, SMALLBUF, '\n');
  // Parse the cgi output for headers.
  while (szBuf[0] != '\0') {
    iCount += strlen(szBuf) + 2;
    szVal = strchr(szBuf, ':');
    if (szVal == NULL) {
      ifIn.getline(szBuf, SMALLBUF, '\n');
      continue;
    }
    *szVal = '\0';
    szVal++;
    ft::strlwr(szBuf);
    // Look for and allow proper response headers.
    if (strcmp(szBuf, "cache-control") == 0) {
      sClient->Send("Cache-Control: ");
    } else if (strcmp(szBuf, "content-type") == 0) {
      sClient->Send("Content-Type: ");
    } else if (strcmp(szBuf, "content-base") == 0) {
      sClient->Send("Content-Base: ");
    } else if (strcmp(szBuf, "content-encoding") == 0) {
      sClient->Send("Content-Encoding: ");
    } else if (strcmp(szBuf, "content-language") == 0) {
      sClient->Send("Content-Language: ");
    } else if (strcmp(szBuf, "content-location") == 0) {
      sClient->Send("Content-Location: ");
    } else if (strcmp(szBuf, "etag") == 0) {
      sClient->Send("Etag: ");
    } else if (strcmp(szBuf, "expires") == 0) {
      sClient->Send("Expires: ");
    } else if (strcmp(szBuf, "from") == 0) {
      sClient->Send("From: ");
    } else if (strcmp(szBuf, "location") == 0) {
      sClient->Send("Location: ");
    } else if (strcmp(szBuf, "last-modified") == 0) {
      sClient->Send("Last-Modified: ");
    } else if (strcmp(szBuf, "vary") == 0) {
      sClient->Send("Vary: ");
    } else // No match. Don't send this unknown header.
    {
      ifIn.getline(szBuf, SMALLBUF, '\n');
      continue;
    }
    sClient->Send(szVal); // Send the parameter for the header line.
    sClient->Send("\r\n");
    ifIn.getline(szBuf, SMALLBUF, '\n'); // コンパイラがvcc
  }
  ifIn.close();
  iCount += 2; // The last CRLF isn't counted within the loop.
  sprintf(szBuf, "Content-Length: %ld\r\n\r\n", sBuf.st_size - iCount);
  sClient->Send(szBuf);

  if (iMethod != HEAD) // Only send the entity if not HEAD.
  {
    hInfo->ulContentLength = sBuf.st_size - iCount;
    ifIn.open(cParms->szOutput, std::ios::binary);
    ifIn.seekg(iCount, std::ios::beg);
    while (! ifIn.eof()) {
      ifIn.read(szBuf, SMALLBUF);
      i = ifIn.gcount();
      sClient->Send(szBuf, i);
    }
    ifIn.close();
  } else {
    hInfo->ulContentLength = 0;
  }

  // Remove the temporary files and memory.
  unlink(cParms->szOutput);
  delete[](cParms->szOutput);
  if (cParms->szPost != NULL)
    unlink(cParms->szPost);
  delete cParms;

  return iRsp;
}

// ------------------------------------------------------------------
//
// CheckMethod
//
// Determine which method the client is sending. Remember
// that methods *ARE* case-sensitive, unlike most of HTTP/1.1.
//

int CheckMethod(char* szMethod) {
  if (strcmp(szMethod, "GET") == 0) {
    return GET;
  } else if (strcmp(szMethod, "POST") == 0) {
    return POST;
  } else if (strcmp(szMethod, "HEAD") == 0) {
    return HEAD;
  } else if (strcmp(szMethod, "OPTIONS") == 0) {
    return OPTIONS;
  } else if (strcmp(szMethod, "PUT") == 0) {
    return PUT;
  } else if (strcmp(szMethod, "DELETE") == 0) {
    return DELETE;
  } else if (strcmp(szMethod, "TRACE") == 0) {
    return TRACE;
  }
  return UNKNOWN;
}

// --------------------------------------------------------
//
// MakeUnique()
//
// Create a unique filename in the specified directory with the
// specified extension.
//

char* MakeUnique(char* szDir, char* szExt) {
  unsigned long ulNum      = 0;
  bool          bNotUnique = true;
  int           iRc;
  char*         szFileName;

  szFileName = new char[PATH_LENGTH];

  while (bNotUnique) {
    sprintf(szFileName, "%s%08lu.%s", szDir, ulNum, szExt);
    iRc = open(szFileName, O_CREAT | O_EXCL | O_WRONLY, S_IWRITE); // O_TEXT除外
    if (iRc != -1) {
      // Success. This file didn't exist before.
      close(iRc);
      bNotUnique = false;
      continue;
    }

    ulNum++;
    if (ulNum > 99999999) {
      delete[] szFileName;
      szFileName = NULL;
      bNotUnique = false;
    }
  }
  return (szFileName);
}

// ------------------------------------------------------------------
//
// DoDelete
//
// This function checks to see if it can delete the resource
// specified by the client.
//

int DoDelete(Socket* sClient, char* szPath, char* szCgi, Headers* hInfo) {
  struct stat   sBuf;
  char *        szTmp, *szExt, szBuf[PATH_LENGTH];
  std::ofstream ofTmp;
  int           iRsp = 200, iRc, iIfUnmod, iIfMatch, iIfNone;

  (void)szCgi;

  iRc = CheckAuth(szPath, hInfo, WRITE_ACCESS); // Check for authorization.
  if (iRc == ACCESS_DENIED)                     // Send request for credentials.
  {
    sClient->Send("HTTP/1.1 401 \r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
    sClient->Send(szBuf);
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz401));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz401);
    return 401;
  } else if (iRc == ACCESS_FAILED) // Send forbidden response.
  {
    sClient->Send("HTTP/1.1 403 Access Denied\r\n");
    sClient->Send("Server: ");
    sClient->Send(szServerVer);
    sClient->Send("\r\n");
    szTmp = CreateDate(time(NULL)); // Create a date header.
    if (szTmp != NULL) {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete[] szTmp;
    }
    sClient->Send("Content-Type: text/html\r\n");
    sprintf(szBuf, "Content-Length: %lu\r\n", strlen(sz403));
    sClient->Send(szBuf);
    sClient->Send("\r\n");
    sClient->Send(sz403);
    return 403;
  }

  if (hInfo->szRange != NULL) // Range not allowed for DELETE.
  {
    SendError(sClient, (char*)"Range header not accepted for DELETE.", 501,
        (char*)HTTP_1_1, hInfo);
    return 501;
  }
  if (hInfo->szIfModSince != NULL) // If-Modified-Since
  {                                // not allowed for DELETE.
    SendError(sClient,
        (char*)"If-Modified-Since header not accepted for DELETE.", 501,
        (char*)HTTP_1_1, hInfo);
    return 501;
  }

  // Now check the If headers.
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone  = IfNone(hInfo, sBuf.st_mtime);
  if ((iIfUnmod == false) || (iIfMatch == false) || (iIfNone == false)) {
    SendError(
        sClient, (char*)"Precondition failed.", 412, (char*)HTTP_1_1, hInfo);
    return 412;
  }

  if (szDeleteDir != NULL) // Save the deleted resource.
  {
    // Use the same file extension as the current resource.
    szExt = strrchr(szPath, '.');
    if (szExt != NULL) {
      szExt++;
    } else {
      szExt = (char*)"del";
    }
    szTmp = MakeUnique(szDeleteDir, szExt);
    ft::copyFile(szPath, szTmp);
    delete[] szTmp;
  }
  iRc = unlink(szPath);
  if (iRc == 0) // Resource deleted.
  {
    sClient->Send("HTTP/1.1 204 No Content\r\n");
    iRsp = 204;
  } else // Delete failed.
  {
    sClient->Send("HTTP/1.1 500 Server Error\r\n");
    iRsp = 500;
  }
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL)); // Create a date header.
  if (szTmp != NULL) {
    sClient->Send("Date: ");
    sClient->Send(szTmp);
    sClient->Send("\r\n");
    delete[] szTmp;
  }
  sClient->Send("\r\n");
  hInfo->ulContentLength = 0;
  return iRsp;
}

// ------------------------------------------------------------------
//
// IfModSince
//
// Check whether the file had been modifed since the date
// given by the client.
//

int IfModSince(Headers* hInfo, time_t ttMtime) {
  if (hInfo->szIfModSince != NULL) {
    if ((hInfo->ttIfModSince > 0) && (hInfo->ttIfModSince < ttMtime)) {
      return true;
    } else {
      return false;
    }
  }
  return true; // Default is true.
}

// ------------------------------------------------------------------
//
// IfUnmodSince
//
// Check whether the file has not been modified since the date
// given by the client.
//

int IfUnmodSince(Headers* hInfo, time_t ttMtime) {
  if (hInfo->szIfUnmodSince != NULL) {
    if ((hInfo->ttIfUnmodSince > 0) && (hInfo->ttIfUnmodSince > ttMtime)) {
      return true;
    } else {
      return false;
    }
  }
  return true; // Default is true.
}

// ------------------------------------------------------------------
//
// IfMatch
//
// Check the etag of the resource against that given by the client
// for a match.
//

int IfMatch(Headers* hInfo, time_t ttMtime) {
  int   iIfMatch = true, i;
  char *szBuf, szEtagStar[] = "*";

  // Check to see if any etags match.
  if (hInfo->szIfMatch != NULL) {
    iIfMatch = false; // We fail unless we match.
    szBuf    = new char[SMALLBUF];
    sprintf(szBuf, "\"%ld\"", ttMtime);
    for (i = 0; hInfo->szIfMatchEtags[i] != NULL; i++) {
      if (strcmp(hInfo->szIfMatchEtags[i], szBuf) == 0) {
        iIfMatch = true;
        break;
      }
      if (strcmp(hInfo->szIfMatchEtags[i], szEtagStar) == 0) {
        iIfMatch = true;
        break;
      }
    }
    delete[] szBuf;
  }
  return iIfMatch;
}

// ------------------------------------------------------------------
//
// IfNone
//
// Check to make sure no etags match the resource.
//

int IfNone(Headers* hInfo, time_t ttMtime) {
  int   iIfNone = true, i;
  char *szBuf, szEtagStar[] = "*";

  // Check to see if any of the If-None-Match etags match
  if (hInfo->szIfNoneMatch != NULL) {
    iIfNone = true; // We're ok unless we match.
    szBuf   = new char[SMALLBUF];
    sprintf(szBuf, "\"%ld\"", ttMtime);
    for (i = 0; hInfo->szIfNoneMatchEtags[i] != NULL; i++) {
      if (strcmp(hInfo->szIfNoneMatchEtags[i], szBuf) == 0) {
        iIfNone = false;
        break;
      }
      if (strcmp(hInfo->szIfNoneMatchEtags[i], szEtagStar) == 0) {
        iIfNone = false;
        break;
      }
    }
    delete[] szBuf;
  }
  return iIfNone;
}

// ------------------------------------------------------------------
//
// IfRange
//
// Find out whether the If-Range tag matches the resource.
//

int IfRange(Headers* hInfo, time_t ttMtime) {
  char*  szBuf;
  time_t ttDate;

  // Check the If-Range header. We must have Range also to be valid.
  if ((hInfo->szIfRange != NULL) && (hInfo->szRange != NULL)) {
    // Figure out whether it is an etag or date.
    if ((hInfo->szIfRange[0] == '"') || (hInfo->szIfRange[2] == '"')) {
      szBuf = new char[SMALLBUF]; // An etag.
      sprintf(szBuf, "\"%ld\"", ttMtime);
      if (strcmp(szBuf, hInfo->szIfRange) == 0) {
        delete[] szBuf;
        return true; // Match, send them the resource.
      }
      delete[] szBuf;
    } else {
      ttDate = ConvertDate(hInfo->szIfRange); // We found a date.
      if (ttDate >= ttMtime) {
        return true; // Match, send them the resource.
      }
    }
  }

  return false; // No match.
}

// ------------------------------------------------------------------
//
// SendByteRange
//
// Send the given byte ranges back to the client.
//

int SendByteRange(Socket* sClient, Headers* hInfo, char* szPath,
    struct stat* sBuf, int iType, int iMethod) {
  std::ifstream ifIn;
  int           iBytes, iCount, iLen, i, j;
  char *        szBuf, *szBoundary;

  szBuf = new char[SMALLBUF];

  if (hInfo->iRangeNum == 1) // Simple response, only one part.
  {
    iLen = hInfo->rRanges[0].iEnd - hInfo->rRanges[0].iStart + 1;
    sprintf(szBuf, "Content-Length: %d\r\n", iLen);
    sClient->Send(szBuf);
    sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
    sClient->Send(szBuf);
    sClient->Send("\r\n");

    if (iMethod == HEAD) // Don't send an entity.
    {
      delete[] szBuf;
      hInfo->ulContentLength = 0;
      return 0;
    }

    ifIn.open(szPath, std::ios::binary); // Open the file, binary mode.
    ifIn.seekg(hInfo->rRanges[0].iStart, std::ios::beg);
    iCount = 0;
    while (iCount < iLen) {
      ifIn.read(szBuf, (SMALLBUF < iLen - iCount ? SMALLBUF : iLen - iCount));
      iBytes = ifIn.gcount();
      iCount += iBytes;
      sClient->Send(szBuf, iBytes);
    }
    ifIn.close();
  } else // Do a multi-part MIME type.
  {
    szBoundary = new char[70];
    srand(sBuf->st_mtime);
    for (i = 0; i < 68; i++) {
      j             = rand();
      szBoundary[i] = szMime[j % iNumMime];
    }
    szBoundary[69] = '\0';

    sprintf(szBuf, "Content-Type: multipart/byteranges; boundary=\"%s\"\r\n",
        szBoundary);
    sClient->Send(szBuf);

    if (iMethod == HEAD) // Don't send an entity.
    {
      delete[] szBuf;
      hInfo->ulContentLength = 0;
      return 0;
    }

    ifIn.open(szPath, std::ios::binary); // Open the file, binary mode.

    for (i = 0; i < hInfo->iRangeNum; i++) {
      sClient->Send("\r\n--"); // The boundary marker first.
      sClient->Send(szBoundary);
      sClient->Send("\r\n");
      sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
      sClient->Send(szBuf); // Now content-type.
      sprintf(szBuf, "Content-Range: bytes %d-%d/%ld\r\n\r\n",
          hInfo->rRanges[i].iStart, hInfo->rRanges[i].iEnd, sBuf->st_size);
      sClient->Send(szBuf); // Now content-range.

      ifIn.seekg(hInfo->rRanges[i].iStart, std::ios::beg);
      iLen   = hInfo->rRanges[i].iEnd - hInfo->rRanges[i].iStart + 1;
      iCount = 0;
      // Read the specified number of bytes.
      while (iCount < iLen) {
        ifIn.read(szBuf, (SMALLBUF < iLen - iCount ? SMALLBUF : iLen - iCount));
        iBytes = ifIn.gcount();
        iCount += iBytes;
        sClient->Send(szBuf, iBytes);
      }
    }
    sClient->Send("\r\n--"); // The ending boundary marker.
    sClient->Send(szBoundary);
    sClient->Send("--\r\n");
    delete[] szBoundary;
    ifIn.close();
  }

  delete[] szBuf;
  return 0;
}

// ------------------------------------------------------------------
//
// GetChunked
//
// Receive the entity using the chunked method.
//

int GetChunked(Socket* sClient, std::ofstream& ofOut, Headers* hInfo) {
  bool  bNotDone = true;
  char* szPtr;
  int   iBytes, i, j, l, iFactor;

  while (bNotDone == true) {
    sClient->RecvTeol(NO_EOL); // Grab a line. Should have chunk size.
    if (strcmp(sClient->szOutBuf, "0") == 0) {
      bNotDone = false; // The end of the chunks.
      continue;
    }

    szPtr = strchr(sClient->szOutBuf, ';');
    if (szPtr != NULL)
      *szPtr = '\0'; // Mark end of chunk-size.

    l = strlen(sClient->szOutBuf); // Find last hex digit.
    l--;
    iBytes  = 0;
    iFactor = 1;
    // Convert to decimal bytes.
    while (l >= 0) {
      iBytes += iFactor * Hex2Dec(sClient->szOutBuf[l]);
      l--;
      iFactor *= 16;
    }
    i = 0;
    // Now receive the specified number of bytes.
    while (i < iBytes) {
      j = sClient->Recv(iBytes - i);     // Some data.
      i += j;                            // Total the bytes.
      ofOut.write(sClient->szOutBuf, j); // Save to disk.
    }
    sClient->RecvTeol(NO_EOL); // Discard end of chunk marker.
  }

  // Now consume anything in the footer.
  hInfo->RcvHeaders(sClient);
  return 0;
}

// ------------------------------------------------------------------
//
// Hex2Dec
//
// Convert a hex character to a decimal character.
//

int Hex2Dec(char c) {
  switch (c) {
    case 'A':
    case 'a':
      return 10;
    case 'B':
    case 'b':
      return 11;
    case 'C':
    case 'c':
      return 12;
    case 'D':
    case 'd':
      return 13;
    case 'E':
    case 'e':
      return 14;
    case 'F':
    case 'f':
      return 15;
    default:
      return (c - 48);
  }
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
