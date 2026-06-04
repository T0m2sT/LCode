#include "controller/input/events.h"

#include <stddef.h>

#include "controller/commands.h"

#define INPUT_EVENT_QUEUE_SIZE 64

static InputEvent queue[INPUT_EVENT_QUEUE_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

bool input_event_push(InputEvent ev) {
  if (count >= INPUT_EVENT_QUEUE_SIZE) {
    head = (head + 1) % INPUT_EVENT_QUEUE_SIZE;
    count--;
  }
  queue[tail] = ev;
  tail = (tail + 1) % INPUT_EVENT_QUEUE_SIZE;
  count++;
  return true;
}

bool input_event_pop(InputEvent *out) {
  if (count == 0) return false;
  if (out != NULL) *out = queue[head];
  head = (head + 1) % INPUT_EVENT_QUEUE_SIZE;
  count--;
  return true;
}

bool input_events_is_empty() { return count == 0; }

void input_events_clear() {
  head = 0;
  tail = 0;
  count = 0;
}

void input_dispatch() {
  InputEvent ev;
  while (input_event_pop(&ev)) {
    switch (ev.type) {
      case INPUT_EVENT_KEY:
        commands_dispatch(ev.data.key);
        break;
      case INPUT_EVENT_MOUSE:
        commands_dispatch_mouse(ev.data.mouse);
        break;
      case INPUT_EVENT_SERIAL:
        commands_dispatch_serial(ev.data.serial);
      default:
        break;
    }
  }
}
