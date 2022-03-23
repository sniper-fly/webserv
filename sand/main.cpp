#include "config.hpp"
#include "ftutil.hpp"
#include "socket.hpp"
#include "util.hpp"
#include "signal.hpp"
#include "w3conn.hpp"

#define PORT      8080
#define STACKSIZE 50000

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

// P193
void Server() {
  Socket    server, *client;
  pthread_t thread;
  int       iRc;

  // socket
  server.Create();
  // bind listen
  server.Passive(PORT);
  while (1) {
    // accept
    client = server.Accept();
    if (client == NULL)
      continue;

    iRc = pthread_create(&thread, NULL, &W3Conn, client);
    if (iRc == -1) {
      client->Close();
      delete client;
    }
  }
}

int main(void) {
  int iRc;

  // config
  iRc = ReadConfig("3wd.cf");
  if (iRc) {
    std::cerr << "Error!" << std::endl;
    std::cerr << "Error reading configuration file. Exiting." << std::endl;
    return 1; // Exit on error.
  }
  // cgi
  // signal
  set_signal_handler(SIGINT);

  Server();
  int N;
  int a;
  // while
  // accept
  // parse request
  // set response
  // send
  // close
}
