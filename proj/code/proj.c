#include <lcom/lcf.h>

#include "fw/common/utils.h"

#include "controller/ih/ih.h"


int(proj_main_loop)(int argc, char *argv[]) {
  if (subscribe_interrupts() != OK)
    return fail(ERR, "proj_main_loop: unable to subscribe interrupts");

  if (unsubscribe_interrupts() != OK)
    return fail(ERR, "proj_main_loop: unable to unsubscribe interrupts");
  
  return 0;
}

int(main)(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/framework/trace.txt");
  lcf_log_output("/home/lcom/labs/framework/output.txt");
  
  if (lcf_start(argc, argv) != OK) {
    lcf_cleanup();
    return 1;
  }

  lcf_cleanup();
  return 0;
}
