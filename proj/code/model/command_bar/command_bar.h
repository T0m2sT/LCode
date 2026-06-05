#pragma once

#include <lcom/lcf.h>

/**
 * @file command_bar.h
 * @brief Status bar state: mode switching, command input, filename, and timed status messages.
 */


/** @brief Maximum number of characters in the command input buffer */
#define CMD_BUF_SIZE 128

typedef enum { MODE_EDITOR, MODE_COMMAND } EditorMode;



/**
 * @brief Initializes the command bar with the given filename
 * @param filename Initial filename to display in the status bar
 */
void command_bar_init(const char *filename);

/**
 * @brief Returns the current editor mode
 * @return MODE_EDITOR during normal editing; MODE_COMMAND when typing a command
 */
EditorMode command_bar_get_mode();

/**
 * @brief Returns the filename currently shown in the status bar
 * @return Null-terminated filename string
 */
const char *command_bar_get_filename();

/**
 * @brief Updates the filename shown in the status bar
 * @param name Null-terminated filename string
 */
void command_bar_set_filename(const char *name);

/**
 * @brief Switches to MODE_COMMAND with an empty input buffer
 */
void command_bar_start();

/**
 * @brief Switches to MODE_COMMAND with a pre-filled input buffer
 * @param text Initial content for the command input
 */
void command_bar_start_prefill(const char *text);

/**
 * @brief Cancels the active command and returns to MODE_EDITOR
 */
void command_bar_cancel();

/**
 * @brief Returns the current command input text.
 * @return Null-terminated string with the text typed so far. Empty string outside MODE_COMMAND
 */
const char *command_bar_get_input();

/**
 * @brief Appends a character to the command input buffer
 * @param c Character to insert
 */
void command_bar_insert(char c);

/**
 * @brief Deletes the last character from the command input buffer
 */
void command_bar_delete();

/**
 * @brief Finalizes the command input and returns to MODE_EDITOR
 * @return Null-terminated string with the committed command text
 */
const char *command_bar_commit();

/**
 * @brief Displays a timed status message in the status bar
 * @param msg Null-terminated message string. Shown until it expires or is replaced
 */
void command_bar_set_status(const char *msg);

/**
 * @brief Advances the status message timer and clears it if expired
 *
 * Should be called once per tick. Returns true if the message just expired,
 * signalling that the status bar needs a redraw.
 * @return true if the status message expired this tick
 */
bool command_bar_tick();

/**
 * @brief Returns the current status message
 * @return Null-terminated status string. Empty string if no message is active
 */
const char *command_bar_get_status();
