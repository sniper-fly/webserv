//
// WWW Server  File: cgios2.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES
#define Sleep(x) DosSleep(x) // Portability definition

#ifdef __IBMCPP__
#include <builtin.h>
#endif

#include "3wd.hpp"
#include "http11.hpp"
#include "cgi.hpp"
#include "config.hpp"
#include "defines.hpp"


#define SMALLBUF 4196
#define STDIN    0x00000000
#define STDOUT   0x00000001

// ------------------------------------------------------------------

volatile int iCgiLock = 0; // Ram semaphore for CGI access.

// The environment variables passed to the cgi process.
char szServerSoftware[64], szServerName[64], szGatewayInterface[64],
    szServerProtocol[64], szServerPort[64], szRequestMethod[64],
    szScriptName[64], *szQueryString, szRemoteHost[64], szRemoteAddr[64],
    szAuthType[64], szRemoteUser[64], szContentType[64], szContentLength[64],
    *szEnvs[15];

// ------------------------------------------------------------------
//
// ExecCgi
//
// This function executes the specified cgi program passing it the
// necessary arguments. It then returns the output to the caller
// for them to return it to the client. We protect this function
// internally with a ram semaphore since there is only one stdin/stdout
// stream for all of our threads.
//

int ExecCgi(Cgi* cParms) {
  int           fds[2]; //, iRc;
  char          szBuf[SMALLBUF], *szArgs[2];
  std::ofstream ofOut;

  // Lock all the other threads out.
  sem_wait(g_cgiSem);

  // Setting the environment variables.
  sprintf(szServerProtocol, "SERVER_PROTOCOL=%s", cParms->hInfo->szVer);
  sprintf(szRequestMethod, "REQUEST_METHOD=%s", cParms->hInfo->szMethod);
  sprintf(szScriptName, "SCRIPT_NAME=%s", cParms->hInfo->szUri);
  if (cParms->hInfo->szQuery != NULL) {
    szQueryString = new char[(strlen(cParms->hInfo->szQuery) + 15)];
    sprintf(szQueryString, "QUERY_STRING=%s", cParms->hInfo->szQuery);
  } else {
    szQueryString = new char[15];
    strcpy(szQueryString, "QUERY_STRING");
  }
  // Since szQueryString is dynamic memory, we must reassign it each time.
  szEnvs[7] = szQueryString;
  if (cParms->sClient->szPeerName != NULL) {
    sprintf(szRemoteHost, "REMOTE_HOST=%s", cParms->sClient->szPeerName);
  } else {
    strcpy(szRemoteHost, "REMOTE_HOST");
  }
  sprintf(szRemoteAddr, "REMOTE_ADDR=%s", cParms->sClient->szPeerIp);
  if (cParms->hInfo->szAuthType != NULL) {
    sprintf(szAuthType, "AUTH_TYPE=%s", cParms->hInfo->szAuthType);
    sprintf(szRemoteUser, "REMOTE_USER=%s", cParms->hInfo->szRemoteUser);
  } else {
    strcpy(szAuthType, "AUTH_TYPE");
    strcpy(szRemoteUser, "REMOTE_USER");
  }
  if (cParms->hInfo->szContentType != NULL) {
    sprintf(szContentType, "CONTENT_TYPE=%s", cParms->hInfo->szContentType);
  } else {
    strcpy(szContentType, "CONTENT_TYPE");
  }
  if (strcmp(cParms->hInfo->szMethod, "POST") == 0) {
    sprintf(
        szContentLength, "CONTENT_LENGTH=%d", (int)cParms->hInfo->ulContentLength);
  } else {
    strcpy(szContentLength, "CONTENT_LENGTH=0");
  }
  szArgs[0] = cParms->szProg; // The program to run.
  szArgs[1] = NULL;

  // pipeは、自分のプロセスへの読み取りと書き込み用のファイルディスクリプタを返してくれる。
  // fds[0]はread fds[1]はwrite
  pipe(fds);
  // Start it.
  int   status;
  pid_t pid = fork();
  if (pid == 0)
  {
    // 子プロセスの場合

    // pipeで作成したファイルディスクリプタはforkした場合、そのまま子プロセスに複製される。
    // これにより、子プロセスで書き込み用ファイルディスクリプタに書いて、親プロセスで読み取り用のファイルディスクリプタを読むことでやりとりが可能。

    // 子プロセスではread用のファイルディスクリプタは使わないのでクローズする。
    close(fds[0]);
    // もともとの標準出力は使わないのでクローズ。
    close(STDOUT_FILENO);
    // 標準出力にpipeで作った書き込み用ファイルディスクリプタを割り当てる
    dup2(fds[1], STDOUT_FILENO);

    execve(cParms->szProg, szArgs, szEnvs);
  }
  // 親プロセスは、write用のファイルディスクリプタは使わないのでクローズする。
  close(fds[1]);
  // 子プロセスの終了を待つ
  waitpid(-1, &status, 0);
  // 子プロセスが書き出した結果を読み込む
  read(fds[0], szBuf, SMALLBUF);

  std::cerr << "[ExecCGI] szBuf=" << szBuf << std::endl;

  // POSTのBodyはどうせ読まない
/*
  if (cParms->szPost != NULL) // Use POST method.
  {
    fpPost = fopen(cParms->szPost, "rb");
    iNum   = 1;
    while (iNum > 0) {
      iNum = fread(szBuf, sizeof(char), SMALLBUF, fpPost);
      fwrite(szBuf, sizeof(char), iNum, fpout);
    }
    fclose(fpPost);
  }
  fclose(fpout);
*/
  cParms->szOutput = MakeUnique((char *)"tmp/tmpCgi/", (char *)"txt");
  ofOut.open(cParms->szOutput, std::ios::binary);
  ofOut << szBuf;
  // Grab all of the output from the child.
/*
  while ((iNum = read(szBuf, sizeof(char), SMALLBUF, fpin)) != 0) {
    ofOut.write(szBuf, iNum);
  }
*/
  ofOut.close();

  delete[] szQueryString;

  // Unlock cgi access.
  sem_post(g_cgiSem);
  return (0);
}

// ------------------------------------------------------------------
//
// InitCgi
//
// Initialize some of the global variables needed for the CGI
// processing.
//

void InitCgi() {
  szEnvs[0]  = szServerSoftware;
  szEnvs[1]  = szServerName;
  szEnvs[2]  = szGatewayInterface;
  szEnvs[3]  = szServerProtocol;
  szEnvs[4]  = szServerPort;
  szEnvs[5]  = szRequestMethod;
  szEnvs[6]  = szScriptName;
  szEnvs[7]  = szQueryString;
  szEnvs[8]  = szRemoteHost;
  szEnvs[9]  = szRemoteAddr;
  szEnvs[10] = szAuthType;
  szEnvs[11] = szRemoteUser;
  szEnvs[12] = szContentType;
  szEnvs[13] = szContentLength;
  szEnvs[14] = NULL;

  // These are the same for all requests, so only set once.
  sprintf(szServerSoftware, "SERVER_SOFTWARE=%s", szServerVer);
  sprintf(szServerName, "SERVER_NAME=%s", szHostName);
  strcpy(szGatewayInterface, "GATEWAY_INTERFACE=CGI/1.1");
  sprintf(szServerPort, "SERVER_PORT=%d", sPort);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
