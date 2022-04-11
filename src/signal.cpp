#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include "signal.hpp"

/* proto type function */

void set_signal_handler(int signame) {
  if (SIG_ERR == signal(signame, signal_handler)) {
    fprintf(stderr, "Error. Cannot set signal handler.\n");
    exit(1);
  }
}

void signal_handler(int signame) {
  printf("Signal(%d) : simple-server stopped.\n", signame);
  exit(0);
}
