#include "weather.h"

#include "core/dir.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "scenario/property.h"
#include "sound/speech.h"
#include "time.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    int x, y;
    int length;
    int speed;
} Drop;

typedef struct {
    int active;        // 1 = enable, 0 = disable
    int intensity;     // number of drops
    int dx, dy;        // direction of the rain
} RainConfig;

RainConfig rain_config = {
    .active = 0,
    .intensity = 200,
    .dx = 1,
    .dy = 5,
};

Drop *drops = NULL;
int rain_initialized = 0;
int lightning_timer = 0;
int lightning_cooldown = 0;
float wind_angle = 0.0f;
float wind_speed = 0.05f;

void init_drop(Drop *drop) {
    drop->x = rand() % screen_width();
    drop->y = rand() % screen_height();
    drop->length = 10 + rand() % 10;
    drop->speed = 4 + rand() % 5;
}

void rain_stop(void) {
    rain_config.active = 0;
    free(drops);
    drops = NULL;
    rain_initialized = 0;
}

static void update_lightning(void) 
{
    if (lightning_cooldown <= 0 && (rand() % 500) == 0) {
        lightning_timer = 5 + rand() % 5; // flash duration
        lightning_cooldown = 300 + rand() % 400; // gap between flashes
    } else if (lightning_cooldown > 0) {
        lightning_cooldown--;
    }
    
    if (lightning_timer > 0) {
        graphics_fill_rect(0, 0, screen_width(), screen_height(), COLOR_WHITE);
        lightning_timer--;
    }

    if (lightning_timer == 5) {        
        char thunder_path[128];
        int thunder_num = 1 + rand() % 2; // 1 ou 2
        snprintf(thunder_path, sizeof(thunder_path), ASSETS_DIRECTORY "/Sounds/Thunder%d.mp3", thunder_num);
        sound_speech_play_file(thunder_path);
    }
}

static void update_wind(void) 
{
    wind_angle += wind_speed;
    rain_config.dx = (int)(2 * sinf(wind_angle)); // between -2 et 2
}

void rain_draw() {
    if (!rain_config.active) {
        rain_stop();
        return;
    }
    
    if (!rain_initialized) {
        srand(time(NULL));
        drops = malloc(sizeof(Drop) * rain_config.intensity);
        for (int i = 0; i < rain_config.intensity; ++i)
            init_drop(&drops[i]);
        rain_initialized = 1;
    }

    if (rain_config.intensity < 500) {
        update_wind();
    }

    if (rain_config.intensity > 800) {
       int raw_darkness = rain_config.intensity - 800;
        if (raw_darkness > 200) raw_darkness = 200;
        int darkness = (raw_darkness * raw_darkness) / 4000;  // max = 10
        if (darkness > 10) darkness = 10;
        graphics_shade_rect(0, 0, screen_width(), screen_height(), darkness);
        update_lightning();
    }

    int wind_strength = abs(rain_config.dx);
    int base_speed = 3 + wind_strength + (rain_config.intensity / 300);

    for (int i = 0; i < rain_config.intensity; ++i) {
        graphics_draw_line(
            (drops[i].x), 
            (drops[i].x + rain_config.dx * 2),
            (drops[i].y),
            ((drops[i].y + drops[i].length)),
            0xccffffff);

        drops[i].x += rain_config.dx;
        drops[i].y += base_speed + drops[i].speed;

        if (drops[i].y  > screen_height() || drops[i].x > screen_width()) {
            init_drop(&drops[i]);
            drops[i].y = 0;
        }
    }
}

void set_weather(int active, int intensity) {
    rain_stop();
    rain_config.active = active;
    rain_config.intensity = intensity;

    /*if (active) {
        char rain_path[128];
        int rain_number = 1;
        if (intensity > 800) {
            rain_number = 3;
        } else if (intensity > 500) {
            rain_number = 2;
        }
        snprintf(rain_path, sizeof(rain_path), ASSETS_DIRECTORY "/Sounds/Rain%d.mp3", rain_number);
        sound_speech_play_file(rain_path);
    }*/
}

void city_weather_update(void) {
    if (scenario_property_climate() == CLIMATE_DESERT) {
        set_weather(0, 0);
    } else {
        int active = (rand() % 4 == 0) ? 1 : 0;
        int intensity = active ? rand() % 1001 : 0;
        set_weather(active, intensity);
    }   
}

