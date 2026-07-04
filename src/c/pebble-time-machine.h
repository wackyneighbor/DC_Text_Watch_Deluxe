// from https://github.com/MorrisTimm/pebble-time-machine
#pragma once

#define TIME_MACHINE

typedef enum {
  TIME_MACHINE_SECONDS,
  TIME_MACHINE_MINUTES,
  TIME_MACHINE_HOURS,
  TIME_MACHINE_DAYS,
  TIME_MACHINE_WEEKS,
  TIME_MACHINE_MONTHS,
  TIME_MACHINE_YEARS
} TimeMachineUnit;

void time_machine_init(struct tm* start, TimeMachineUnit unit, int interval);
void time_machine_init_loop(struct tm* start, struct tm* end, TimeMachineUnit unit, int interval);
struct tm * time_machine_get_time();

void time_machine_tick_timer_service_subscribe(TimeUnits units, TickHandler handler);
void time_machine_tick_timer_service_unsubscribe();

typedef void(*TimeMachineTickHandler)(struct tm *tick_time, TimeUnits units_changed, void *context);
int time_machine_events_tick_timer_service_subscribe(TimeUnits units, TickHandler handler);
int time_machine_events_tick_timer_service_subscribe_context(TimeUnits units, TimeMachineTickHandler handler, void *context);
void time_machine_events_tick_timer_service_unsubscribe(int handle);