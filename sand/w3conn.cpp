#include <cstdio>
#include "main.hpp"

static int DoHttp11(Socket* sClient, char* szMethod, char* szUri) { return 0; }

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
