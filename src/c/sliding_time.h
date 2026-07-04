#pragma once
#include <pebble.h>

typedef struct {
    TextLayer *current_layer;
    TextLayer *next_layer;
    PropertyAnimation *current_animation;
    PropertyAnimation *next_animation;
    bool current_layer_is_primary;
} Line;
