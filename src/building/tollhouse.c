#include "tollhouse.h"

#include "building/building.h"
#include "building/distribution.h"
#include "city/buildings.h"
#include "game/resource.h"
#include "map/data.h"
#include "map/terrain.h"

#define MAX_DISTANCE 40

static int count_highway_tiles(void)
{
    int tiles = 0;
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_HIGHWAY)) {
                tiles++;
            }
        }
    }
    return tiles;
}

int building_tollhouse_monthly_need(void)
{
    int tiles = count_highway_tiles();
    if (tiles <= 0) {
        return 0;
    }
    return (tiles + TOLLHOUSE_HIGHWAY_TILES_PER_UNIT - 1) / TOLLHOUSE_HIGHWAY_TILES_PER_UNIT;
}

int building_tollhouse_is_functional(building *b)
{
    if (!b || b->state != BUILDING_STATE_IN_USE || b->num_workers <= 0) {
        return 0;
    }
    int need = building_tollhouse_monthly_need();
    if (need == 0) {
        return 1;
    }
    int need_internal = need * TOLLHOUSE_RESOURCE_PER_LOAD;
    if (b->resources[RESOURCE_STONE] < need_internal) {
        return 0;
    }
    if (b->resources[RESOURCE_SAND] < need_internal) {
        return 0;
    }
    return 1;
}

void building_tollhouse_consume_monthly(void)
{
    int id = city_buildings_get_tollhouse();
    if (!id) {
        return;
    }
    building *b = building_get(id);
    if (b->state != BUILDING_STATE_IN_USE) {
        return;
    }
    int need = building_tollhouse_monthly_need();
    if (need <= 0) {
        return;
    }
    int need_internal = need * TOLLHOUSE_RESOURCE_PER_LOAD;
    if (b->resources[RESOURCE_STONE] >= need_internal && b->resources[RESOURCE_SAND] >= need_internal) {
        b->resources[RESOURCE_STONE] -= need_internal;
        b->resources[RESOURCE_SAND] -= need_internal;
    }
}

int building_tollhouse_get_storage_destination(building *tollhouse)
{
    resource_storage_info info[RESOURCE_MAX] = { 0 };
    if (!building_distribution_get_handled_resources_for_building(tollhouse, info) ||
        !building_distribution_get_resource_storages_for_building(info, tollhouse, MAX_DISTANCE)) {
        return 0;
    }
    int fetch_inventory = building_distribution_fetch(tollhouse, info, 0, 1);
    if (fetch_inventory != RESOURCE_NONE) {
        tollhouse->data.market.fetch_inventory_id = fetch_inventory;
        return info[fetch_inventory].building_id;
    }
    fetch_inventory = building_distribution_fetch(tollhouse, info, BASELINE_STOCK, 0);
    if (fetch_inventory != RESOURCE_NONE) {
        tollhouse->data.market.fetch_inventory_id = fetch_inventory;
        return info[fetch_inventory].building_id;
    }
    return 0;
}
