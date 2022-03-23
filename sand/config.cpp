#include "config.hpp"

int ReadConfig(char *szConfigName) {
  ifstream ifIn;  // The config file handle.
  char *szBuf, *szDirective, *szVal1, *szVal2;
  int iNum1 = 0, iNum2 = 0, iNum3 = 1, i;

  ifIn.open(szConfigName);
  if (!ifIn) {
    cerr << "Error!" << endl;
    cerr << "Could not read configuration file: " << szConfigName << endl;
    return 1;
  }

  SetDefaults();

  szBuf = new char[SMALLBUF];
  szDirective = new char[SMALLBUF];
  szVal1 = new char[SMALLBUF];
  szVal2 = new char[SMALLBUF];

  pAliasPath = new Paths[MAX_ALIASES];
  pAliasExec = new Paths[MAX_ALIASES];
  eExtMap = new Extensions[MAX_EXTENSIONS];
  eExtMap[0].szExt = new char[1];
  eExtMap[0].szExt[0] = '\0';
  eExtMap[0].szType = strdup("application/octet-stream");

  while (!ifIn.eof())  // Until the end of the file.
  {
    memset(szBuf, 0, SMALLBUF);
    memset(szDirective, 0, SMALLBUF);
    ifIn.getline(szBuf, SMALLBUF, '\n');

    if ((szBuf[0] == '#') || (szBuf[0] == '\0')) continue;  // Skip comments.

    sscanf(szBuf, "%s %s %s", szDirective, szVal1, szVal2);  // Parse the line.

    if (ft::stricmp(szDirective, "ServerRoot") == 0) {
      if (szServerRoot) delete[] szServerRoot;
      Convert(szVal1);
      if (szVal1[strlen(szVal1) - 1] != '/') {
        szServerRoot = new char[strlen(szVal1) + 2];
        sprintf(szServerRoot, "%s/", szVal1);
      } else {
        szServerRoot = strdup(szVal1);
      }
    } else if (ft::stricmp(szDirective, "HostName") == 0) {
      if (szHostName) delete[] szHostName;
      szHostName = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "GMTOffset") == 0) {
      lGmtOffset = 60 * (atol(szVal1) / 100);  // Number of hours in minutes
      lGmtOffset += (atol(szVal1) % 100);      // Number of minutes specified
      lGmtOffset *= 60;                        // Convert minutes to seconds
    } else if (ft::stricmp(szDirective, "Welcome") == 0) {
      if (szWelcome) delete[] szWelcome;
      szWelcome = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "AccessLog") == 0) {
      if (szAccessLog) delete[] szAccessLog;
      szAccessLog = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "ErrorLog") == 0) {
      if (szErrorLog) delete[] szErrorLog;
      szErrorLog = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "DeleteDir") == 0) {
      if (szDeleteDir) delete[] szDeleteDir;
      Convert(szVal1);
      if (szVal1[strlen(szVal1) - 1] != '/') {
        szDeleteDir = new char[strlen(szVal1) + 2];
        sprintf(szDeleteDir, "%s/", szVal1);
      } else {
        szDeleteDir = strdup(szVal1);
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
      if (szReadAccess) delete[] szReadAccess;
      szReadAccess = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "WriteAccessName") == 0) {
      if (szWriteAccess) delete[] szWriteAccess;
      szWriteAccess = strdup(szVal1);
    } else if (ft::stricmp(szDirective, "PathAlias") == 0) {
      if (iNum1 == MAX_ALIASES) {
        cerr << "Exceeded maximum path aliases. " << szVal1 << " ignored."
             << endl;
        continue;
      }
      pAliasPath[iNum1].szAlias = strdup(szVal1);
      pAliasPath[iNum1].szTrue = new char[strlen(szVal2) + 2];
      strcpy(pAliasPath[iNum1].szTrue, szVal2);
      Convert(pAliasPath[iNum1].szAlias);
      Convert(pAliasPath[iNum1].szTrue);

      i = strlen(pAliasPath[iNum1].szTrue);
      if (pAliasPath[iNum1].szTrue[i - 1] != '/') {
        pAliasPath[iNum1].szTrue[i] = '/';
        pAliasPath[iNum1].szTrue[i + 1] = '\0';
      }

      iNum1++;
    } else if (ft::stricmp(szDirective, "ExecAlias") == 0) {
      if (iNum2 == MAX_ALIASES) {
        cerr << "Exceeded maximum exec aliases. " << szVal1 << " ignored."
             << endl;
        continue;
      }
      pAliasExec[iNum2].szAlias = strdup(szVal1);
      pAliasExec[iNum2].szTrue = new char[strlen(szVal2) + 2];
      strcpy(pAliasExec[iNum2].szTrue, szVal2);
      Convert(pAliasExec[iNum2].szAlias);
      Convert(pAliasExec[iNum2].szTrue);

      i = strlen(pAliasExec[iNum2].szTrue);
      if (pAliasExec[iNum2].szTrue[i - 1] != '/') {
        pAliasExec[iNum2].szTrue[i] = '/';
        pAliasExec[iNum2].szTrue[i + 1] = '\0';
      }

      iNum2++;
    } else if (ft::stricmp(szDirective, "ExtType") == 0) {
      if (iNum3 == MAX_EXTENSIONS) {
        cerr << "Exceeded maximum extensions. " << szVal1 << " ignored."
             << endl;
        continue;
      }
      eExtMap[iNum3].szExt = strdup(szVal1);
      eExtMap[iNum3].szType = strdup(szVal2);

      iNum3++;
    }
  }

  ifIn.close();

  iNumPathAliases = iNum1;
  iNumExecAliases = iNum2;
  iNumTypes = iNum3;

  delete[] szBuf;
  delete[] szDirective;
  delete[] szVal1;
  delete[] szVal2;

  for (i = 0; i < iNumTypes; i++) {
    if (strstr(eExtMap[i].szType, "text/") == NULL) {
      eExtMap[i].bBinary = true;
    }
  }

  return 0;
}

void SetDefaults() {
  sPort = WWW_PORT;
  bDnsLookup = true;
  bGmtTime = false;
  szReadAccess = strdup(HTREADACCESS);
  szWriteAccess = strdup(HTWRITEACCESS);
}
