#include <lcom/lcf.h>

#include "fw/common/utils.h"
#include <unistd.h>
#include <sys/stat.h>

#define WORK_DIR "/home/lcom/labs/proj/docs"

#include "controller/ih/ih.h"
#include "controller/commands.h"
#include "view/video.h"
#include "view/scene.h"
#include "model/editor.h"
#include "model/command_bar.h"
#include "model/filetree.h"
#include "render_flag.h"


#define TIMER_HZ 60

int(proj_main_loop)(int argc, char *argv[]) {
  int ipc_status, r;
  message msg;

  mkdir(WORK_DIR, 0755);
  chdir(WORK_DIR);

  command_bar_init(argc > 1 ? argv[1] : "untitled");
  filetree_init();

  if (editor_init() != OK)
    return fail(ERR, "proj_main_loop: editor_init failed");

  if (video_init() != OK)
    return fail(ERR_VIDEO, "proj_main_loop: video_init failed");

  if (scene_init(SCENE_EDITOR) != OK) {
    video_cleanup();
    return fail(ERR, "proj_main_loop: scene_init failed");
  }

  if (subscribe_interrupts() != OK)
    return fail(ERR, "proj_main_loop: unable to subscribe interrupts");

  if (timer_set_frequency(0, TIMER_HZ) != OK) {
    unsubscribe_interrupts();
    return fail(ERR_TIMER, "proj_main_loop: unable to set timer frequency");
  }

  while (!get_quit())
  {
    if ((r = driver_receive(ANY, &msg, &ipc_status))!= OK) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          interrupts_handler(msg.m_notify.interrupts);
          break;

        default:
          break;
      }
    }

    if (get_render())
      view_render();
  }

  scene_cleanup();
  video_cleanup();

  if (unsubscribe_interrupts() != OK)
    return fail(ERR, "proj_main_loop: unable to unsubscribe interrupts");

  return 0;
}

int(main)(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/proj/trace.txt");
  lcf_log_output("/home/lcom/labs/proj/output.txt");
  
  if (lcf_start(argc, argv) != OK) {
    lcf_cleanup();
    return 1;
  }

  lcf_cleanup();
  return 0;
}
