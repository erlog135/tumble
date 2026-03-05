#pragma once
#include <pebble.h>

#define DRAW_DEBUG_RECTANGLES 0

#define MINIVIEW_BORDER_SIZE 6

#define TOP_SECTION_HEIGHT_RATIO 0.40f

#define MID_SECTION_HEIGHT_RATIO 0.40f
#define BOTTOM_SECTION_HEIGHT_RATIO 0.20f

#define SECTION_MARGIN 2
#define GRAPH_PLOT_MARGIN 4

#define TIME_FONT_HEIGHT 50
#define SMALL_FONT_HEIGHT 24
#define TINY_FONT_HEIGHT 22
#define SECONDS_LAYER_WIDTH 18


#define TIME_GLYPH_SHEET_RESOURCE_ID RESOURCE_ID_SHEET_GLYPHS_52

#define TIME_BITMAP_SIZE GSize(304, 52)
#define BITMAP_GLYPH_WIDTH 28
#define BITMAP_GLYPH_HEIGHT 52
#define GLYPH_NUMERAL_MARGIN_X 1
#define GLYPH_COLON_MARGIN_X 8
#define GLYPH_SPACING_X 2

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
    GRect miniview_tiny_text_bounds;
    GRect miniview_small_text_bounds;

    GRect time_layer_bounds;
    GRect time_display_bounds;
    GRect seconds_layer_bounds;

    GRect bottom_left_bounds;
    GRect bottom_right_bounds;
    
} Layout;

void layout_init(Layout *layout, GRect bounds);