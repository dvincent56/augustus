#ifndef BUILDING_TOLLHOUSE_H
#define BUILDING_TOLLHOUSE_H

#include "building/building.h"

#define TOLLHOUSE_RESOURCE_PER_LOAD 100

int building_tollhouse_is_functional(building *b);

int building_tollhouse_monthly_need(void);

void building_tollhouse_consume_monthly(void);

int building_tollhouse_get_storage_destination(building *tollhouse);

#endif // BUILDING_TOLLHOUSE_H
