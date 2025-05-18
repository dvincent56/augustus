#include "weather.h"

#include "core/config.h"
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
} drop;

typedef struct {
    int x, y;
    float speed;
    float drift_offset;
    int drift_direction; // +1 or -1
} snowflake;

typedef struct {
    float x, y;
    float speed;
    float offset;
} sand_particle;

typedef struct {
    int active;        // 1 = enable, 0 = disable
    int intensity;     // number of drops/flakes
    int dx, dy;        // direction
    weather_type type;  // rain or snow
} weatherConfig;

weatherConfig weather_config = {
    .active = 0,
    .intensity = 200,
    .dx = 1,
    .dy = 5,
    .type = WEATHER_RAIN
};

drop *drops = NULL;
snowflake *flakes = NULL;
sand_particle *sand_particles = NULL;

int weather_initialized = 0;
int lightning_timer = 0;
int lightning_cooldown = 0;
float wind_angle = 0.0f;
float wind_speed = 0.05f;

void init_drop(drop *drop) {
    drop->x = rand() % screen_width();
    drop->y = rand() % screen_height();
    drop->length = 10 + rand() % 10;
    drop->speed = 4 + rand() % 5;
}

void init_snowflake(snowflake *flake) {
    flake->x = rand() % screen_width();
    flake->y = rand() % screen_height();
    flake->drift_offset = (float)(rand() % 100);
    flake->speed = 1 + rand() % 2;
    flake->drift_direction = (rand() % 2 == 0) ? 1 : -1;
}

void init_sand_particle(sand_particle *sand) {
    sand->x = rand() % screen_width();
    sand->y = rand() % screen_height();
    sand->speed = 1.5f + (rand() % 100) / 50.0f;
    sand->offset = rand() % 1000;
}

void weather_stop(void) {
    weather_config.active = 0;
    if (drops) {
        free(drops);
        drops = NULL;
    }
    if (flakes) {
        free(flakes);
        flakes = NULL;
    }
    if (sand_particles) {
        free(sand_particles);
        sand_particles = NULL;
    }
    weather_initialized = 0;
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
    weather_config.dx = (int)(2 * sinf(wind_angle)); // between -2 et 2
}

void weather_draw() {
    if (!config_get(CONFIG_UI_DRAW_WEATHER) || !weather_config.active) {
        weather_stop();
        return;
    }
    
    if (!weather_initialized) {
        srand(time(NULL));

        if (weather_config.type == WEATHER_RAIN) {
            drops = malloc(sizeof(drop) * weather_config.intensity);
            for (int i = 0; i < weather_config.intensity; ++i)
                init_drop(&drops[i]);
        } else if (weather_config.type == WEATHER_SNOW) {
            flakes = malloc(sizeof(snowflake) * weather_config.intensity);
            for (int i = 0; i < weather_config.intensity; ++i) {
                init_snowflake(&flakes[i]);                
            }
        } else if (weather_config.type == WEATHER_SAND) {
            sand_particles = malloc(sizeof(sand_particle) * weather_config.intensity);
            for (int i = 0; i < weather_config.intensity; ++i)
                init_sand_particle(&sand_particles[i]);        
        }
        weather_initialized = 1;
    }

    // snowflakes
    if (weather_config.type == WEATHER_SNOW) {
        for (int i = 0; i < weather_config.intensity; ++i) {
            float drift = sinf((flakes[i].y + flakes[i].drift_offset) * 0.02f);
            flakes[i].x += drift * 0.5f * flakes[i].drift_direction;
            flakes[i].y += (int)(flakes[i].speed);

            graphics_draw_line(flakes[i].x, flakes[i].x + 1, flakes[i].y, flakes[i].y, 0x88FFFFFF);
            graphics_draw_line(flakes[i].x, flakes[i].x, flakes[i].y, flakes[i].y + 1, 0x88FFFFFF);
            
            if (flakes[i].y >= screen_height() || flakes[i].x <= 0 || flakes[i].x >= screen_width()) {
                init_snowflake(&flakes[i]);
                flakes[i].y = 0;
            }
        }

        graphics_fill_rect(0, 0, screen_width(), screen_height(), 0x33DDEEFF);
        
        return;
    }

    if (weather_config.type == WEATHER_SAND) {
        for (int i = 0; i < weather_config.intensity; ++i) {
            float wave = sinf((sand_particles[i].y + sand_particles[i].offset) * 0.03f);
            sand_particles[i].x += sand_particles[i].speed + wave;

            graphics_draw_line(
                (int)sand_particles[i].x,
                (int)sand_particles[i].x + 1,
                (int)sand_particles[i].y,
                (int)sand_particles[i].y + 1,
                0xAAE1C699);

            if (sand_particles[i].x > screen_width()) {
                init_sand_particle(&sand_particles[i]);
                sand_particles[i].x = 0;
            }
        }
        
        graphics_fill_rect(0, 0, screen_width(), screen_height(), 0x44E1C699);

        return;
    }   

    // Rain
    if (weather_config.intensity < 500) {
        update_wind();
    }

    if (weather_config.intensity > 800) {
        int raw_darkness = weather_config.intensity - 800;
        if (raw_darkness > 200) raw_darkness = 200;
        int darkness = (raw_darkness * raw_darkness) / 4000;  // max = 10
        if (darkness > 10) darkness = 10;
        graphics_shade_rect(0, 0, screen_width(), screen_height(), darkness);
        update_lightning();
    }

    int wind_strength = abs(weather_config.dx);
    int base_speed = 3 + wind_strength + (weather_config.intensity / 300);

    for (int i = 0; i < weather_config.intensity; ++i) {
        graphics_draw_line(
            (drops[i].x), 
            (drops[i].x + weather_config.dx * 2),
            (drops[i].y),
            ((drops[i].y + drops[i].length)),
            0xccffffff);

        drops[i].x += weather_config.dx;
        drops[i].y += base_speed + drops[i].speed;

        if (drops[i].y >= screen_height() || drops[i].x <= 0 || drops[i].x >= screen_width()) {
            init_drop(&drops[i]);
            drops[i].y = 0;
        }
    }
}

void set_weather(int active, int intensity, weather_type type) {
    weather_stop();
    weather_config.active = active;
    weather_config.intensity = intensity;
    weather_config.type = type;
}

void city_weather_update(int month) {
    if (scenario_property_climate() == CLIMATE_DESERT) {
        int active = (rand() % 8 == 0);
        set_weather(active, 5000, WEATHER_SAND);
    } else {
        int active = (rand() % 4 == 0);
        int intensity = active ? rand() % 1001 : 0;
        weather_type type = WEATHER_RAIN;

        if (month == 10 || month == 11 || month == 0 || month == 1 || month == 2) {
            type = (rand() % 2 == 0) ? WEATHER_RAIN : WEATHER_SNOW;
        }
        
        set_weather(active, intensity, type);
    }
}

