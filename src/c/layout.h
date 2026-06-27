#pragma once
#include <pebble.h>

#define DRAW_DEBUG_RECTANGLES 0


#define TOP_SECTION_HEIGHT_RATIO 0.40f

#define MID_SECTION_HEIGHT_RATIO 0.40f
#define BOTTOM_SECTION_HEIGHT_RATIO 0.20f


#if defined(PBL_PLATFORM_GABBRO)
  #define ROUND_SCREEN_INSET_X 30
  #define ROUND_SCREEN_INSET_Y 15
#else
  #define ROUND_SCREEN_INSET_X 20
  #define ROUND_SCREEN_INSET_Y 10
#endif

#define SECTION_MARGIN 2
#define GRAPH_PLOT_MARGIN 4

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)



  #define SMALL_FONT_HEIGHT  28
  #define SMALL_FONT_CAP_HEIGHT 20
  
  #define MEDIUM_FONT_HEIGHT 36
  #define MEDIUM_FONT_CAP_HEIGHT 28
  
  #define SMALL_FONT_BOTTOM_MARGIN 4
  #define MINIVIEW_ELEMENT_PADDING 4
  #define GRAPH_LINE_WIDTH 1
  #define MINIVIEW_DECORATION_WIDTH 2
  #define MINIVIEW_BORDER_SIZE 8
  #define SECONDS_FONT_HEIGHT 23
#else

  #define SMALL_FONT_HEIGHT  20
  #define SMALL_FONT_CAP_HEIGHT 14

  #define MEDIUM_FONT_HEIGHT 28
  #define MEDIUM_FONT_CAP_HEIGHT 20
  
  #define SMALL_FONT_BOTTOM_MARGIN 3
  #define MINIVIEW_ELEMENT_PADDING 2
  #define GRAPH_LINE_WIDTH 1
  #define MINIVIEW_DECORATION_WIDTH 1
  #define MINIVIEW_BORDER_SIZE 7
  #define SECONDS_FONT_HEIGHT 16
#endif



// Bebas Neue's built-in gap from the top of the cell to the cap height.
// Used to vertically center the miniview text stack within the inner circle.
#define MINIVIEW_FONT_CAP_TOP_MARGIN 11

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define TIME_GLYPH_SHEET_RESOURCE_ID RESOURCE_ID_SHEET_GLYPHS_66
#define TIME_BITMAP_SIZE GSize(440, 66)
#define BITMAP_GLYPH_WIDTH 40
#define BITMAP_GLYPH_HEIGHT 66
#define GLYPH_NUMERAL_MARGIN_X 2
#define GLYPH_COLON_MARGIN_X 16
#define GLYPH_SPACING_X 2
#define SECONDS_LAYER_WIDTH 20

#else
#define TIME_GLYPH_SHEET_RESOURCE_ID RESOURCE_ID_SHEET_GLYPHS_52
#define TIME_BITMAP_SIZE GSize(304, 52)
#define BITMAP_GLYPH_WIDTH 28
#define BITMAP_GLYPH_HEIGHT 52
#define GLYPH_NUMERAL_MARGIN_X 1
#define GLYPH_COLON_MARGIN_X 8
#define GLYPH_SPACING_X 2

#define SECONDS_LAYER_WIDTH 16
#endif

#define GLYPH_NUMERAL_DISPLAY_WIDTH (BITMAP_GLYPH_WIDTH - 2 * GLYPH_NUMERAL_MARGIN_X)
#define GLYPH_COLON_DISPLAY_WIDTH (BITMAP_GLYPH_WIDTH - 2 * GLYPH_COLON_MARGIN_X)


// ── Layout geometry ────────────────────────────────────────────────────────
//
// All bounds are derived from the display size plus inset constants above.
// Round screens inset the usable area; rectangular screens use the full area.

#ifdef PBL_ROUND
#  define LAYOUT_OX  ROUND_SCREEN_INSET_X
#  define LAYOUT_OY  ROUND_SCREEN_INSET_Y
#  define LAYOUT_W   (PBL_DISPLAY_WIDTH  - 2 * ROUND_SCREEN_INSET_X)
#  define LAYOUT_H   (PBL_DISPLAY_HEIGHT - 2 * ROUND_SCREEN_INSET_Y)
#else
#  define LAYOUT_OX  0
#  define LAYOUT_OY  0
#  define LAYOUT_W   PBL_DISPLAY_WIDTH
#  define LAYOUT_H   PBL_DISPLAY_HEIGHT
#endif

// Section heights derived from ratios
#define LAYOUT_TOP_H  ((int16_t)(LAYOUT_H * TOP_SECTION_HEIGHT_RATIO))
#define LAYOUT_MID_H  ((int16_t)(LAYOUT_H * MID_SECTION_HEIGHT_RATIO))
#define LAYOUT_BOT_H  ((int16_t)(LAYOUT_H * BOTTOM_SECTION_HEIGHT_RATIO))

// Section bounds (screen-absolute coordinates)
#define LAYOUT_TOP_SECTION  GRect(LAYOUT_OX, LAYOUT_OY,                              LAYOUT_W, LAYOUT_TOP_H)
#define LAYOUT_MID_SECTION  GRect(LAYOUT_OX, LAYOUT_OY + LAYOUT_TOP_H,               LAYOUT_W, LAYOUT_MID_H)
#define LAYOUT_BOT_SECTION  GRect(LAYOUT_OX, LAYOUT_OY + LAYOUT_TOP_H + LAYOUT_MID_H, LAYOUT_W, LAYOUT_BOT_H)

// Graph + miniview (top section, left/right split)
#define LAYOUT_MINIVIEW_SIZE  LAYOUT_TOP_H
#define LAYOUT_GRAPH_LAYER \
    grect_inset(GRect(LAYOUT_OX, LAYOUT_OY, LAYOUT_W - LAYOUT_MINIVIEW_SIZE, LAYOUT_MINIVIEW_SIZE), GEdgeInsets(SECTION_MARGIN))
#define LAYOUT_MINIVIEW \
    grect_inset(GRect(LAYOUT_OX + LAYOUT_W - LAYOUT_MINIVIEW_SIZE, LAYOUT_OY, LAYOUT_MINIVIEW_SIZE, LAYOUT_MINIVIEW_SIZE), GEdgeInsets(SECTION_MARGIN))

// Graph plot bounds — layer-local coordinates (origin 0,0 within the graph layer)
#define LAYOUT_GRAPH_PLOT_X   GRAPH_PLOT_MARGIN
#define LAYOUT_GRAPH_PLOT_Y   SMALL_FONT_HEIGHT
#define LAYOUT_GRAPH_PLOT_W   (LAYOUT_GRAPH_LAYER.size.w - GRAPH_PLOT_MARGIN * 2)
#define LAYOUT_GRAPH_PLOT_H   (LAYOUT_GRAPH_LAYER.size.h - SMALL_FONT_HEIGHT - GRAPH_PLOT_MARGIN)
#define LAYOUT_GRAPH_PLOT     GRect(LAYOUT_GRAPH_PLOT_X, LAYOUT_GRAPH_PLOT_Y, LAYOUT_GRAPH_PLOT_W, LAYOUT_GRAPH_PLOT_H)

// Miniview text bounds — layer-local coordinates (origin 0,0 within the miniview layer).
// The medium box starts MINIVIEW_BORDER_SIZE px before the small box ends (intentional
// overlap that accounts for the font's built-in cap-top gap).
// text_top is clamped to >= MINIVIEW_BORDER_SIZE.
#define LAYOUT_MINIVIEW_INNER_H      (LAYOUT_MINIVIEW.size.h - 2 * MINIVIEW_BORDER_SIZE)
#define LAYOUT_MINIVIEW_TEXT_SPAN    (SMALL_FONT_HEIGHT + MEDIUM_FONT_HEIGHT - MINIVIEW_BORDER_SIZE)
#define LAYOUT_MINIVIEW_TEXT_TOP_RAW \
    (MINIVIEW_BORDER_SIZE + (LAYOUT_MINIVIEW_INNER_H - LAYOUT_MINIVIEW_TEXT_SPAN) / 2 \
     - MINIVIEW_FONT_CAP_TOP_MARGIN / 2)
#define LAYOUT_MINIVIEW_TEXT_TOP \
    ((LAYOUT_MINIVIEW_TEXT_TOP_RAW) < MINIVIEW_BORDER_SIZE \
        ? MINIVIEW_BORDER_SIZE : (LAYOUT_MINIVIEW_TEXT_TOP_RAW))
#define LAYOUT_MINIVIEW_SMALL_TEXT \
    GRect(0, LAYOUT_MINIVIEW_TEXT_TOP, LAYOUT_MINIVIEW.size.w, SMALL_FONT_HEIGHT)
#define LAYOUT_MINIVIEW_MEDIUM_TEXT \
    GRect(0, LAYOUT_MINIVIEW_TEXT_TOP + SMALL_FONT_HEIGHT - MINIVIEW_BORDER_SIZE, \
          LAYOUT_MINIVIEW.size.w, MEDIUM_FONT_HEIGHT)

// Time layer (mid section, inset top/bottom only)
#define LAYOUT_TIME_LAYER \
    grect_inset(LAYOUT_MID_SECTION, GEdgeInsets(SECTION_MARGIN, 0))
#define LAYOUT_TIME_DISPLAY \
    GRect(LAYOUT_TIME_LAYER.origin.x, LAYOUT_TIME_LAYER.origin.y, \
          LAYOUT_TIME_LAYER.size.w - SECONDS_LAYER_WIDTH, BITMAP_GLYPH_HEIGHT)
#define LAYOUT_SECONDS \
    GRect(LAYOUT_TIME_LAYER.origin.x + LAYOUT_TIME_LAYER.size.w - SECONDS_LAYER_WIDTH, \
          LAYOUT_TIME_LAYER.origin.y, \
          SECONDS_LAYER_WIDTH, SMALL_FONT_HEIGHT)

// Bottom complication bounds — both widgets share the full-width rect
// so each can center itself relative to the other (cross-centering).
#define LAYOUT_BOTTOM_FULL   grect_inset(LAYOUT_BOT_SECTION, GEdgeInsets(SECTION_MARGIN))
#define LAYOUT_BOTTOM_LEFT   LAYOUT_BOTTOM_FULL
#define LAYOUT_BOTTOM_RIGHT  LAYOUT_BOTTOM_FULL