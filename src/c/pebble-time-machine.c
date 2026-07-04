#include <pebble.h>
#include "pebble-time-machine.h"

#define HANDLE 0xFA57

static TickHandler s_handler = NULL;
static TimeMachineTickHandler s_tick_handler = NULL;
static void* s_context = NULL;
static int s_interval = 0;
static time_t s_start;
static time_t s_end;
static time_t s_time;
static struct tm s_last_time;
static AppTimer* s_timer = NULL;
static TimeMachineUnit s_unit;
static TimeUnits s_units;

static int prv_number_of_days_in_month(int month) {
  switch(month) {
    case  1: return 28; // ignore leap years
    case  3:
    case  5:
    case  8:
    case 10: return 30;
    default: return 31;
  }
}

static void prv_timer_callback(void* tick) {
  static struct tm* tick_time;
  tick_time = localtime(&s_time);
  TimeUnits changed = 0;
  if(tick) {
    if(s_units & SECOND_UNIT) {
      changed |= SECOND_UNIT;
    }
    if(s_last_time.tm_min != tick_time->tm_min) {
      changed |= MINUTE_UNIT;
    }
    if(s_last_time.tm_hour != tick_time->tm_hour) {
      changed |= MINUTE_UNIT | HOUR_UNIT;
    }
    if(s_last_time.tm_mday != tick_time->tm_mday) {
      changed |= MINUTE_UNIT | HOUR_UNIT | DAY_UNIT;
    }
    if(s_last_time.tm_mon != tick_time->tm_mon) {
      changed |= MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT;
    }
    if(s_last_time.tm_year != tick_time->tm_year) {
      changed |= MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT;
    }
  }
  if(s_units & changed || 0 == changed) {
    if(s_handler) {
      s_handler(tick_time, changed);
    }
    if(s_tick_handler) {
      s_tick_handler(tick_time, changed, s_context);
    }
  }
  s_last_time = *tick_time;
  if(s_end && s_time >= s_end) {
    s_time = s_start;
  } else {
    switch(s_unit) {
      case TIME_MACHINE_SECONDS: ++s_time; break;
      case TIME_MACHINE_MINUTES: s_time += SECONDS_PER_MINUTE; break;
      case TIME_MACHINE_HOURS:   s_time += SECONDS_PER_HOUR; break;
      case TIME_MACHINE_DAYS:    s_time += SECONDS_PER_DAY; break;
      case TIME_MACHINE_WEEKS:   s_time += SECONDS_PER_DAY*7; break;
      case TIME_MACHINE_MONTHS:  s_time += SECONDS_PER_DAY*prv_number_of_days_in_month(tick_time->tm_mon); break;
      case TIME_MACHINE_YEARS:   s_time += SECONDS_PER_DAY*365; break; // ignore leap years
    }
  }
  if(s_interval) {
    s_timer = app_timer_register(s_interval, prv_timer_callback, (void*)1);
  }
}

void time_machine_init(struct tm* start, TimeMachineUnit unit, int interval) {
  if(NULL == start) {
    time_t now = time(NULL);
    start = localtime(&now);
  }
  s_start = s_time = mktime(start);
  s_end = 0;
  s_unit = unit;
  s_interval = interval;
}

void time_machine_init_loop(struct tm* start, struct tm* end, TimeMachineUnit unit, int interval) {
  time_machine_init(start, unit, interval);
  if(end) {
    s_end = mktime(end);
  }
}

struct tm * time_machine_get_time() {
  static struct tm external_time;
  external_time = s_last_time;
  return &external_time;
}

void time_machine_tick_timer_service_subscribe(TimeUnits units, TickHandler handler) {
  s_units = units;
  s_handler = handler;
  s_tick_handler = NULL;
  s_context = NULL;
  if(s_timer) {
    app_timer_cancel(s_timer);
  }
  s_timer = app_timer_register(10, prv_timer_callback, NULL);
}

void time_machine_tick_timer_service_unsubscribe() {
  if(s_timer) {
    app_timer_cancel(s_timer);
  }
}

int time_machine_events_tick_timer_service_subscribe(TimeUnits units, TickHandler handler) {
  time_machine_tick_timer_service_subscribe(units, handler);
  return HANDLE;
}

int time_machine_events_tick_timer_service_subscribe_context(TimeUnits units, TimeMachineTickHandler handler, void *context) {
  time_machine_tick_timer_service_subscribe(units, NULL);
  s_tick_handler = handler;
  s_context = context;
  return HANDLE;
}

void time_machine_events_tick_timer_service_unsubscribe(int handle) {
  time_machine_tick_timer_service_unsubscribe();
}