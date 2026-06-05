#include "model/command_bar/command_bar.h"
#include <string.h>
#include "fw/drivers/timer.h"

static EditorMode mode = MODE_EDITOR;
static char filename[CMD_BUF_SIZE] = "untitled";
static char input[CMD_BUF_SIZE] = {0};
static int input_len = 0;
static char status_msg[CMD_BUF_SIZE] = {0};

//Countdown timer
static int status_ticks = 0;


void command_bar_init(const char *name) {
  if (name) strncpy(filename, name, CMD_BUF_SIZE - 1); 
  mode = MODE_EDITOR;
}

EditorMode command_bar_get_mode() { return mode; }

const char *command_bar_get_filename() { return filename; }

void command_bar_set_filename(const char *name) {
  strncpy(filename, name, CMD_BUF_SIZE - 1);
}

void command_bar_start() {
  memset(input, 0, CMD_BUF_SIZE);
  input_len = 0;
  mode = MODE_COMMAND;
}

void command_bar_start_prefill(const char *text) {
  command_bar_start();
  if (!text) return;
  while (*text && input_len < CMD_BUF_SIZE - 1)
    input[input_len++] = *text++;
  input[input_len] = '\0';
}

void command_bar_cancel() { mode = MODE_EDITOR; }

const char *command_bar_get_input() { return input; }

void command_bar_insert(char c) {
  if (input_len >= CMD_BUF_SIZE - 1) return;
  input[input_len++] = c;
  input[input_len] = '\0';
}

void command_bar_delete() {
  if (input_len > 0) input[--input_len] = '\0';
}

const char *command_bar_commit() {
  mode = MODE_EDITOR;
  return input;
}

void command_bar_set_status(const char *msg) {
  strncpy(status_msg, msg, CMD_BUF_SIZE - 1);
  status_msg[CMD_BUF_SIZE - 1] = '\0';
  status_ticks = TIMER_HZ; /* 1 second */
}

bool command_bar_tick() {
  if (status_ticks <= 0) return false;
  
  // Decrease and clear on expiry. Return true for caller to redraw
  if (--status_ticks == 0) { status_msg[0] = '\0'; return true; }
  return false;
}

const char *command_bar_get_status() { return status_msg; }
