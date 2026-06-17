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

typedef struct {
    GRect bounds;

    GRect top_section_bounds;
    GRect mid_section_bounds;
    GRect bottom_section_bounds;

    GRect graph_layer_bounds;
    GRect graph_plot_bounds;
    GRect miniview_bounds;
    GRect miniview_small_text_bounds;
    GRect miniview_medium_text_bounds;

    GRect time_layer_bounds;
    GRect time_display_bounds;
    GRect seconds_layer_bounds;

    GRect bottom_left_bounds;
    GRect bottom_right_bounds;
    
} Layout;

void layout_init(Layout *layout, GRect bounds);