#include "message_dialog.h"

#include "city/message.h"
#include "city/sentiment.h"
#include "city/view.h"
#include "core/image_group.h"
#include "core/lang.h"
#include "core/string.h"
#include "editor/editor.h"
#include "empire/city.h"
#include "figure/formation.h"
#include "game/settings.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/text.h"
#include "graphics/video.h"
#include "graphics/window.h"
#include "input/input.h"
#include "input/scroll.h"
#include "scenario/custom_messages.h"
#include "scenario/property.h"
#include "scenario/request.h"
#include "sound/device.h"
#include "sound/music.h"
#include "sound/speech.h"
#include "translation/translation.h"
#include "window/advisors.h"
#include "window/city.h"
#include "window/editor/map.h"

#define MAX_HISTORY 200
#define POPUP_PROTECTION_MILIS 400

static void draw_foreground_video(void);

static void button_back(int param1, int param2);
static void button_close(int param1, int param2);
static void button_help(int param1, int param2);
static void button_advisor(int advisor, int param2);
static void button_go_to_problem(int param1, int param2);

static image_button image_button_back = {
    0, 0, 31, 20, IB_NORMAL, GROUP_MESSAGE_ICON, 8, button_back, button_none, 0, 0, 1
};
static image_button image_button_close = {
    0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1
};
static image_button image_button_go_to_problem = {
    0, 0, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 52, button_go_to_problem, button_none, 1, 0, 1
};
static image_button image_button_help = {
    0, 0, 18, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 1, 0, 1
};
static image_button image_button_labor = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 0, button_advisor, button_none, ADVISOR_LABOR, 0, 1
};
static image_button image_button_trade = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor, button_none, ADVISOR_TRADE, 0, 1
};
static image_button image_button_population = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 15, button_advisor, button_none, ADVISOR_POPULATION, 0, 1
};
static image_button image_button_imperial = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 6, button_advisor, button_none, ADVISOR_IMPERIAL, 0, 1
};
static image_button image_button_military = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 3, button_advisor, button_none, ADVISOR_MILITARY, 0, 1
};
static image_button image_button_health = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 18, button_advisor, button_none, ADVISOR_HEALTH, 0, 1
};
static image_button image_button_entertainment = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 24, button_advisor, button_none, ADVISOR_ENTERTAINMENT, 0, 1
};
static image_button image_button_religion = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 27, button_advisor, button_none, ADVISOR_RELIGION, 0, 1
};
static image_button image_button_financial = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 30, button_advisor, button_none, ADVISOR_FINANCIAL, 0, 1
};
static image_button image_button_chief = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 33, button_advisor, button_none, ADVISOR_CHIEF, 0, 1
};

static struct {
    struct {
        int text_id;
        int scroll_position;
    } history[MAX_HISTORY];
    int num_history;

    int text_id;
    int is_custom_message;
    void (*background_callback)(void);
    int show_video;
    int should_play_audio;
    int should_play_speech;
    int should_play_background_music;
    int background_image_id;
    time_millis time_message_displayed;

    int x;
    int y;
    int x_text;
    int y_text;
    int text_height_blocks;
    int text_width_blocks;
    unsigned int focus_button_id;

    lang_message custom_lang_message;
    custom_message_t *custom_msg;
} data;

static struct {
    int year;
    int month;
    int param1;
    int param2;
    int message_advisor;
    int use_popup;
} player_message;

static void set_city_message(int year, int month,
    int param1, int param2,
    message_advisor advisor, int use_popup)
{
    player_message.year = year;
    player_message.month = month;
    player_message.param1 = param1;
    player_message.param2 = param2;
    player_message.message_advisor = advisor;
    player_message.use_popup = use_popup;

    if (use_popup) {
        data.time_message_displayed = time_get_millis();
    }
}

static void setup_custom_lang_message(int text_id)
{
    data.custom_lang_message.type = TYPE_MESSAGE;
    data.custom_lang_message.message_type = MESSAGE_TYPE_CUSTOM;
    data.custom_lang_message.x = 0;
    data.custom_lang_message.y = 64;
    data.custom_lang_message.width_blocks = 40;
    data.custom_lang_message.height_blocks = 30;
    data.custom_lang_message.title.x = 0;
    data.custom_lang_message.title.y = 0;
    data.custom_lang_message.subtitle.x = 16;
    data.custom_lang_message.subtitle.y = 48;
    data.custom_lang_message.urgent = 0;

    data.custom_msg = custom_messages_get(text_id);
    data.custom_lang_message.title.text = custom_messages_get_title(data.custom_msg);
    data.custom_lang_message.subtitle.text = custom_messages_get_subtitle(data.custom_msg);
    data.custom_lang_message.content.text = custom_messages_get_text(data.custom_msg);

    data.custom_lang_message.video.text = custom_messages_get_video(data.custom_msg);

    data.should_play_audio = 0;
    if (custom_messages_get_audio(data.custom_msg)) {
        data.should_play_audio = 1;
    }

    data.should_play_speech = 0;
    if (custom_messages_get_speech(data.custom_msg)) {
        data.should_play_speech = 1;
    }

    data.should_play_background_music = 0;
    if (custom_messages_get_background_music(data.custom_msg)) {
        data.should_play_background_music = 1;
    }

    const uint8_t *background_image_text = custom_messages_get_background_image(data.custom_msg);
    if (background_image_text) {
        data.background_image_id = rich_text_parse_image_id(&background_image_text, GROUP_INTERMEZZO_BACKGROUND, 1);
    }
}

static void clear_custom_lang_message(void)
{
    data.custom_msg = 0;
    data.custom_lang_message.title.text = 0;
    data.custom_lang_message.subtitle.text = 0;
    data.custom_lang_message.content.text = 0;
    data.custom_lang_message.video.text = 0;
    data.should_play_audio = 0;
    data.should_play_speech = 0;
    data.should_play_background_music = 0;
    data.background_image_id = 0;
}

static void fadeout_music(sound_type unused)
{
    sound_device_fadeout_music(5000);
    sound_device_on_audio_finished(0);
}

static void init_speech(sound_type type)
{
    if (type != SOUND_TYPE_SPEECH) {
        return;
    }
    int has_speech = data.should_play_speech && data.should_play_background_music;
    if (data.should_play_speech) {
        has_speech &= sound_device_play_file_on_channel(custom_messages_get_speech(data.custom_msg),
                SOUND_TYPE_SPEECH, setting_sound(SOUND_TYPE_SPEECH)->volume);
    }
    if (data.should_play_background_music) {
        int volume = 100;
        if (has_speech) {
            volume = setting_sound(SOUND_TYPE_SPEECH)->volume / 3;
        }
        if (volume > setting_sound(SOUND_TYPE_MUSIC)->volume) {
            volume = setting_sound(SOUND_TYPE_MUSIC)->volume;
        }
        has_speech &= sound_device_play_music(custom_messages_get_background_music(data.custom_msg), volume, 0);
    }
    sound_device_on_audio_finished(has_speech ? fadeout_music : 0);
}

static void init_audio(void)
{
    if (!data.show_video) {
        int playing_audio = 0;
        if (data.should_play_audio) {
            playing_audio = sound_device_play_file_on_channel(custom_messages_get_audio(data.custom_msg),
                SOUND_TYPE_SPEECH, setting_sound(SOUND_TYPE_SPEECH)->volume);
        }
        if (data.should_play_speech) {
            if (!playing_audio) {
                init_speech(SOUND_TYPE_SPEECH);
            } else {
                sound_device_on_audio_finished(init_speech);
            }
        } else if (data.should_play_background_music) {
            sound_device_play_music(custom_messages_get_background_music(data.custom_msg),
                setting_sound(SOUND_TYPE_MUSIC)->volume, 0);
        }
    }
}

static void custom_message_stop_video_and_show_message(void)
{
    if (data.show_video) {
        video_stop();
        data.show_video = 0;
        init_audio();
    }
    window_request_refresh();
}

static const lang_message *get_custom_or_standard_lang_message(int text_id)
{
    if (!data.is_custom_message) {
        return lang_get_message(text_id);
    } else {
        return &data.custom_lang_message;
    }
}

static void init(int text_id, int is_custom_message, void (*background_callback)(void))
{
    scroll_drag_end();
    for (int i = 0; i < MAX_HISTORY; i++) {
        data.history[i].text_id = 0;
        data.history[i].scroll_position = 0;
    }
    data.num_history = 0;
    rich_text_reset(0);
    data.text_id = text_id;
    clear_custom_lang_message();
    data.is_custom_message = is_custom_message;
    if (data.is_custom_message) {
        setup_custom_lang_message(text_id);
    }
    data.background_callback = background_callback;
    const lang_message *msg = get_custom_or_standard_lang_message(text_id);
    if (player_message.use_popup != 1) {
        data.show_video = 0;
    } else if (msg->video.text && video_start((char *) msg->video.text)) {
        data.show_video = 1;
    } else {
        data.show_video = 0;
    }
    if (data.show_video) {
        int restart_music = !data.should_play_audio && !data.should_play_speech && !data.should_play_background_music;
        video_init(restart_music);
    }
    init_audio();
}

static int resource_image(int resource)
{
    return resource_get_data(resource)->image.icon;
}

static int is_event_message(const lang_message *msg)
{
    return msg->type == TYPE_MESSAGE &&
        (msg->message_type == MESSAGE_TYPE_DISASTER ||
        msg->message_type == MESSAGE_TYPE_INVASION ||
        msg->message_type == MESSAGE_TYPE_BUILDING_COMPLETION);
}

static void draw_city_message_text(const lang_message *msg)
{
    if (msg->message_type != MESSAGE_TYPE_TUTORIAL) {
        int width = lang_text_draw(25, player_message.month, data.x_text + 10, data.y_text + 6, FONT_NORMAL_WHITE);
        width += lang_text_draw_year(player_message.year,
            data.x_text + 12 + width, data.y_text + 6, FONT_NORMAL_WHITE);
        if (msg->message_type == MESSAGE_TYPE_DISASTER && player_message.param1) {
            if (data.text_id == MESSAGE_DIALOG_THEFT) {
                // param1 = denarii
                lang_text_draw_amount(8, 0, player_message.param1, data.x + 240, data.y_text + 6, FONT_NORMAL_WHITE);
            } else {
                // param1 = building type
                lang_text_draw(41, player_message.param1, data.x + 240, data.y_text + 6, FONT_NORMAL_WHITE);
            }
        } else {
            width += lang_text_draw(63, 5, data.x_text + width + 60, data.y_text + 6, FONT_NORMAL_WHITE);
            text_draw(scenario_player_name(), data.x_text + width + 60, data.y_text + 6, FONT_NORMAL_WHITE, 0);
        }
    }
    switch (msg->message_type) {
        case MESSAGE_TYPE_DISASTER:
        case MESSAGE_TYPE_INVASION:
            lang_text_draw(12, 1, data.x + 100, data.y_text + 44, FONT_NORMAL_WHITE);
            rich_text_draw(msg->content.text, data.x_text + 8, data.y_text + 86,
                BLOCK_SIZE * data.text_width_blocks, data.text_height_blocks - 1, 0);
            break;
        case MESSAGE_TYPE_BUILDING_COMPLETION:
            text_draw(translation_for(TR_BUTTON_GO_TO_SITE), data.x + 100, data.y_text + 44, FONT_NORMAL_WHITE, 0);
            rich_text_draw(msg->content.text, data.x_text + 8, data.y_text + 86,
                16 * data.text_width_blocks, data.text_height_blocks - 1, 0);
            break;

        case MESSAGE_TYPE_EMIGRATION:
        {
            low_mood_cause cause = player_message.param1;
            if (!cause) {
                cause = city_sentiment_low_mood_cause();
            }
            if (cause >= LOW_MOOD_CAUSE_NO_FOOD && cause <= LOW_MOOD_CAUSE_MANY_TENTS) {
                int max_width = BLOCK_SIZE * (data.text_width_blocks - 1) - 64;
                lang_text_draw_multiline(12, cause + 2,
                    data.x + 64, data.y_text + 44, max_width, FONT_NORMAL_WHITE);
            } else if (cause == LOW_MOOD_CAUSE_SQUALOR) {
                int max_width = BLOCK_SIZE * (data.text_width_blocks - 1) - 64;
                lang_text_draw_multiline(CUSTOM_TRANSLATION, TR_CITY_MESSAGE_SQUALOR,
                    data.x + 64, data.y_text + 44, max_width, FONT_NORMAL_WHITE);
            }
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 86, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
            break;
        }
        case MESSAGE_TYPE_TUTORIAL:
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 6, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
            break;

        case MESSAGE_TYPE_TRADE_CHANGE:
        {
            image_draw(resource_image(player_message.param2), data.x + 64, data.y_text + 40,
                COLOR_MASK_NONE, SCALE_NONE);
            empire_city *city = empire_city_get(player_message.param1);
            const uint8_t *city_name = empire_city_get_name(city);
            text_draw(city_name, data.x + 100, data.y_text + 44, FONT_NORMAL_WHITE, 0);
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 86, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
            break;
        }

        case MESSAGE_TYPE_PRICE_CHANGE:
            image_draw(resource_image(player_message.param2), data.x + 64, data.y_text + 40,
                COLOR_MASK_NONE, SCALE_NONE);
            text_draw_money(player_message.param1, data.x + 100, data.y_text + 44, FONT_NORMAL_WHITE);
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 86, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
            break;

        case MESSAGE_TYPE_ROUTE_PRICE_CHANGE:
        {
            empire_city *city = empire_city_get(player_message.param1);
            const uint8_t *city_name = empire_city_get_name(city);
            text_draw(city_name, data.x + 64, data.y_text + 44, FONT_NORMAL_WHITE, 0);
            text_draw_money(player_message.param2, data.x + 240, data.y_text + 44, FONT_NORMAL_WHITE);
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 86, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
        }
        break;

        case MESSAGE_TYPE_CUSTOM:
        {
            rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 56, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 4, 0);
        }
        break;

        default:
        {
            int lines = rich_text_draw(msg->content.text,
                data.x_text + 8, data.y_text + 56, BLOCK_SIZE * (data.text_width_blocks),
                data.text_height_blocks - 1, 0);
            if (msg->message_type == MESSAGE_TYPE_IMPERIAL) {
                const scenario_request *request = scenario_request_get(player_message.param1);
                int y_offset = data.y_text + 86 + lines * 16;
                int requested_amount = player_message.param2 ? player_message.param2 : request->amount.requested;
                text_draw_number(requested_amount, '@', " ", data.x_text + 8, y_offset, FONT_NORMAL_WHITE, 0);
                image_draw(resource_image(request->resource), data.x_text + 70, y_offset - 5,
                    COLOR_MASK_NONE, SCALE_NONE);
                text_draw(resource_get_data(request->resource)->text,
                    data.x_text + 100, y_offset, FONT_NORMAL_WHITE, COLOR_MASK_NONE);
                if (request->state == REQUEST_STATE_NORMAL || request->state == REQUEST_STATE_OVERDUE) {
                    int width = lang_text_draw_amount(8, 4, request->months_to_comply,
                        data.x_text + 200, y_offset, FONT_NORMAL_WHITE);
                    lang_text_draw(12, 2, data.x_text + 200 + width, y_offset, FONT_NORMAL_WHITE);
                }
            }
            break;
        }
    }
}

static int get_message_image_id(const lang_message *msg)
{
    if (!msg->image.id) {
        return 0;
    } else if (data.text_id == 0) {
        // message id = 0 ==> "about": fixed image position
        return image_group(GROUP_BIG_PEOPLE);
    } else {
        return image_group(GROUP_MESSAGE_IMAGES) + msg->image.id - 1;
    }
}

static void draw_title(const lang_message *msg)
{
    if (!msg->title.text) {
        return;
    }
    int image_id = get_message_image_id(msg);
    const image *img = image_id ? image_get(image_id) : 0;
    // title
    if (msg->message_type == MESSAGE_TYPE_TUTORIAL) {
        text_draw_centered(msg->title.text,
            data.x, data.y + msg->title.y, BLOCK_SIZE * msg->width_blocks, FONT_LARGE_BLACK, 0);
    } else {
        // Center title in the dialog but ensure it does not overlap with the
        // image: if the title is too long, it will start 8px from the image.
        int title_x_offset = img ? img->width + msg->image.x + 8 : 0;
        text_draw_centered(msg->title.text, data.x + title_x_offset, data.y + 14,
            BLOCK_SIZE * msg->width_blocks - 2 * title_x_offset, FONT_LARGE_BLACK, 0);
    }
    data.y_text = data.y + 48;

    // picture
    if (img) {
        int image_x = msg->image.x;
        int image_y = msg->image.y;
        image_draw(image_id, data.x + image_x, data.y + image_y, COLOR_MASK_NONE, SCALE_NONE);
        if (data.y + image_y + img->height + 8 > data.y_text) {
            data.y_text = data.y + image_y + img->height + 8;
        }
    }
}

static void draw_subtitle(const lang_message *msg)
{
    if (msg->subtitle.x && msg->subtitle.text) {
        int width = BLOCK_SIZE * (msg->width_blocks - 1) - msg->subtitle.x;
        int height = text_draw_multiline(msg->subtitle.text,
            data.x + msg->subtitle.x, data.y + msg->subtitle.y, width, 0, FONT_NORMAL_BLACK, 0);
        if (data.y + msg->subtitle.y + height > data.y_text) {
            data.y_text = data.y + msg->subtitle.y + height;
        }
    }
}

static void draw_content(const lang_message *msg)
{
    if (!msg->content.text) {
        return;
    }
    if (msg->message_type == MESSAGE_TYPE_CUSTOM && !msg->title.text) {
        data.y_text += 128;
    }

    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_GREEN, FONT_NORMAL_RED, 5);
    int header_offset = msg->type == TYPE_MANUAL ? 48 : 32;
    data.text_height_blocks = msg->height_blocks - 1 - (header_offset + data.y_text - data.y) / BLOCK_SIZE;
    data.text_width_blocks = rich_text_init(msg->content.text,
        data.x_text, data.y_text, msg->width_blocks - 4, data.text_height_blocks, 1);

    // content!
    inner_panel_draw(data.x_text, data.y_text, data.text_width_blocks, data.text_height_blocks);
    rich_text_clear_links();

    if (msg->type == TYPE_MESSAGE) {
        draw_city_message_text(msg);
    } else {
        rich_text_draw(msg->content.text,
            data.x_text + 8, data.y_text + 6, BLOCK_SIZE * (data.text_width_blocks - 1),
            data.text_height_blocks - 1, 0);
    }
}

static void draw_background_normal(void)
{
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);
    data.x = msg->x;
    data.y = msg->y;
    data.x_text = data.x + 16;
    outer_panel_draw(data.x, data.y, msg->width_blocks, msg->height_blocks);

    draw_title(msg);
    draw_subtitle(msg);
    draw_content(msg);

    if (msg->type == TYPE_MANUAL && data.num_history > 0) {
        // Back button text
        lang_text_draw(12, 0,
            data.x + 52, data.y + BLOCK_SIZE * msg->height_blocks - 31, FONT_NORMAL_BLACK);
    }
}

static void draw_background_video(void)
{
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);
    data.x = 32;
    data.y = 28;

    int small_font = 0;
    int lines_required = 0;
    int lines_available = 4;
    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_IMPERIAL) {
        lines_available = 3;
    }
    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_GREEN, FONT_NORMAL_RED, 5);
    rich_text_clear_links();
    if (msg->message_type != MESSAGE_TYPE_CUSTOM) {
        lines_required = rich_text_draw(msg->content.text, 0, 0, 384, lines_available, 1);
        if (lines_required > lines_available) {
            small_font = 1;
            rich_text_set_fonts(FONT_SMALL_PLAIN, FONT_SMALL_PLAIN, FONT_SMALL_PLAIN, 5);
            lines_required = rich_text_draw(msg->content.text, 0, 0, 384, lines_available, 1);
        }
    }

    outer_panel_draw(data.x, data.y, 26, 28);
    graphics_fill_rect(data.x + 7, data.y + 7, 402, 294, COLOR_BLACK);

    int y_base = data.y + 308;
    int inner_height_blocks = 6;
    if (lines_required > lines_available) {
        // create space to cram an extra line into the dialog
        y_base = y_base - 8;
        inner_height_blocks += 1;
    }
    inner_panel_draw(data.x + 8, y_base, 25, inner_height_blocks);
    if (msg->message_type != MESSAGE_TYPE_CUSTOM) {
        text_draw_centered(msg->title.text,
            data.x + 8, data.y + 414, 400, FONT_NORMAL_BLACK, 0);
    }

    int width = lang_text_draw(25, player_message.month, data.x + 16, y_base + 4, FONT_NORMAL_WHITE);
    width += lang_text_draw_year(player_message.year, data.x + 18 + width, y_base + 4, FONT_NORMAL_WHITE);

    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_DISASTER &&
        data.text_id == MESSAGE_DIALOG_THEFT) {
        lang_text_draw_amount(8, 0, player_message.param1, data.x + 90 + width, y_base + 4, FONT_NORMAL_WHITE);
    } else {
        width += lang_text_draw(63, 5, data.x + 70 + width, y_base + 4, FONT_NORMAL_WHITE);
        text_draw(scenario_player_name(), data.x + 70 + width, y_base + 4, FONT_NORMAL_WHITE, 0);
    }

    data.text_height_blocks = msg->height_blocks - 1 - (32 + data.y_text - data.y) / BLOCK_SIZE;
    data.text_width_blocks = msg->width_blocks - 4;
    if (msg->message_type != MESSAGE_TYPE_CUSTOM) {
        if (small_font) {
            // Draw in black and then white to create shadow effect
           // rich_text_draw_colored(msg->content.text,
           //     data.x + 16 + 1, y_base + 24 + 1, 384, data.text_height_blocks - 1, COLOR_BLACK);
            rich_text_draw_colored(msg->content.text,
                data.x + 16, y_base + 24, 384, data.text_height_blocks - 1, 0); //COLOR_WHITE
        } else {
            rich_text_draw(msg->content.text, data.x + 16, y_base + 24, 384, data.text_height_blocks - 1, 0);
        }
    }

    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_IMPERIAL) {
        int y_text = data.y + 384;
        if (lines_required > lines_available) {
            y_text += 8;
        }
        const scenario_request *request = scenario_request_get(player_message.param1);
        width = text_draw_number(request->amount.requested, '@', " ", data.x + 8, y_text, FONT_NORMAL_WHITE, 0);
        image_draw(resource_get_data(request->resource)->image.icon,
            data.x + 15 + width, y_text - 5, COLOR_MASK_NONE, SCALE_NONE);
        width += text_draw(resource_get_data(request->resource)->text, data.x + 40 + width, y_text, FONT_NORMAL_WHITE, COLOR_MASK_NONE);
        if (request->state == REQUEST_STATE_NORMAL || request->state == REQUEST_STATE_OVERDUE) {
            width += lang_text_draw_amount(8, 4, request->months_to_comply, data.x + 60 + width, y_text, FONT_NORMAL_WHITE);
            width += lang_text_draw(12, 2, data.x + 60 + width, y_text, FONT_NORMAL_WHITE);
        }
    }

    draw_foreground_video();
}

static void draw_background(void)
{
    data.x_text = 0;
    data.y_text = 0;
    if (data.background_image_id && !editor_is_active()) {
        image_draw_fullscreen_background(data.background_image_id);
    } else if (data.background_callback) {
        data.background_callback();
    } else {
        window_draw_underlying_window();
    }
    graphics_in_dialog();
    if (data.show_video) {
        draw_background_video();
    } else {
        draw_background_normal();
    }
    graphics_reset_dialog();
}

static image_button *get_advisor_button(void)
{
    switch (player_message.message_advisor) {
        case MESSAGE_ADVISOR_LABOR:
            return &image_button_labor;
        case MESSAGE_ADVISOR_TRADE:
            return &image_button_trade;
        case MESSAGE_ADVISOR_POPULATION:
            return &image_button_population;
        case MESSAGE_ADVISOR_IMPERIAL:
            return &image_button_imperial;
        case MESSAGE_ADVISOR_MILITARY:
            return &image_button_military;
        case MESSAGE_ADVISOR_HEALTH:
            return &image_button_health;
        case MESSAGE_ADVISOR_RELIGION:
            return &image_button_religion;
        case MESSAGE_ADVISOR_CHIEF:
            return &image_button_chief;
        case MESSAGE_ADVISOR_ENTERTAINMENT:
            return &image_button_entertainment;
        case MESSAGE_ADVISOR_FINANCIAL:
            return &image_button_financial;
        default:
            return &image_button_help;
    }
}

static void draw_foreground_normal(void)
{
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);

    if (msg->type == TYPE_MANUAL && data.num_history > 0) {
        image_buttons_draw(
            data.x + 16, data.y + BLOCK_SIZE * msg->height_blocks - 36,
            &image_button_back, 1);
    }

    if (msg->type == TYPE_MESSAGE) {
        image_buttons_draw(data.x + 16, data.y + BLOCK_SIZE * msg->height_blocks - 40, get_advisor_button(), 1);
        if (is_event_message(msg)) {
            image_buttons_draw(data.x + 64, data.y_text + 36, &image_button_go_to_problem, 1);
        }
    }
    image_buttons_draw(data.x + BLOCK_SIZE * msg->width_blocks - 38, data.y + BLOCK_SIZE * msg->height_blocks - 36,
        &image_button_close, 1);
    rich_text_draw_scrollbar();
}

static void draw_foreground_video(void)
{
    video_draw(data.x + 8, data.y + 8, 400, 292);
    image_buttons_draw(data.x + 16, data.y + 408, get_advisor_button(), 1);
    image_buttons_draw(data.x + 372, data.y + 410, &image_button_close, 1);
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);
    if (is_event_message(msg)) {
        image_buttons_draw(data.x + 48, data.y + 407, &image_button_go_to_problem, 1);
    }
    if (msg->message_type == MESSAGE_TYPE_CUSTOM) {
        if (msg->title.text) {
            text_draw_centered(msg->title.text, data.x + 16, data.y + 336, 384, FONT_LARGE_BLACK, 0);
        }
        if (msg->subtitle.text) {
            text_draw_centered(msg->subtitle.text, data.x + 32, data.y + 368, 368, FONT_SMALL_PLAIN, 0);
        }
    }
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    if (data.show_video) {
        draw_foreground_video();
    } else {
        draw_foreground_normal();
    }
    graphics_reset_dialog();
}

static int handle_input_video(const mouse *m_dialog, const lang_message *msg, const hotkeys *h)
{
    if (image_buttons_handle_mouse(m_dialog, data.x + 16, data.y + 408, get_advisor_button(), 1, 0)) {
        return 1;
    }
    if (image_buttons_handle_mouse(m_dialog, data.x + 372, data.y + 410, &image_button_close, 1, 0)) {
        return 1;
    }
    if (is_event_message(msg)) {
        if (image_buttons_handle_mouse(m_dialog, data.x + 48, data.y + 407,
            &image_button_go_to_problem, 1,
            &data.focus_button_id)) {
            return 1;
        }
    }
    if (msg->message_type == MESSAGE_TYPE_CUSTOM && input_go_back_requested(m_dialog, h)) {
        custom_message_stop_video_and_show_message();
        return 1;
    }
    return 0;
}

static int handle_input_normal(const mouse *m_dialog, const lang_message *msg)
{
    if (rich_text_handle_mouse(m_dialog)) {
        return 1;
    }
    if (msg->type == TYPE_MANUAL && image_buttons_handle_mouse(
        m_dialog, data.x + 16, data.y + BLOCK_SIZE * msg->height_blocks - 36, &image_button_back, 1, 0)) {
        return 1;
    }
    if (msg->type == TYPE_MESSAGE) {
        if (image_buttons_handle_mouse(
            m_dialog, data.x + 16, data.y + BLOCK_SIZE * msg->height_blocks - 40, get_advisor_button(), 1, 0)) {
            return 1;
        }
        if (is_event_message(msg)) {
            if (image_buttons_handle_mouse(m_dialog, data.x + 64, data.y_text + 36,
                &image_button_go_to_problem, 1, 0)) {
                return 1;
            }
        }
    }

    if (image_buttons_handle_mouse(m_dialog,
        data.x + BLOCK_SIZE * msg->width_blocks - 38,
        data.y + BLOCK_SIZE * msg->height_blocks - 36,
        &image_button_close, 1, 0)) {
        return 1;
    }
    int text_id = rich_text_get_clicked_link(m_dialog);
    if (text_id >= 0) {
        if (data.num_history < MAX_HISTORY - 1) {
            data.history[data.num_history].text_id = data.text_id;
            data.history[data.num_history].scroll_position = rich_text_scroll_position();
            data.num_history++;
        }
        data.text_id = text_id;
        rich_text_reset(0);
        window_invalidate();
        return 1;
    }
    return 0;
}

static int can_skip_popup(void)
{
    if (time_get_millis() > (data.time_message_displayed + POPUP_PROTECTION_MILIS)) {
        return 1;
    }

    return 0;
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    data.focus_button_id = 0;
    const mouse *m_dialog = mouse_in_dialog(m);
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);
    int handled;
    if (data.show_video) {
        handled = handle_input_video(m_dialog, msg, h);
    } else {
        handled = handle_input_normal(m_dialog, msg);
    }
    if (!handled && input_go_back_requested(m, h) && can_skip_popup()) {
        button_close(0, 0);
    }


}

static void button_back(int param1, int param2)
{
    if (data.num_history > 0) {
        data.num_history--;
        data.text_id = data.history[data.num_history].text_id;
        rich_text_reset(data.history[data.num_history].scroll_position);
        window_invalidate();
    }
}

static void cleanup(void)
{
    if (data.show_video) {
        video_stop();
        data.show_video = 0;
    }
    player_message.message_advisor = 0;
    if (data.should_play_audio || data.should_play_speech) {
        sound_speech_stop();
    }
    if (data.should_play_background_music) {
        sound_music_stop();
    }
    sound_music_update(1);
}

static void button_close(int param1, int param2)
{
    if (data.is_custom_message && data.show_video) {
        custom_message_stop_video_and_show_message();
        return;
    }
    cleanup();
    window_go_back();
    window_invalidate();
}

static void button_help(int param1, int param2)
{
    button_close(0, 0);
    window_message_dialog_show(MESSAGE_DIALOG_HELP, data.background_callback);
}

static void button_advisor(int advisor, int param2)
{
    cleanup();
    if (!window_advisors_show_advisor(advisor)) {
        window_city_show();
    }
}

static void button_go_to_problem(int param1, int param2)
{
    cleanup();
    const lang_message *msg = get_custom_or_standard_lang_message(data.text_id);
    int grid_offset = player_message.param2;
    if (msg->message_type == MESSAGE_TYPE_INVASION) {
        int invasion_grid_offset = formation_grid_offset_for_invasion(player_message.param1);
        if (invasion_grid_offset > 0) {
            grid_offset = invasion_grid_offset;
        }
    }
    if (grid_offset > 0 && grid_offset < 26244) {
        city_view_go_to_grid_offset(grid_offset);
    }
    window_city_show();
}

static void get_tooltip(tooltip_context *c)
{
    if (data.focus_button_id) {
        c->type = TOOLTIP_BUTTON;
        c->text_group = 12;
        c->text_id = 1;
    }
}

static void init_window(int text_id, int is_custom_message, void (*background_callback)(void))
{
    window_type window = {
        WINDOW_MESSAGE_DIALOG,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    init(text_id, is_custom_message, background_callback);
    window_show(&window);
}

void window_message_dialog_show(int text_id, void (*background_callback)(void))
{
    init_window(text_id, 0, background_callback);
}

void window_message_dialog_show_city_message(int text_id, int year, int month,
    int param1, int param2, int advisor, int use_popup)
{
    set_city_message(year, month, param1, param2, advisor, use_popup);
    init_window(text_id, 0, window_city_draw_all);
}

void window_message_dialog_show_custom_message(int custom_message_id, int year, int month)
{
    set_city_message(year, month, custom_message_id, 0, MESSAGE_ADVISOR_NONE, 1);
    if (!editor_is_active()) {
        init_window(custom_message_id, 1, window_city_draw_all);
    } else {
        init_window(custom_message_id, 1, window_editor_map_draw_all);
    }
}
