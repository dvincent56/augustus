#include "epithets.h"

#include "assets/assets.h"
#include "city/constants.h"
#include "city/gods.h"
#include "core/image_group.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/scrollbar.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "window/advisors.h"
#include "window/message_dialog.h"
#include "window/option_popup.h"

#define MAX_EPITHETS_VISIBLE 1

static void button_god(int god, int param2);
static void button_close(int param1, int param2);
static void on_scroll(void);


static struct {
    option_menu_item option;
    const char image_id[32];
} epithets_options[18] = {
    {
        { TR_BUILDING_GRAND_TEMPLE_CERES_DESC, TR_BUILDING_GRAND_TEMPLE_CERES_BONUS_DESC },
        "Ceres M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_CERES_DESC_MODULE_1, TR_BUILDING_GRAND_TEMPLE_CERES_MODULE_1_DESC },
        "Ceres M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_CERES_DESC_MODULE_2, TR_BUILDING_GRAND_TEMPLE_CERES_MODULE_2_DESC },
        "Ceres M2 Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_NEPTUNE_DESC, TR_BUILDING_GRAND_TEMPLE_NEPTUNE_BONUS_DESC },
        "Nept M2 Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_NEPTUNE_DESC_MODULE_1, TR_BUILDING_GRAND_TEMPLE_NEPTUNE_MODULE_1_DESC },
        "Nept M2 Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_NEPTUNE_DESC_MODULE_2, TR_BUILDING_GRAND_TEMPLE_NEPTUNE_MODULE_2_DESC },
        "Nept M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_MERCURY_DESC, TR_BUILDING_GRAND_TEMPLE_MERCURY_BONUS_DESC },
        "Merc M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_MERCURY_DESC_MODULE_1, TR_BUILDING_GRAND_TEMPLE_MERCURY_MODULE_1_DESC },
        "Merc M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_MERCURY_DESC_MODULE_2, TR_BUILDING_GRAND_TEMPLE_MERCURY_MODULE_2_DESC },
        "Merc M2 Icon"
    }, 
    {
        { TR_BUILDING_GRAND_TEMPLE_MARS_DESC, TR_BUILDING_GRAND_TEMPLE_MARS_BONUS_DESC },
        "Mars M2 Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_MARS_DESC_MODULE_1, TR_BUILDING_GRAND_TEMPLE_MARS_MODULE_1_DESC },
        "Mars M2 Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_MARS_DESC_MODULE_2, TR_BUILDING_GRAND_TEMPLE_MARS_MODULE_2_DESC },
        "Mars M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_VENUS_DESC, TR_BUILDING_GRAND_TEMPLE_VENUS_BONUS_DESC },
        "Venus M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_VENUS_DESC_MODULE_1, TR_BUILDING_GRAND_TEMPLE_VENUS_MODULE_1_DESC },
        "Venus M Icon"
    },
    {
        { TR_BUILDING_GRAND_TEMPLE_VENUS_DESC_MODULE_2, TR_BUILDING_GRAND_TEMPLE_VENUS_MODULE_2_DESC },
        "Venus M2 Icon"
    },
    {
        { TR_BUILDING_PANTHEON_DESC, TR_BUILDING_PANTHEON_BONUS_DESC },
        "Panth M Icon"
    },
    {
        { TR_BUILDING_PANTHEON_DESC_MODULE_1, TR_BUILDING_PANTHEON_MODULE_1_DESC },
        "Panth M Icon"
    },
    {
        { TR_BUILDING_PANTHEON_DESC_MODULE_2, TR_BUILDING_PANTHEON_MODULE_2_DESC },
        "Panth M2 Icon"
    }
};

static int selected_god_id;

static image_button image_buttons_bottom[] = {
    {605, 345, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1}
};

static generic_button buttons_gods_size[] = {
    {30, 70, 80, 90, button_god, button_none, 0, 0},
    {130, 70, 80, 90, button_god, button_none, 1, 0},
    {230, 70, 80, 90, button_god, button_none, 2, 0},
    {330, 70, 80, 90, button_god, button_none, 3, 0},
    {430, 70, 80, 90, button_god, button_none, 4, 0},
    {530, 70, 80, 90, button_god, button_none, 5, 0},
    {630, 70, 80, 90, button_god, button_none, 6, 0}
};

static scrollbar_type scrollbar = { 1224, 496, 140, 540, MAX_EPITHETS_VISIBLE, on_scroll, 0, 4};

static int focus_button_id;
static int focus_image_button_id;

static void init(void)
{
    selected_god_id = 0;
    scrollbar_init(&scrollbar, 0, 3);
}

static void draw_background(void)
{
    window_advisors_draw_dialog_background();

    graphics_in_dialog();

    outer_panel_draw(0, 0, 40, 24);
    
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_ADVISOR_EPITHETS, 0, 15, 640, FONT_LARGE_BLACK);

    for (int god = 0; god < MAX_GODS + 1; god++) {
        if (god == selected_god_id) {
            button_border_draw(100 * god + 26, 66, 90, 100, 1);
            if (god == MAX_GODS) {
                image_draw(assets_get_image_id("UI", "Jupiter Portrait Selected"), 100 * god + 30, 70, COLOR_MASK_NONE, SCALE_NONE);
            } else {
                image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 21, 100 * god + 30, 70, COLOR_MASK_NONE, SCALE_NONE);
            }
        } else {
            if (god == MAX_GODS) {
                image_draw(assets_get_image_id("UI", "Jupiter Portrait Unselected"), 100 * god + 30, 70, COLOR_MASK_NONE, SCALE_NONE);
            } else {
                image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 16, 100 * god + 30, 70, COLOR_MASK_NONE, SCALE_NONE);
            }
        }
    }
    
    graphics_reset_dialog();
}


static void draw_foreground(void)
{
    graphics_in_dialog();

    inner_panel_draw(30, 196, 34, 9);

    int module_id = scrollbar.scroll_position;

    int module_name = epithets_options[selected_god_id * 3 + module_id].option.header;
    text_draw_centered(translation_for(module_name), 50, 215, 490, FONT_NORMAL_PLAIN, 0);
            
    int module_desc = epithets_options[selected_god_id * 3 + module_id].option.desc;
    text_draw_multiline(translation_for(module_desc), 50, 250, 490, FONT_NORMAL_WHITE, 0);

    image_buttons_draw(0, 0, image_buttons_bottom, 1);
    
    graphics_reset_dialog();

    scrollbar_draw(&scrollbar);
}

static void button_god(int god, int param2)
{
    selected_god_id = god;
    scrollbar_reset(&scrollbar, 0);
    window_invalidate();
}

static void button_close(int param1, int param2)
{
    window_advisors_show();
}

static void get_tooltip(tooltip_context *c)
{
     if (!focus_image_button_id && !focus_button_id) {
        return;
    }

    c->type = TOOLTIP_BUTTON;

    // image buttons
    if (focus_image_button_id) {
        c->text_id = 2;
    }
    // gods
    switch (focus_button_id) {
        case 1: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_CERES; break;
        case 2: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_NEPTUNE; break;
        case 3: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_MERCURY; break;
        case 4: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_MARS; break;
        case 5: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_VENUS; break;
        case 6: c->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP_JUPITER; break;
    }
}

static void on_scroll(void)
{
    window_request_refresh();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    int handled = 0;
    if (scrollbar_handle_mouse(&scrollbar, m, 1)) {
        focus_button_id = 0;
        handled = 1;
    }

    const mouse *m_dialog = mouse_in_dialog(m);

    if (handled == 0) {
        handled = image_buttons_handle_mouse(m_dialog, 0, 0, image_buttons_bottom, 1, &focus_image_button_id);
    }

    if (handled == 0) {
        handled = generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_gods_size, 6, &focus_button_id);
    }

    if (focus_image_button_id) {
        focus_button_id = 0;
    }

    if (!handled && input_go_back_requested(m, h)) {
        window_advisors_show();
    }
}

void window_epithets_show(void)
{
    window_type window = {
        WINDOW_EPITHETS,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    init();
    window_show(&window);
}
