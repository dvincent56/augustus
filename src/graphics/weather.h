#ifndef GRAPHICS_WEATHER_H
#define GRAPHICS_WEATHER_H

typedef enum {
    WEATHER_NONE,
    WEATHER_RAIN,
    WEATHER_SNOW
} WeatherType;

void weather_stop(void);
void weather_draw(void);
void city_weather_update(int month);

#endif // GRAPHICS_WEATHER_H
