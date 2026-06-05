#include <lcom/lcf.h>
#include <minix/sysutil.h>
#include <string.h>

#include "fw/drivers/rtc.h"
#include "fw/drivers/timer.h"
#include "fw/common/utils.h"
#include "fw/drivers/keyboard.h"
#include "fw/drivers/mouse.h"
#include "fw/drivers/video.h"
#include "fw/hw/vbe.h"

#ifdef ASSERT
  #undef ASSERT
#endif

#define ASSERT(x, msg)                                                         \
  if (!(x)) {                                                                  \
    printf("%s FAILED!\n", msg);                                               \
    exit(1);                                                                   \
  } else {                                                                     \
    printf("%s passed.\n", msg);                                               \
  }

void print_usage() {
  printf("\n  Usage:\n");
  printf("    ./proj <driver>\n");
  printf("    <driver>: error | rtc | timer | keyboard | mouse | video\n");
}

// Errors
void error_example();

// RTC
int rtc_example();
void test_rtc_date();

// Timer
int timer_example();

// Keyboard
#define ESC_BREAK 0x81
int keyboard_example();

// Mouse
int mouse_example();

// Video
int video_example(uint16_t mode);

int(proj_main_loop)(int argc, char *argv[]) {
  if (argc != 1) {
    print_usage();
    return fail(ERR, "argv[1]: missing argument");
  } else {
    if (strcmp(argv[0], "error") == 0) {
      error_example();

    } else if (strcmp(argv[0], "rtc") == 0) {
      rtc_example();
      test_rtc_date();

    } else if (strcmp(argv[0], "timer") == 0) {
      timer_example();

    } else if (strcmp(argv[0], "keyboard") == 0) {
      keyboard_example();

    } else if (strcmp(argv[0], "mouse") == 0) {
      mouse_example();

    } else if (strcmp(argv[0], "video") == 0) {
      video_example(VBE_864p_DC);
    } else {
      print_usage();
      return fail(ERR, "argv[1]: invalid argument");
    }
  }

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

void error_example() {
  fail(WARN, "This is a Warning");
  fail(ERR, "This is an Error");
  fail(ERR_RTC, "This is a RTC Error");
  fail(ERR_TIMER, "This is a Timer Error");
  fail(ERR_KBC, "This is a KBC Error");
  fail(ERR_KEYBOARD, "This is a Keybooard Error");
  fail(ERR_MOUSE, "This is a Mouse Error");
  fail(ERR_VIDEO, "This is a Video Error");
}

int rtc_example() {
  rtc_date date = {0, 0, 0, 0, 0, 0};
  if (rtc_read_date(&date) != OK) return 1;

  printf("\n\n");
  printf("RTC EXAMPLE \n");
  printf("seconds: %u \n", date.seconds);
  printf("minutes: %u \n", date.minutes);
  printf("hours: %u \n", date.hours);
  printf("day: %u \n", date.day);
  printf("month: %u \n", date.month);
  printf("year: %u", date.year);

  return 0;
}

void test_rtc_date() {
  rtc_date date = {0, 0, 0, 0, 0, 0};
  rtc_read_date(&date);

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tm_info = localtime(&ts.tv_sec);

  printf("\n\n");
  printf("RTC TEST \n");

  ASSERT(abs((int)date.day - (int)tm_info->tm_mday) < 2, "rtc read day");
  ASSERT(abs((int)date.month - (int)tm_info->tm_mon) < 2, "rtc read month");
  ASSERT(date.year == tm_info->tm_year % 100, "rtc read year");
}

int timer_example() {
  printf("\n\n");
  printf("Timer Example \n");

  uint8_t bit_no;

  int ipc_status;
  message msg;

  uint32_t seconds_to_quit = 5;

  if (timer_subscribe_int(&bit_no) != 0)
    return fail(ERR, "timer_example: unable to subscribe timer interrupt");

  if (timer_set_frequency(0, TIMER_HZ) != OK) {
    timer_unsubscribe_int();
    return fail(ERR_TIMER, "timer_example: unable to set timer frequency");
  }

  int irq_set = BIT(bit_no);

  int r;
  while (get_int_counter() < seconds_to_quit * TIMER_HZ) {
    
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) { /* received notification */
      
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE: /* hardware interrupt notification */
          
          if (msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */
            
            timer_int_handler();

            if(get_int_counter() % TIMER_HZ == 0)
              printf(" -%us\n", get_int_counter() / TIMER_HZ);
            }
          break;
      }
    }

    
  }

  timer_unsubscribe_int();

  return 0;
}

int keyboard_example() {
  printf("\n\n");
  printf("Keyboard Example \n");


  uint8_t bit_no;

  packet_scancode ps = {
    .two_byte = false,
    .make = false,
    .size = 0,
    .bytes = {0, 0}
  };

  int ipc_status;
  message msg;

  if (keyboard_subscribe_int(&bit_no) != 0)
    return fail(ERR, "keyboard_example: unable to subscribe keyboard interrupt");

  int irq_set = BIT(bit_no);

  int r;
  while (true) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            keyboard_ih();
            
            if (build_scancode(&ps) != OK)
              continue;

            if (ps.two_byte) {
              continue;
            }

            keyboard_print_scancode(ps);

            if (ps.bytes[0] == ESC_BREAK) {
              keyboard_unsubscribe_int();
              return 0;
            }
          }
          break;
        default:
          break;
      }
    }
  }

  // Should not reach this
  keyboard_unsubscribe_int();
  return 0;
}

int mouse_example() {
  printf("\n\n");
  printf("Mouse Example \n");

  uint8_t bit_no;
  int ipc_status;
  message msg;
  int r;

  uint32_t packets_read = 0;

  if (mouse_subscribe_int(&bit_no) != OK)
    return fail(ERR_MOUSE, "mouse_example: unable to subscribe mouse interrupt");
  if (mouse_enable_wheel_mode() != OK) {
    mouse_unsubscribe_int();
    return fail(ERR_MOUSE, "mouse_example: unable to enable wheel mode");
  }
  if (my_mouse_enable_data_reporting() != OK) {
    mouse_unsubscribe_int();
    return fail(ERR_MOUSE, "mouse_example: unable to enable data reporting");
  }

  uint8_t irq_set = BIT(bit_no);

  while (true) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != OK)
      continue;

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            mouse_ih();

            if (is_packet_ready()) {
              mouse_packet pp;
              if (build_packet(&pp) != OK) {
                fail(ERR_MOUSE, "mouse_example: unable to build packet");
                continue;
              }
              my_mouse_print_packet(&pp);

              packets_read++;

              if (pp.rb && pp.lb) {
                if (my_mouse_disable_data_reporting() != OK)
                  return fail(ERR_MOUSE, "mouse_example: unable to disable data reporting");
                if (mouse_unsubscribe_int() != OK)
                  return fail(ERR_MOUSE, "mouse_example: unable to unsubscribe mouse interrupt");

                return 0;
              }
            }
          }
          break;
        default:
          break;
      }
    }
  }

  if (my_mouse_disable_data_reporting() != OK)
    return fail(ERR_MOUSE, "mouse_example: unable to disable data reporting");
  if (mouse_unsubscribe_int() != OK)
    return fail(ERR_MOUSE, "mouse_example: unable to unsubscribe mouse interrupt");

  return 0;
}

int video_example(uint16_t mode) {
  printf("\n\n");
  printf("Video Example \n");

  if (vg_map_vram(mode) != OK) return 1;

  if (set_graphics_mode(mode) != OK) return 1;

  if (vg_draw_rectangle(10, 10, 100, 100, 0xFF0000) != OK) return 1;

  if (keyboard_example() != OK) {
    set_text_mode();
    return 1;
  }

  if (set_text_mode() != OK) return 1;

  return 0;
}
