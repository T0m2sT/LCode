#include "controller/commands/commands.h"
#include "controller/commands/filetree_commands.h"
#include "model/filetree/filetree.h"
#include "model/editor/editor.h"
#include "model/command_bar/command_bar.h"
#include "view/renderer/scene.h"
#include "view/editor/syntax.h"
#include "model/render_state.h"
#include "controller/serial.h"
#include <stdio.h>
#include <string.h>

#define FILE_READ_BUF 4096

static bool quit_flag = false;
static bool remote = false;

/* Use instead of set_render when the operation might have scrolled the
 * viewport — upgrades to RENDER_FULL if the scroll offset changed. */
static void set_render_ex(int mode) {
  bool needs_full = editor_consume_scroll_dirty() || editor_consume_sel_dirty() || editor_sel_is_active();
  set_render(needs_full ? RENDER_FULL : mode);
}

bool get_quit() { return quit_flag; }

static void execute_save(const char *name) {
  if (!name || name[0] == '\0') return;
  FILE *f = fopen(name, "w");
  if (!f) return;
  for (int r = 0; r < editor_get_row_count(); r++)
    fprintf(f, "%s\n", editor_get_line(r));
  fclose(f);
  command_bar_set_filename(name);
  // update syntax language in case the extension changed
  scene_set_language(syntax_detect_language(name));
}

static void execute_open(const char *name) {
  if (!name || name[0] == '\0') return;
  FILE *f = fopen(name, "r");
  if (!f) return;

  // reset buffer before loading new content
  editor_init();

  char line[FILE_READ_BUF];
  while (fgets(line, sizeof(line), f)) {
    int len = strlen(line);
    // strip trailing newline from fgets output
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (editor_load_line(line, len) != EDITOR_OK) break;
  }
  fclose(f);
  editor_load_finalize();
  command_bar_set_filename(name);
  scene_set_language(syntax_detect_language(name));
}

/*
 * Executes the full file synchronization process over the serial port.
 * Starts by sending a CMD_FILE_START packet to prepare the remote editor,
 * and then iterates through all lines of the local file, sending them via CMD_FILE_LINE.
 * Finally, updates the remote cursor position to match the local cursor.
 */
static void execute_sync() {
  
  const char *filename = command_bar_get_filename();
  int filename_len = strlen(filename);
  int total_lines = editor_get_row_count();

  build_packet_serial(CMD_FILE_START, (uint8_t*)filename, filename_len, total_lines);

  //Send line by line
  for (int i = 0; i < total_lines; i++) {
    const char *line = editor_get_line(i);
    build_packet_serial(CMD_FILE_LINE, (uint8_t*)line, editor_get_line_len(i), 0);
  }
    
  //Send the cursor position
  uint8_t load[4];
  int r = editor_get_cursor_row();
  int c = editor_get_cursor_col();
  load[0] = (r >> 8) & 0xFF;
  load[1] = r & 0xFF;
  load[2] = (c >> 8) & 0xFF;
  load[3] = c & 0xFF;
  build_packet_serial(CMD_MOVE_CURSOR, load, 4, 0);
}

/*
 * Command bar handler for the ":sync" command.
 * Enables the remote synchronization mode and triggers the full file share process.
 */
static void cmd_sync(const char *args) {
  (void)args;
  remote = true; 
  execute_sync();
  command_bar_set_status("Ficheiro Partilhado!");
}


static void cmd_save(const char *args) {
  const char *name = (args && args[0]) ? args : command_bar_get_filename();
  execute_save(name);
}

static void cmd_open(const char *args) {
  execute_open(args);
}

static void cmd_quit(const char *args) {
  (void)args;
  quit_flag = true;
}

typedef void (*CommandHandler)(const char *args);
typedef struct { const char *name; CommandHandler handler; } CommandEntry;

static const CommandEntry command_table[] = {
  { "save", cmd_save },
  { "w",    cmd_save },
  { "open", cmd_open },
  { "q",    cmd_quit },
  { "sync", cmd_sync },
  { NULL,   NULL }
};

static void execute_command(const char *raw) {
  char name[CMD_BUF_SIZE];
  const char *args = "";
  // split raw input at the first space into command name and args
  int i = 0;
  while (raw[i] && raw[i] != ' ' && i < CMD_BUF_SIZE - 1) i++;
  strncpy(name, raw, i);
  name[i] = '\0';
  if (raw[i] == ' ') args = raw + i + 1;
  // linear scan of command table; NULL-terminated sentinel ends the loop
  for (int j = 0; command_table[j].name; j++) {
    if (strcmp(command_table[j].name, name) == 0) {
      command_table[j].handler(args);
      return;
    }
  }
}

static void dispatch_command_mode(KeyEvent ev) {
  if (ev.escape) {
    command_bar_cancel();
    set_render(RENDER_FULL);
    return;
  }
  if (ev.enter) {
    const char *input = command_bar_commit();
    execute_command(input);
    set_render(RENDER_FULL);
    return;
  }
  if (ev.backspace && !ev.ctrl) {
    command_bar_delete();
    set_render(RENDER_STATUS);
    return;
  }
  if (ev.c) {
    command_bar_insert(ev.c);
    set_render(RENDER_STATUS);
  }
}

/*
 * Calculates and synchronizes a block of memory over the serial port.
 * Implements the Delta Sync logic for complex operations (like paste or cut).
 * If exactly 1 line is modified, it sends a simple CMD_UPDATE_LINE.
 * If multiple lines are affected, it sends a CMD_REPLACE_BLOCK to adjust the remote
 * heap memory dynamically, followed by multiple CMD_UPDATE_LINE packets to fill the text.
 */
static void sync_block(int start_row, int deleted_count, int inserted_count) {
  if (!remote) return;
  
  if (deleted_count == 1 && inserted_count == 1){
    const char *text = editor_get_line(start_row);
    build_packet_serial(CMD_UPDATE_LINE, (uint8_t*)text, editor_get_line_len(start_row), start_row);
  }
  else{
    uint8_t payload[6];
    payload[0] = (start_row >> 8) & 0xFF;
    payload[1] = start_row & 0xFF;
    payload[2] = (deleted_count >> 8) & 0xFF;
    payload[3] = deleted_count & 0xFF;
    payload[4] = (inserted_count >> 8) & 0xFF;
    payload[5] = inserted_count & 0xFF;
    build_packet_serial(CMD_REPLACE_BLOCK, payload, 6, 0);
      
    for (int i = 0; i < inserted_count; i++) {
      int r = start_row + i;
      const char *text = editor_get_line(r);
      build_packet_serial(CMD_UPDATE_LINE, (uint8_t*)text, editor_get_line_len(r), r);
    }
  }

  uint8_t load[4];
  int r = editor_get_cursor_row();
  int c = editor_get_cursor_col();
  load[0] = (r >> 8) & 0xFF;
  load[1] = r & 0xFF;
  load[2] = (c >> 8) & 0xFF;
  load[3] = c & 0xFF;
  build_packet_serial(CMD_MOVE_CURSOR, load, 4, 0);
}

void commands_open_file(const char *path) {
  execute_open(path);
}

void commands_dispatch(KeyEvent ev) {
  // filetree consumes the event if it is focused
  if (filetree_commands_try(ev)) return;

  if (command_bar_get_mode() == MODE_COMMAND) {
    dispatch_command_mode(ev);
    return;
  }

  // escape and ctrl+q are both quit shortcuts
  if (ev.escape || (ev.ctrl && ev.c == 'q')) {
    quit_flag = true;
    return;
  }

  if (ev.backspace) {
    EditorResult r;
    //delete selection if active
    if (editor_sel_is_active()) {
      int sr, sc, er, ec;
      editor_sel_get_range(&sr, &sc, &er, &ec);
      int deleted_count = er - sr + 1;
      
      r = editor_delete_selection();
      if (r == EDITOR_ERR_ALLOC_FAILED) {
        command_bar_set_status("Out of memory"); 
        set_render(RENDER_STATUS); 
      }
      else {
        if (remote) sync_block(sr, deleted_count, 1); 
        set_render_ex(RENDER_FULL);
      }
    }
    //control + del
    else if (ev.ctrl) {
      //if at first char of line, go to previous line and delete
      if (editor_get_cursor_col() == 0) {
        int sr = editor_get_cursor_row() - 1;
        r = editor_delete_char();
        if (r == EDITOR_ERR_ALLOC_FAILED) {
          command_bar_set_status("Out of memory");
          set_render(RENDER_STATUS);
        }
        else {
          if (remote) sync_block(sr, 2, 1);
          set_render_ex(RENDER_FULL);
        }  
      } 
      //delete word
      else {
        int sr = editor_get_cursor_row();
        r = editor_delete_word();
        if (r == EDITOR_ERR_ALLOC_FAILED) {
          command_bar_set_status("Out of memory");
          set_render(RENDER_STATUS);
        }
        else {
          if (remote) sync_block(sr, 1, 1);
          set_render_ex(RENDER_LINE);
        }  
      }
    } 
    //normal del
    else {
      bool mid_line = (editor_get_cursor_col() > 0);
      r = editor_delete_char();
      if (r == EDITOR_ERR_ALLOC_FAILED) {
        command_bar_set_status("Out of memory");
        set_render(RENDER_STATUS);
      }
      else if (r == EDITOR_OK) { 
        if (remote) {       
          build_packet_serial(CMD_DELETE_CHAR, NULL, 0, 0);
        }
        set_render_ex(mid_line ? RENDER_LINE : RENDER_FULL);
      }
    }
    return;
  }

  if (ev.dir != DIR_NONE) {
    // shift extends selection from anchor; no shift clears it
    if (ev.shift) {
      if (!editor_sel_is_active()) editor_sel_set_anchor();
    } 
    else editor_sel_clear();
  
    switch (ev.dir) {
      case DIR_LEFT: ev.ctrl ? editor_move_word_left() : editor_move_left(); break;
      case DIR_RIGHT: ev.ctrl ? editor_move_word_right() : editor_move_right(); break;
      case DIR_UP: editor_move_up(); break;
      case DIR_DOWN: editor_move_down(); break;
      case DIR_HOME: editor_move_home(); break;
      case DIR_END: editor_move_end(); break;
      default: break;
    }
    
    if (remote) {
      uint8_t payload[4];
      int r = editor_get_cursor_row();
      int c = editor_get_cursor_col();
      payload[0] = (r >> 8) & 0xFF;
      payload[1] = r & 0xFF;
      payload[2] = (c >> 8) & 0xFF;
      payload[3] = c & 0xFF;
      build_packet_serial(CMD_MOVE_CURSOR, payload, 4, 0);
    }
    set_render_ex(RENDER_CHAR);
    return;
  }

  if (ev.ctrl && ev.c == 'c') {
    editor_copy_selection();
    return;
  }
  if (ev.ctrl && ev.c == 'x') {
    editor_copy_selection();
    if (editor_sel_is_active()) {
      int sr, sc, er, ec;
      editor_sel_get_range(&sr, &sc, &er, &ec);
      int deleted_count = er - sr + 1;
        
      EditorResult r = editor_delete_selection();
      
      if (r == EDITOR_ERR_ALLOC_FAILED) {
        command_bar_set_status("Out of memory");
        set_render(RENDER_STATUS);
      } else {
        if (remote) sync_block(sr, deleted_count, 1); // Cortar deixa 1 linha
        set_render_ex(RENDER_FULL);
      }
    }
    return;
  }
  if (ev.ctrl && ev.c == 'v') {
    int start_r = editor_get_cursor_row();
    int deleted_count = 1;//normal paste

    if (editor_sel_is_active()){ 
      int sr,sc,er,ec;
      editor_sel_get_range(&sr, &sc, &er, &ec);
      start_r = sr;
      deleted_count = er - sr + 1; 
      editor_delete_selection();
    }  
    
    int old_rows = editor_get_row_count();
    EditorResult result = editor_paste();
    int inserted_count = editor_get_row_count() - old_rows + 1;//number of lines
    
    switch (result) {
      case EDITOR_OK:{
        if (remote) sync_block(start_r, deleted_count, inserted_count);
        set_render_ex(RENDER_FULL);
        break;
      } 
      case EDITOR_ERR_NO_CLIPBOARD: {
        command_bar_set_status("Nothing to paste");
        set_render(RENDER_STATUS); 
        break;
      }
      case EDITOR_ERR_ALLOC_FAILED: {
        command_bar_set_status("Out of memory");
        set_render(RENDER_STATUS); 
        break;
      }
    }
    return;
  }
  if (ev.ctrl && ev.c == 's') {
    // untitled file: prompt for a name instead of saving immediately
    if (strcmp(command_bar_get_filename(), "untitled") == 0)
      command_bar_start_prefill("save ");
    else
      execute_save(command_bar_get_filename());
    set_render(RENDER_FULL);
    return;
  }
  if (ev.ctrl && ev.c == 'o') {
    command_bar_start_prefill("open ");
    set_render(RENDER_FULL);
    return;
  }
  if (ev.ctrl) return;

  if (ev.c == ':') {
    command_bar_start();
    set_render(RENDER_FULL);
    return;
  }

  if (ev.enter) {
    if (editor_sel_is_active()){
      int sr, sc, er, ec;
      editor_sel_get_range(&sr, &sc, &er, &ec);
      int deleted_count = er - sr + 1;
      
      editor_delete_selection();
      
      EditorResult r = editor_insert_char('\n');
      if (r == EDITOR_OK) {
        if (remote) sync_block(sr, deleted_count, 2);
        set_render(RENDER_FULL);
      }
      return;
    }
    EditorResult r = editor_insert_char('\n');
    if (r == EDITOR_ERR_ALLOC_FAILED) {
      command_bar_set_status("Out of memory");
      set_render(RENDER_STATUS); 
    }
    else if (r == EDITOR_OK) {
      if (remote) {
        uint8_t c = '\n';
        build_packet_serial(CMD_INSERT_CHAR, &c, 1, 0);
      }
      set_render(RENDER_FULL);
    }
    return;
  }

  if (ev.c) {
    if (editor_sel_is_active()){
      int sr, sc, er, ec;
      editor_sel_get_range(&sr, &sc, &er, &ec);
      int deleted_count = er - sr + 1;
      
      editor_delete_selection();
      
      EditorResult r = editor_insert_char(ev.c);
      if (r == EDITOR_OK) {
        if (remote) sync_block(sr, deleted_count, 1);
        set_render(RENDER_FULL);
      }
      return;
    }
    
    EditorResult r = editor_insert_char(ev.c);
    if (r == EDITOR_ERR_ALLOC_FAILED) {
      command_bar_set_status("Out of memory");
      set_render(RENDER_STATUS);
    }
    else if (r == EDITOR_OK) {
      if (remote) { 
        uint8_t c = ev.c;
        build_packet_serial(CMD_INSERT_CHAR, &c, 1, 0);
      }
      set_render_ex(RENDER_LINE);
    }
  }
}

void commands_dispatch_mouse(MouseEvent me) {
  // release clears any active selection
  if (!me.left_holding) {
    editor_sel_clear();
  }

  if (!me.left_clicked && me.scroll == 0 && !me.left_holding) return;

  if (scene_click_scrollbar(me.click_x, me.click_y)) return;

  if (filetree_commands_mouse(me)) return;

  if (me.scroll != 0) {
    editor_scroll_by(me.scroll * SCROLL_SPEED_MULTIPLIER, 0);
    set_render(RENDER_FULL);
    return;
  }

  int row, col;
  if (scene_px_to_text(me.click_x, me.click_y, &row, &col) && me.left_clicked) {
    // click: move cursor and start a new selection anchor
    editor_sel_clear();
    editor_set_cursor(row, col);
    if (remote) {
      uint8_t payload[4];
      payload[0] = (row >> 8) & 0xFF;
      payload[1] = row & 0xFF;
      payload[2] = (col >> 8) & 0xFF;
      payload[3] = col & 0xFF;
      build_packet_serial(CMD_MOVE_CURSOR, payload, 4, 0);
    }
    set_render_ex(RENDER_CHAR);
    if (!editor_sel_is_active()) editor_sel_set_anchor();
  } else if (scene_px_to_text(me.click_x, me.click_y, &row, &col) && me.left_holding) {
    // drag: extend selection by moving cursor
    editor_set_cursor(row, col);
  }

}

static uint16_t file_num_lines = 0;
static uint16_t lines_contador = 0;

/*
 * Dispatches and executes an incoming serial event on the local editor.
 * Maps each SerialCommand received from the network to the corresponding 
 * local editor function (e.g., editor_remote_insert_char, editor_remote_replace_block).
 * Also controls the render flags to optimize screen redraws based on the impact of the command.
 */
void commands_dispatch_serial(SerialEvent se) {
  if (se.payload_len == 0 && se.cmd != CMD_DELETE_CHAR && se.cmd != CMD_FILE_LINE) return;

  EditorResult r;
  
  switch (se.cmd) {
    case CMD_INSERT_CHAR:
      r = editor_remote_insert_char(se.payload_buf[0]);
      if (r == EDITOR_ERR_ALLOC_FAILED) {
        command_bar_set_status("Out of memory");
        set_render(RENDER_STATUS);
      }
      set_render_ex(RENDER_REMOTE_LINE);
      break;
      
    case CMD_DELETE_CHAR:{
      bool mid_line = (editor_get_remote_cursor_col() > 0);
      r = editor_remote_delete_char();
      if (r == EDITOR_ERR_ALLOC_FAILED) {
        command_bar_set_status("Out of memory");
        set_render(RENDER_STATUS);
      }
      else set_render_ex(mid_line ? RENDER_REMOTE_LINE : RENDER_FULL);
      break;
    }  
    case CMD_MOVE_CURSOR:{
      // row and col are each encoded as big-endian uint16 (2 bytes each)
      uint8_t row_msb = se.payload_buf [0];
      uint8_t row_lsb = se.payload_buf [1];
      uint8_t col_msb = se.payload_buf [2];
      uint8_t col_lsb = se.payload_buf [3];

      int remote_cursor_row = row_msb << 8 | row_lsb;
      int remote_cursor_col = col_msb << 8 | col_lsb;

      // row changed: old and new lines both need redraw
      if (editor_get_remote_cursor_row()!=remote_cursor_row){
        editor_set_remote_cursor(remote_cursor_row,remote_cursor_col);
        set_render_ex(RENDER_FULL);
      }
      // same row: only redraw that line
      else{
        editor_set_remote_cursor(remote_cursor_row,remote_cursor_col);
        set_render_ex(RENDER_REMOTE_LINE);
      }
      break;
    }
    case CMD_REPLACE_BLOCK:{
      uint16_t start_row = (se.payload_buf[0] << 8) | se.payload_buf[1];
      uint16_t deleted   = (se.payload_buf[2] << 8) | se.payload_buf[3];
      uint16_t inserted  = (se.payload_buf[4] << 8) | se.payload_buf[5];
      
      editor_remote_replace_block(start_row, deleted, inserted);
      //not necessary rend_full because new lines will arrive later
      break;
    }
    case CMD_UPDATE_LINE:{
      uint16_t row_index = (se.payload_buf[0] << 8) | se.payload_buf[1];
      
      int text_len = se.payload_len - 2;
      
      editor_remote_update_line(row_index, (const char*)&se.payload_buf[2], text_len);
      
      set_render_ex(RENDER_FULL);
      break;
    }
    case CMD_FILE_START:{
      editor_init();//clean the screen to receive the new file
      
      file_num_lines = (se.payload_buf[0] << 8) | se.payload_buf[1];
      lines_contador = 0; 
      
      char file_name[256];
      int name_len = se.payload_len - 2;
      
      memcpy(file_name, &se.payload_buf[2], name_len);
      file_name[name_len] = '\0';
      
      command_bar_set_filename(file_name);
      command_bar_set_status("A Receber Ficheiro...");
      remote=true;
      set_render_ex(RENDER_FULL); 
      break;
    }  
    case CMD_FILE_LINE:{
      if (file_num_lines==0){
        break;
      }
      
      lines_contador++;
      editor_load_line((const char*) se.payload_buf,se.payload_len);
      
      if (lines_contador>=file_num_lines){
        set_render_ex(RENDER_FULL);
        command_bar_set_status("");
        editor_load_finalize();
        file_num_lines=0;
      }
      break;
    }  
    default:
      break;
  }
}


