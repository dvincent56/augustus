#include "tollhouse.h"

#include "building/building.h"
#include "building/tollhouse.h"
#include "game/resource.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "translation/translation.h"
#include "window/building/figures.h"

void window_building_draw_tollhouse(building_info_context *c)
{
    c->help_id = 0;
    window_building_play_sound(c, "wavs/forum.wav");

    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    text_draw_centered(translation_for(TR_BUILDING_TOLLHOUSE),
        c->x_offset, c->y_offset + 12, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK, 0);

    building *b = building_get(c->building_id);
    int text_width = BLOCK_SIZE * (c->width_blocks - 4);
    int x_text = c->x_offset + 32;

    int y_status = c->y_offset + 56;

    if (!c->has_road_access) {
        window_building_draw_description_at(c, 56, 69, 25);
    } else if (b->num_workers <= 0) {
        text_draw_multiline(translation_for(TR_BUILDING_TOLLHOUSE_NO_EMPLOYEES),
            x_text, y_status, text_width, 0, FONT_NORMAL_BLACK, 0);
    } else if (!building_tollhouse_is_functional(b)) {
        text_draw_multiline(translation_for(TR_BUILDING_TOLLHOUSE_NO_RESOURCES),
            x_text, y_status, text_width, 0, FONT_NORMAL_BLACK, 0);
    } else {
        text_draw_multiline(translation_for(TR_BUILDING_TOLLHOUSE_FUNCTIONAL),
            x_text, y_status, text_width, 0, FONT_NORMAL_BLACK, 0);
    }

    int y_stocks = y_status + 70;
    int need = building_tollhouse_monthly_need();

    text_draw_label_and_number(translation_for(TR_BUILDING_TOLLHOUSE_MONTHLY_NEED),
        need, " ", x_text, y_stocks, FONT_NORMAL_BLACK, 0);

    int stone_units = b->resources[RESOURCE_STONE] / 100;
    int sand_units = b->resources[RESOURCE_SAND] / 100;

    text_draw_label_and_number(translation_for(TR_BUILDING_TOLLHOUSE_STONE_STOCK),
        stone_units, " ", x_text, y_stocks + 24, FONT_NORMAL_BLACK, 0);
    text_draw_label_and_number(translation_for(TR_BUILDING_TOLLHOUSE_SAND_STOCK),
        sand_units, " ", x_text, y_stocks + 48, FONT_NORMAL_BLACK, 0);

    text_draw_multiline(translation_for(TR_BUILDING_TOLLHOUSE_DESC),
        x_text, y_stocks + 80, text_width, 0, FONT_NORMAL_BLACK, 0);

    inner_panel_draw(c->x_offset + 16, c->y_offset + BLOCK_SIZE * c->height_blocks - 80,
        c->width_blocks - 2, 4);
    window_building_draw_employment(c, BLOCK_SIZE * c->height_blocks - 76);
}
