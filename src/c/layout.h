#pragma once
#include <pebble.h>

#define MINIVIEW_BORDER_SIZE 6

#define TOP_SECTION_HEIGHT_RATIO 0.40f

#define MID_SECTION_HEIGHT_RATIO 0.40f
#define BOTTOM_SECTION_HEIGHT_RATIO 0.20f

#define SECTION_MARGIN 2
#define GRAPH_PLOT_MARGIN 10

#define TIME_FONT_HEIGHT 50
#define SMALL_FONT_HEIGHT 24
#define TINY_FONT_HEIGHT 22
#define SECONDS_LAYER_WIDTH 28

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
    GRect seconds_layer_bounds;

    GRect bottom_left_bounds;
    GRect bottom_right_bounds;
    
} Layout;

void layout_init(Layout *layout, GRect bounds);