#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "defines.hpp"
#include "ftutil.hpp"
#include "socket.hpp"
#include "util.hpp"

#define Convert(str)                \
  for (i = 0; i < strlen(str); i++) \
    if (str[i] == '\\') str[i] = '/'

// The Paths class is used to hold path aliases and executable
// aliases information.
class Paths {
 public:
  char *szAlias;
  char *szTrue;

  Paths() {
    szAlias = NULL;
    szTrue = NULL;
  };
  ~Paths() {
    if (szAlias != NULL) delete[] szAlias;
    if (szTrue != NULL) delete[] szTrue;
  };
};

// The Extensions class holds the information for MIME types
// by extension.
class Extensions {
 public:
  char *szExt;
  char *szType;
  bool bBinary;

  Extensions() {
    szExt = NULL;
    szType = NULL;
    bBinary = false;
  };
  ~Extensions() {
    if (szExt != NULL) delete[] szExt;
    if (szType != NULL) delete[] szType;
  };
};

const char szServerVer[] = "3wd/1.1";  // The server name and version.

#define HTREADACCESS "3wdread"    // Default read access filename.
#define HTWRITEACCESS "3wdwrite"  // Default write access filename.

extern char *szServerRoot,  // The root directory for serving files.
    *szHostName,    // The hostname given out. Overrides the call to hostname().
    *szWelcome,     // The default file to serve.
    *szAccessLog,   // The access log filename.
    *szErrorLog,    // The error log filename.
    *szReadAccess,  // The read access file name.
    *szWriteAccess,          // The write access file name.
    *szDeleteDir;            // The directory to store deleted resources.
extern short sPort;          // The port number to serve.
extern bool bDnsLookup,      // Flag whether to do dns reverse lookups.
    bGmtTime;                // Flag whether to use GMT in access log file.
extern int iNumPathAliases,  // The number of path aliases.
    iNumExecAliases,         // The number of exec aliases.
    iNumTypes;               // The number of extension types.
extern long lGmtOffset;    // The offset in minutes between local time and GMT.
extern Paths *pAliasPath,  // The set of root aliases.
    *pAliasExec;           // The set of exec aliases.
extern Extensions *eExtMap;  // The set of extensions and types.

int ReadConfig(char *szConfigName);
void SetDefaults();

#endif