#pragma once

#include <stdbool.h>

#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include "controller/serial.h"

typedef enum {
  INPUT_EVENT_KEY,
  INPUT_EVENT_MOUSE,
  INPUT_EVENT_SERIAL
} InputEventType;

typedef struct {
  InputEventType type;
  union {
    KeyEvent key;
    MouseEvent mouse;
    SerialEvent serial;
  } data;
} InputEvent;

bool input_event_push(InputEvent ev);
bool input_event_pop(InputEvent *out);
bool input_events_is_empty();
void input_events_clear();
void input_dispatch();
