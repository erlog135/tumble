#include "layout.h"

void layout_init(Layout *layout, GRect bounds) {
    layout->bounds = bounds;
    layout->top_section_bounds = GRect(0, 0, bounds.size.w, bounds.size.h * TOP_SECTION_HEIGHT_RATIO);
    layout->mid_section_bounds = GRect(0, bounds.size.h * TOP_SECTION_HEIGHT_RATIO, bounds.size.w, bounds.size.h * MID_SECTION_HEIGHT_RATIO);
    layout->bottom_section_bounds = GRect(0, bounds.size.h * (TOP_SECTION_HEIGHT_RATIO + MID_SECTION_HEIGHT_RATIO), bounds.size.w, bounds.size.h * BOTTOM_SECTION_HEIGHT_RATIO);

    // miniview is a square whose side matches the top section height
    int16_t miniview_size = layout->top_section_bounds.size.h;
    GRect graph_rect = GRect(0, 0, layout->top_section_bounds.size.w - miniview_size, layout->top_section_bounds.size.h);
    GRect miniview_rect = GRect(layout->top_section_bounds.size.w - miniview_size, 0, miniview_size, miniview_size);
    layout->graph_layer_bounds = grect_inset(graph_rect, GEdgeInsets(SECTION_MARGIN));
    layout->graph_plot_bounds = GRect(
        GRAPH_PLOT_MARGIN,
        TINY_FONT_HEIGHT,
        layout->graph_layer_bounds.size.w - GRAPH_PLOT_MARGIN * 2,
        layout->graph_layer_bounds.size.h - TINY_FONT_HEIGHT - GRAPH_PLOT_MARGIN
    );

    layout->miniview_bounds = grect_inset(miniview_rect, GEdgeInsets(SECTION_MARGIN));
    int16_t miniview_text_top = MINIVIEW_BORDER_SIZE;
    layout->miniview_tiny_text_bounds = GRect(0, miniview_text_top, layout->miniview_bounds.size.w, TINY_FONT_HEIGHT);
    layout->miniview_small_text_bounds = GRect(0, TINY_FONT_HEIGHT, layout->miniview_bounds.size.w, SMALL_FONT_HEIGHT);

    GRect time_layer_rect = GRect(layout->mid_section_bounds.origin.x, layout->mid_section_bounds.origin.y, layout->mid_section_bounds.size.w, layout->mid_section_bounds.size.h);
    layout->time_layer_bounds = grect_inset(time_layer_rect, GEdgeInsets(SECTION_MARGIN));
    layout->time_display_bounds = GRect(
        layout->time_layer_bounds.origin.x,
        layout->time_layer_bounds.origin.y,
        layout->time_layer_bounds.size.w - SECONDS_LAYER_WIDTH,
        BITMAP_GLYPH_HEIGHT
    );
    layout->seconds_layer_bounds = GRect(
        layout->time_layer_bounds.origin.x + layout->time_layer_bounds.size.w - SECONDS_LAYER_WIDTH,
        layout->time_layer_bounds.origin.y,
        SECONDS_LAYER_WIDTH,
        TINY_FONT_HEIGHT
    );

    // Compute and inset bottom left/right within bottom_section_bounds
    GRect bottom_left_rect = GRect(layout->bottom_section_bounds.origin.x, layout->bottom_section_bounds.origin.y, layout->bottom_section_bounds.size.w / 2, layout->bottom_section_bounds.size.h);
    GRect bottom_right_rect = GRect(layout->bottom_section_bounds.size.w / 2, layout->bottom_section_bounds.origin.y, layout->bottom_section_bounds.size.w / 2, layout->bottom_section_bounds.size.h);
    layout->bottom_left_bounds = grect_inset(bottom_left_rect, GEdgeInsets(SECTION_MARGIN));
    layout->bottom_right_bounds = grect_inset(bottom_right_rect, GEdgeInsets(SECTION_MARGIN));
}
