#include "layout.h"

void layout_init(Layout *layout, GRect bounds) {
#ifdef PBL_ROUND
    bounds = grect_inset(bounds, GEdgeInsets(ROUND_SCREEN_INSET_Y, ROUND_SCREEN_INSET_X));
#endif
    layout->bounds = bounds;

    int16_t ox = bounds.origin.x;
    int16_t oy = bounds.origin.y;
    int16_t w  = bounds.size.w;
    int16_t h  = bounds.size.h;

    layout->top_section_bounds    = GRect(ox, oy,                                              w, h * TOP_SECTION_HEIGHT_RATIO);
    layout->mid_section_bounds    = GRect(ox, oy + h * TOP_SECTION_HEIGHT_RATIO,               w, h * MID_SECTION_HEIGHT_RATIO);
    layout->bottom_section_bounds = GRect(ox, oy + h * (TOP_SECTION_HEIGHT_RATIO + MID_SECTION_HEIGHT_RATIO), w, h * BOTTOM_SECTION_HEIGHT_RATIO);

    // miniview is a square whose side matches the top section height
    int16_t miniview_size = layout->top_section_bounds.size.h;
    GRect graph_rect    = GRect(ox,                              oy, w - miniview_size, miniview_size);
    GRect miniview_rect = GRect(ox + w - miniview_size,          oy, miniview_size,     miniview_size);
    layout->graph_layer_bounds = grect_inset(graph_rect, GEdgeInsets(SECTION_MARGIN));
    // plot_bounds is in graph layer-local coordinates (origin 0,0)
    layout->graph_plot_bounds = GRect(
        GRAPH_PLOT_MARGIN,
        SMALL_FONT_HEIGHT,
        layout->graph_layer_bounds.size.w - GRAPH_PLOT_MARGIN * 2,
        layout->graph_layer_bounds.size.h - SMALL_FONT_HEIGHT - GRAPH_PLOT_MARGIN
    );

    layout->miniview_bounds = grect_inset(miniview_rect, GEdgeInsets(SECTION_MARGIN));
    // text bounds are in miniview layer-local coordinates (origin 0,0)
    layout->miniview_small_text_bounds  = GRect(0, MINIVIEW_BORDER_SIZE,
        layout->miniview_bounds.size.w, SMALL_FONT_HEIGHT);
    layout->miniview_medium_text_bounds = GRect(0, SMALL_FONT_HEIGHT,
        layout->miniview_bounds.size.w, MEDIUM_FONT_HEIGHT);

    layout->time_layer_bounds = grect_inset(layout->mid_section_bounds, GEdgeInsets(SECTION_MARGIN, 0));
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
        SMALL_FONT_HEIGHT
    );

    // Both bottom widgets share the full-width bounds so centering across both is unclipped
    GRect bottom_full_rect = grect_inset(layout->bottom_section_bounds, GEdgeInsets(SECTION_MARGIN));
    layout->bottom_left_bounds  = bottom_full_rect;
    layout->bottom_right_bounds = bottom_full_rect;
}
