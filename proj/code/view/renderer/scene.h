#pragma once

#include <lcom/lcf.h>
#include "view/editor/syntax.h"

/**
 * @file scene.h
 * @brief Editor scene rendering and screen-to-model hit-mapping
 */

typedef enum {
  SCENE_EDITOR
} SceneID;

/**
 * @brief Initializes the scene, layout, and mouse cursor.
 * @param id The scene to display.
 * @return 0 on success. Non-zero on failure.
 */
int scene_init(SceneID id);

/**
 * @brief Releases all resources owned by the scene
 */
void scene_cleanup();

/**
 * @brief Returns the number of visible text rows in the editor area
 * @return Visible row count
 */
int scene_get_vis_rows();

/**
 * @brief Renders the current scene and flips changed regions to the screen
 */
void view_render();

/**
 * @brief Screen coordinates to text position mapping. Converts pixel coordinates to editor row and column
 * @param px Pixel x coordinate
 * @param py Pixel y coordinate
 * @param out_row Output row index (0-based)
 * @param out_col Output column index (0-based), clamped to line length
 * @return true if the pixel falls inside the editor text area, false otherwise
 */
bool scene_px_to_text(int px, int py, int *out_row, int *out_col);

/**
 * @brief Handles a click on the scrollbar, scrolling to the clicked position
 * @param px Pixel x coordinate
 * @param py Pixel y coordinate
 * @return true if the pixel falls inside the scrollbar, false otherwise
 */
bool scene_click_scrollbar(int px, int py);

/**
 * @brief Converts a pixel coordinate to a file tree entry index
 * @param px Pixel x coordinate
 * @param py Pixel y coordinate
 * @param out_index Output entry index, or -1 if below all entries
 * @return true if the pixel falls inside the file tree. False otherwise
 */
bool scene_px_to_filetree(int px, int py, int *out_index);


/**
 * @brief Sets the syntax highlighting language for the current file.
 * @param lang The language to use.
 */
void scene_set_language(SyntaxLanguage lang);
