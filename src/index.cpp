//
// WWW Server  File: index.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// This file contains the source code to the basic framework
// for the WWW server.
//

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>


#ifdef __IBMCPP__
#include <builtin.h>
#endif

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
#define Sleep(x) DosSleep(x) // Portability definition
#elif __WINDOWS__
#include <windows.h>
#endif

#include "3wd.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "scodes.hpp"
#include "socket.hpp"


// ------------------------------------------------------------------
//
// Index
//
// This function performs a search function indicated by the
// ISINDEX tag for a document.
//

int Index(char* szDoc, char* szSearch, char* szFile, char* szLink) {
  char *        szBuf, *szTerms[5];
  std::ifstream ifDoc;
  std::ofstream ofOut;
  int           i, iCounts[5];

  ifDoc.open(szDoc);
  if (! ifDoc) // Failure to find document.
  {
    return 1;
  }

  szBuf = szSearch;
  while (*szBuf != NULL) // Replace plus signs with spaces.
  {
    if (*szBuf == '+')
      *szBuf = ' ';
    szBuf++;
  }
  DeHexify(szSearch);

  // Convert to lowercase for search.
  ft::strlwr(szSearch);
  // Accept up to 5 search terms.
  szTerms[0] = strtok(szSearch, " ");
  szTerms[1] = strtok(NULL, " ");
  szTerms[2] = strtok(NULL, " ");
  szTerms[3] = strtok(NULL, " ");
  szTerms[4] = strtok(NULL, " ");

  // Set the count to zero for each.
  iCounts[0] = iCounts[1] = iCounts[2] = iCounts[3] = iCounts[4] = 0;

  // Search the document for the terms.
  szBuf = new char[SMALLBUF];
  while (! ifDoc.eof()) {
    ifDoc.getline(szBuf, SMALLBUF, '\n');
    ft::strlwr(szBuf);
    for (i = 0; i < 5; i++) {
      if (szTerms[i]) // If not NULL.
      {
        if (strstr(szBuf, szTerms[i])) // If found.
        {
          iCounts[i]++; // Count it.
        }
      }
    }
  }
  ifDoc.close();
  delete[] szBuf; // Delete memory not needed.

  // Now create a document with the counts in it.
  tmpnam(szFile);
  ofOut.open(szFile);
  if (! ofOut) // Could not produce a temporary file.
  {
    return 1;
  }

  // Write some html to present the search results.
  ofOut << "<!doctype html public \"-//IETF//DTD HTML 2.0//EN\">" << std::endl;
  ofOut << "<html><head><title>Search Results</title></head><body>"
        << std::endl;
  ofOut << "<h1>Your Search Results</h1><hr>" << std::endl;
  ofOut << "The search on the document <b>" << szLink << "</b> "
        << "yielded the following results:" << std::endl;
  ofOut << "<ul>" << std::endl;
  for (i = 0; i < 5; i++) {
    if (szTerms[i] != NULL)
      ofOut << "<li><b>" << szTerms[i] << "</b> appeared " << iCounts[i]
            << " times." << std::endl;
  }
  ofOut << "</ul><p>" << std::endl;
  ofOut << "Pick this <a href=\"" << szLink << "\">Link</a> to view it."
        << std::endl;
  ofOut << "</body></html>" << std::endl;
  ofOut.close();

  return 0; // Success code.
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
