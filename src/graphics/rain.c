#include "rain.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "time.h"

#include <stdlib.h>
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
    .active = 1,
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
    drop->y = 0;
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
    if (rain_config.active == 1 && rain_config.intensity > 800) {
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

    update_wind();
    update_lightning();

    for (int i = 0; i < rain_config.intensity; ++i) {
        graphics_draw_line(
            (drops[i].x), 
            (drops[i].x + rain_config.dx * 2),
            (drops[i].y),
            ((drops[i].y + drops[i].length)),
            COLOR_WHITE);

        drops[i].x += rain_config.dx;
        drops[i].y += drops[i].speed + rain_config.dy;

        if (drops[i].y  > screen_height() || drops[i].x > screen_width()) {
            init_drop(&drops[i]);
        }
    }
}

void set_rain_intensity(int intensity) {
    if (intensity < 0) {
        intensity = 0;
    } else if (intensity > 1000) {
        intensity = 1000;
    }
    rain_config.intensity = intensity;
}

