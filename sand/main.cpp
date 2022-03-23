#include "config.hpp"
#include "ftutil.hpp"
#include "socket.hpp"

#define PORT      8080
#define STACKSIZE 50000

template<typename T>
void puterr(T t) {
  std::cerr << "[" << t << "]" << std::endl;
}

template<typename T, typename S>
void puterr(T t, S s) {
  std::cerr << "[" << t << "]"
            << " " << s << std::endl;
}

template<typename T, typename S, typename U>
void puterr(T t, S s, U u) {
  std::cerr << "[" << t << "]"
            << " " << s << " " << u << std::endl;
}

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

/* proto type function */
static void set_signal_handler(int signame);
static void signal_handler(int signame);

static void set_signal_handler(int signame) {
  if (SIG_ERR == signal(signame, signal_handler)) {
    fprintf(stderr, "Error. Cannot set signal handler.\n");
    exit(1);
  }
}

static void signal_handler(int signame) {
  printf("Signal(%d) : simple-server stopped.\n", signame);
  exit(0);
}

// dummy
int DoHttp11(Socket* sClient, char* szMethod, char* szUri) { return 0; }

// void _Optlink W3Conn(void *client)
void* W3Conn(void* client) {
  printf("simple-server started.\n");

  Socket* sClient;
  char *  szRequest, *szUri, *szVer;
  int     iRc;

  sClient = (Socket*)client; // Get the pointer to the socket

  // Resolve the IP Name if requested.
  if (bDnsLookup == true) {
    sClient->ResolveName();
  }

  szRequest = new char[SMALLBUF];
  szUri     = new char[SMALLBUF];
  szVer     = new char[SMALLBUF];

  iRc = sClient->RecvTeol(NO_EOL); // Get the message

  //  sClient->debug();

  // Parse the components of the request
  // "GET / HTTP/1.1"
  sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);

  puterr("szRequest", szRequest);
  puterr("szUri", szUri);
  puterr("szVer", szVer);

  if (ft::stricmp(szVer, "http/1.0") == 0) {
    //      DoHttp10(sClient, szRequest, szUri);
  } else if (ft::stricmp(szVer, "http/1.1") == 0) {
    iRc = DoHttp11(sClient, szRequest, szUri);
    while (iRc == true) // Do persistent connections.
    {
      sClient->RecvTeol(NO_EOL);
      sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);
      iRc = DoHttp11(sClient, szRequest, szUri);
    }
  } else // Treat this request as a HTTP/0.9 request.
  {
    //      DoHttp09(sClient, szRequest, szUri);
  }
  delete[] szRequest;
  delete[] szUri;
  delete[] szVer;
  delete sClient;
  return NULL;
}

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
  // iRc = ReadConfig("3wd.cf");
  // if (iRc) {
  //     std::cerr << "Error!" << std::endl;
  //     std::cerr << "Error reading configuration file. Exiting." <<
  //     std::endl; return 1; // Exit on error.
  // }
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
