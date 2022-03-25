#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.hpp"

extern char *szServerRoot, // The root directory for serving files.
    *szHostName,    // The hostname given out. Overrides the call to hostname().
    *szWelcome,     // The default file to serve.
    *szAccessLog,   // The access log filename.
    *szErrorLog,    // The error log filename.
    *szReadAccess,  // The read access file name.
    *szWriteAccess, // The write access file name.
    *szDeleteDir;   // The directory to store deleted resources.
extern short sPort; // The port number to serve.
extern bool  bDnsLookup,    // Flag whether to do dns reverse lookups.
    bGmtTime;               // Flag whether to use GMT in access log file.
extern int iNumPathAliases, // The number of path aliases.
    iNumExecAliases,        // The number of exec aliases.
    iNumTypes;              // The number of extension types.
extern long   lGmtOffset;   // The offset in minutes between local time and GMT.
extern Paths *pAliasPath,   // The set of root aliases.
    *pAliasExec;            // The set of exec aliases.
extern Extensions* eExtMap; // The set of extensions and types.

const char szMonth[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
  "Aug", "Sep", "Oct", "Nov", "Dec" };
const char szDay[7][4]    = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

#endif
