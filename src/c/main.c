//need to add day of week to screenshot mode; now always sunday

#include <pebble.h>
#include "my_math.h"
#include "suncalc.h"
#include "num2words-en.h"
#include "sliding_time.h"
#include "demo.h"
#define BUFFER_SIZE 44

// For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
#include "pebble-time-machine.h"

// Uncomment to enable accelerated time
//#define ENABLE_TIME_MACHINE

// Select ONE mode below when enabled:
//#define TM_MODE_1_MINUTE_PER_2_SECONDS  // good for testing font sizes of minute display
//#define TM_MODE_24_MINUTES_PER_SECOND  // good for testing motion of perimeter time indicator
//#define TM_MODE_12_HOURS_PER_SECOND  // good for testing font sizes of month displays (can be clipped on round if you're not careful)

#ifdef ENABLE_TIME_MACHINE
static struct tm s_current_time;
    #ifdef TM_MODE_1_MINUTE_PER_2_SECONDS
        #define TM_UNIT TIME_MACHINE_MINUTES
        #define TM_INTERVAL_MS 2000
        #define TM_TICK_UNIT MINUTE_UNIT
    #endif
    #ifdef TM_MODE_24_MINUTES_PER_SECOND
        #define TM_UNIT TIME_MACHINE_MINUTES
        #define TM_INTERVAL_MS 42
        #define TM_TICK_UNIT MINUTE_UNIT
    #endif
    #ifdef TM_MODE_12_HOURS_PER_SECOND
        #define TM_UNIT TIME_MACHINE_HOURS
        #define TM_INTERVAL_MS 83
        #define TM_TICK_UNIT HOUR_UNIT
    #endif
#endif

// Uncomment next line to enable screenshot mode
//#define ENABLE_SCREENSHOT_MODE

#ifdef ENABLE_SCREENSHOT_MODE
bool s_screenshot_mode = true;
#else
bool s_screenshot_mode = false;
#endif

static struct tm s_screenshot_time = {
    .tm_year = 2026 - 1900,  // Update 1st number only, for desired year.  Used for Sunrise/Sunset calculation only (does not affect day of week for given date)
    .tm_mon  = 0,   // Note: 0 = January, 11 = December
    .tm_mday = 1,
    .tm_wday = 3,   // 0 = Sunday
    .tm_hour = 12,  // 0 = 12 midnight, 12 = 12 noon
    .tm_min  = 0,
    .tm_sec  = 0,
    .tm_isdst = 0
};

float  s_screenshot_lat_deg = 25.0000;
float  s_screenshot_lng_deg = 00.0000;
static int8_t s_screenshot_utc_offset = 0;
static int8_t s_screenshot_dst = 0;

// Forward declarations
static void main_window_unload(Window *window);
static void main_window_load(Window *window);
void setup_time_layers(bool animate);
static void click_config_provider(void *context);
void cancel_all_watch_animations(void);
int get_line_x_offset(int rect_top, int rect_bottom, int margin, bool align_right);
static bool next_minute_is_two_line(struct tm *t);
static void make_scroll_up(struct tm *t);

// Month and day string tables
// We'll use abbreviations for months on round watches since the bottom of screen is so narrow, but spell out completely on rectangular
#if defined(PBL_ROUND)
const char* const MONTHS[] = {
    "Jan ",
    "Feb ",
    "Mar ",
    "Apr ",
    "May ",
    "June ",
    "July ",
    "Aug ",
    "Sept ",
    "Oct ",
    "Nov ",
    "Dec ",
};
#else
const char* const MONTHS[] = {
    "January ",
    "February ",
    "March ",
    "April ",
    "May ",
    "June ",
    "July ",
    "August ",
    "September ",
    "October ",
    "November ",
    "Dececember ",
};
#endif

const char* const DAYS[] = {
    "",
    "1st",
    "2nd",
    "3rd",
    "4th",
    "5th",
    "6th",
    "7th",
    "8th",
    "9th",
    "10th",
    "11th",
    "12th",
    "13th",
    "14th",
    "15th",
    "16th",
    "17th",
    "18th",
    "19th",
    "20th",
    "21st",
    "22nd",
    "23rd",
    "24th",
    "25th",
    "26th",
    "27th",
    "28th",
    "29th",
    "30th",
    "31st",
};

// The three text lines for time display
Line s_line1;
Line s_line2;
Line s_line3;

GRect s_bounds;

char s_line1_str[2][BUFFER_SIZE];
char s_line2_str[2][BUFFER_SIZE];
char s_line3_str[2][BUFFER_SIZE];

Window *s_main_window;

Layer *s_back_layer;
Layer *s_scroll;

static PropertyAnimation *s_scroll_down = NULL;
static PropertyAnimation *s_scroll_up = NULL;

// State flags and config values
static bool s_no_gps;
static bool s_analog_elements;
static bool s_enlarge;
static bool s_custom_fonts_loaded = false;
bool s_demo_running = false;
int  s_time_align;
int  s_date_align;

// Display colors
GColor s_background_color;
static GColor s_text_line1_color;
static GColor s_text_line2_color;
static GColor s_text_line3_color;
GColor s_text_day_color;
GColor s_text_date_color;
GColor s_time_indicator_color;
GColor s_sunrise_indicator_color;
GColor s_sunset_indicator_color;

// Fonts and layout metrics
static GFont s_hour_font;
static GFont s_minute_font;
static GFont s_midnight_font;
GFont s_date_font;
static int s_time_line_height;
int s_time_line_spacing;
static int s_top_time_margin;
static int s_middle_time_offset;
int s_date_line_height;
int s_date_line_spacing;
int s_date_lower_margin;
static int s_time_side_margin;
int s_date_side_margin;
int s_time_indicator_radius;
int s_time_indicator_thickness;
int s_sun_indicator_radius;
static int s_rect_corner_radius;

// how long do the horizontal slide animations take?  (used for minute ticks and initial display)
const int s_horizontal_slide_duration = 400;

// GPS and location globals
float s_local_lat_deg;      // Last known latitude
float s_local_lng_deg;      // Last known longitude
uint16_t s_last_update_time;    // Last GPS update time
uint32_t s_last_update_date;    // Last GPS update date
static int8_t s_last_update_offset;   // Hours offset from UTC at last GPS update
static int8_t s_last_update_dst;      // DST active at last GPS update

// Javascript dictionary keys for GPS get & config updates
enum {
    KEY_LATITUDE = 1,
    KEY_LONGITUDE = 2,
    KEY_USEOLDDATA = 3,
    KEY_UTCH = 4,
    KEY_SINGLE_PREFIX_TYPE = 5,
    KEY_BACKGROUND_COLOR = 6,
    KEY_TEXT_LINE_1_COLOR = 7,
    KEY_TEXT_LINE_2_COLOR = 8,
    KEY_TEXT_LINE_3_COLOR = 9,
    KEY_TEXT_DAY_COLOR = 10,
    KEY_TEXT_DATE_COLOR = 11,
    KEY_TIME_INDICATOR_COLOR = 12,
    KEY_SUNRISE_INDICATOR_COLOR = 13,
    KEY_SUNSET_INDICATOR_COLOR = 14,
    KEY_NOGPS = 15,
    KEY_ENLARGE = 16,
    KEY_TIME_ALIGN = 17,
    KEY_DATE_ALIGN = 18,
    KEY_START_DEMO = 19,
    KEY_ANALOG_ELEMENTS = 20
};

// Persistent storage keys
enum {
    LAT_STORED = 0,
    LNG_STORED = 1,
    UTC_STORED = 2,
    LAST_DATE_STORED = 3,
    LAST_TIME_STORED = 4,
    DST_STORED = 5,
    SHOW_O_PREFIX_STORED = 6,
    SHOW_OH_PREFIX_STORED = 7,
    BACKGROUND_COLOR_STORED = 8,
    TEXT_LINE_1_COLOR_STORED = 9,
    TEXT_LINE_2_COLOR_STORED = 10,
    TEXT_LINE_3_COLOR_STORED = 11,
    TEXT_DAY_COLOR_STORED = 12,
    TEXT_DATE_COLOR_STORED = 13,
    TIME_INDICATOR_COLOR_STORED = 14,
    SUNRISE_INDICATOR_COLOR_STORED = 15,
    SUNSET_INDICATOR_COLOR_STORED = 16,
    NO_GPS_STORED = 17,
    ENLARGE_STORED = 18,
    TIME_ALIGN_STORED = 19,
    DATE_ALIGN_STORED = 20,
    ANALOG_ELEMENTS_STORED = 21
};

// abort demo mode if user clicks a button
static void demo_abort_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (s_demo_running) {
        demo_abort();
    }
}

// abort demo mode if user clicks a button
static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, demo_abort_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, demo_abort_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, demo_abort_click_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, demo_abort_click_handler);
}

// GPS update procedure
static void update_gps() {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
    // Send the message!
    app_message_outbox_send();
}

// helper routine added for "screenshot mode" implementation
static struct tm *get_display_time(void) {

    // Screenshot mode overrides everything
    if (s_screenshot_mode) {
        return &s_screenshot_time;
    }

#ifdef ENABLE_TIME_MACHINE
    return &s_current_time;
#else
    time_t now = time(NULL);
    return localtime(&now);
#endif
}

// Time zone correction needed for sunrise and sunset times
void adjust_timezone(float *time) {
//    *time += s_last_update_offset;
// last line replaced by this vibe code for screenshot mode implementation:
    int8_t utc_offset =  s_screenshot_mode ? s_screenshot_utc_offset : s_last_update_offset;
    *time += utc_offset;
//end vibe code
  
    if (*time > 24) *time -= 24;
    if (*time < 0)  *time += 24;
}

// For rectangular watches: calculate coordinates for indicators on rounded rectangular path
GPoint time_to_rectangular_coords(int input_time, GRect bounds) {
    GPoint coordinate = GPointZero;
    int arc_length = M_PI * s_rect_corner_radius / 2;
    int arc_progress = 0;
    int angle = 0;

    // Calculate full rounded rectangle path length
    int path_length = 2 * bounds.size.w + 2 * bounds.size.h + 4 * arc_length - 8 * s_rect_corner_radius;

    // Calculate length along that path for the given time
    int position = (input_time * path_length) / 1440;

    // Plot position with chain of if statements that check if it's in a particular section before moving to next
    if (position < (bounds.size.w / 2 - s_rect_corner_radius)) {
        // Moving left of center along bottom
        coordinate.x = bounds.size.w / 2 - position;
        coordinate.y = bounds.size.h - 1;
    } else if (position < (bounds.size.w / 2 + arc_length - s_rect_corner_radius)) {
        // Going around lower left arc
        arc_progress = position - (bounds.size.w / 2 - s_rect_corner_radius);
        angle = 5 * TRIG_MAX_ANGLE / 4 + (arc_progress * TRIG_MAX_ANGLE) / (4 * arc_length);
        coordinate.x = s_rect_corner_radius + s_rect_corner_radius * cos_lookup(angle) / TRIG_MAX_RATIO;
        coordinate.y = bounds.size.h - s_rect_corner_radius + s_rect_corner_radius * sin_lookup(angle) / TRIG_MAX_RATIO;
    } else if (position < (bounds.size.w / 2 + bounds.size.h + arc_length - 3 * s_rect_corner_radius)) {
        // Moving up left side
        coordinate.x = 0;
        coordinate.y = bounds.size.h + bounds.size.w / 2 + arc_length - position - 2 * s_rect_corner_radius;
    } else if (position < (bounds.size.w / 2 + bounds.size.h + 2 * arc_length - 3 * s_rect_corner_radius)) {
        // Going around upper left arc
        arc_progress = position - (bounds.size.w / 2 + bounds.size.h + arc_length - 3 * s_rect_corner_radius);
        angle = TRIG_MAX_ANGLE / 2 + (arc_progress * TRIG_MAX_ANGLE) / (4 * arc_length);
        coordinate.x = s_rect_corner_radius + s_rect_corner_radius * cos_lookup(angle) / TRIG_MAX_RATIO;
        coordinate.y = s_rect_corner_radius + s_rect_corner_radius * sin_lookup(angle) / TRIG_MAX_RATIO;
    } else if (position < (bounds.size.w + bounds.size.w / 2 + bounds.size.h + 2 * arc_length - 5 * s_rect_corner_radius)) {
        // Moving right along top
        coordinate.x = s_rect_corner_radius + position - (bounds.size.w / 2 + bounds.size.h + 2 * arc_length - 3 * s_rect_corner_radius);
        coordinate.y = 0;
    } else if (position < (bounds.size.w + bounds.size.w / 2 + bounds.size.h + 3 * arc_length - 5 * s_rect_corner_radius)) {
        // Going around upper right arc
        arc_progress = position - (bounds.size.w + bounds.size.w / 2 + bounds.size.h + 2 * arc_length - 5 * s_rect_corner_radius);
        angle = 3 * TRIG_MAX_ANGLE / 4 + (arc_progress * TRIG_MAX_ANGLE) / (4 * arc_length);
        coordinate.x = bounds.size.w - s_rect_corner_radius + s_rect_corner_radius * cos_lookup(angle) / TRIG_MAX_RATIO;
        coordinate.y = s_rect_corner_radius + s_rect_corner_radius * sin_lookup(angle) / TRIG_MAX_RATIO;
    } else if (position < (bounds.size.w + bounds.size.w / 2 + 2 * bounds.size.h + 3 * arc_length - 7 * s_rect_corner_radius)) {
        // Moving down along right side
        coordinate.x = bounds.size.w - 1;
        coordinate.y = position - ((3 * bounds.size.w) / 2 + bounds.size.h + 3 * arc_length - 6 * s_rect_corner_radius);
    } else if (position < (bounds.size.w + bounds.size.w / 2 + 2 * bounds.size.h + 4 * arc_length - 7 * s_rect_corner_radius)) {
        // Going around lower right arc
        arc_progress = position - (bounds.size.w + bounds.size.w / 2 + 2 * bounds.size.h + 3 * arc_length - 7 * s_rect_corner_radius);
        angle = (arc_progress * TRIG_MAX_ANGLE) / (4 * arc_length);
        coordinate.x = bounds.size.w - s_rect_corner_radius + s_rect_corner_radius * cos_lookup(angle) / TRIG_MAX_RATIO;
        coordinate.y = bounds.size.h - s_rect_corner_radius + s_rect_corner_radius * sin_lookup(angle) / TRIG_MAX_RATIO;
    } else {
        // Moving left along bottom
        coordinate.x = bounds.size.w - (s_rect_corner_radius + position - (bounds.size.w + bounds.size.w / 2 + 2 * bounds.size.h + 4 * arc_length - 7 * s_rect_corner_radius));
        coordinate.y = bounds.size.h - 1;
    }
    return coordinate;
}

// Draw time indicators (current time, sunrise, sunset, day, date) on background layer
static void back_update_proc(Layer *layer, GContext *ctx) {

    struct tm *t = get_display_time();

    GRect bounds = layer_get_bounds(layer);

    // This will only be unused on rectangular watches, so don't create for round ones, preventing build error
#if defined(PBL_ROUND)
    GPoint center = grect_center_point(&bounds);
    int radius = bounds.size.w / 2;
#endif

    int curr_y;
    int curr_x;

// override GPS coordinates when in "screenshot mode"  
  float display_lat  = s_screenshot_mode ? s_screenshot_lat_deg : s_local_lat_deg;
  float display_lng  = s_screenshot_mode ? s_screenshot_lng_deg : s_local_lng_deg;

// don't draw perimeter analog elements if user has disabled them. 
if (!s_analog_elements) {
    goto DRAW_DATE_AND_TEXT;
}
  
// Display sunrise & sunset indicators, but only if location has been successfully determined
    if (s_last_update_date != 0) {

// Calculate sunrise & set times
//   vibe code says: "tm_year is years since 1900, so currently your sunrise calculations are technically
//   using 126 instead of 2026.  This bug has probably been subtly affecting sunrise/sunset all along."
//        float sunrise_time = calcSunRise(t->tm_year, t->tm_mon + 1, t->tm_mday, display_lat, display_lng, 91.0f);
//        float sunset_time  = calcSunSet(t->tm_year,  t->tm_mon + 1, t->tm_mday, display_lat, display_lng, 91.0f);
float sunrise_time = calcSunRise(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, display_lat, display_lng, 91.0f);
float sunset_time  = calcSunSet(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, display_lat, display_lng, 91.0f);

// Only proceed with drawing sunrise indicator if there is a sunrise today
        if (sunrise_time != 100) {
// Correct sunrise time in case Daylight Savings Time was changed since last GPS update
//            if (s_last_update_dst < t->tm_isdst) {
// last line replaced by this vibe code for screenshot mode implementation:
          int8_t compare_dst =  s_screenshot_mode ? s_screenshot_dst : s_last_update_dst;
          if (compare_dst < t->tm_isdst) {
// end vibe code          
            sunrise_time += 1;

// vibe code replacement for screenshot mode implementation:
//           } else if (s_last_update_dst > t->tm_isdst) {
           } else if (compare_dst > t->tm_isdst) {
              sunrise_time -= 1;
            }

// Fix time zone of sunrise
            adjust_timezone(&sunrise_time);

// Display sunrise indicator
#if defined(PBL_ROUND)
// Along circular path
            int32_t sunrise_angle = (TRIG_MAX_ANGLE * (sunrise_time / 24));
            curr_y = radius * cos_lookup(sunrise_angle) / TRIG_MAX_RATIO + center.y;
            curr_x = -radius * sin_lookup(sunrise_angle) / TRIG_MAX_RATIO + center.x;
#else
// Along rounded rectangular path
            GPoint current_point = time_to_rectangular_coords((sunrise_time * 60), bounds);
            curr_x = current_point.x;
            curr_y = current_point.y;
#endif
            graphics_context_set_fill_color(ctx, s_sunrise_indicator_color);
            graphics_fill_circle(ctx, GPoint(curr_x, curr_y), s_sun_indicator_radius);
        }

// Only proceed with drawing sunset indicator if there is a sunset today
        if (sunset_time != 100) {
            // Correct sunset time in case Daylight Savings Time was changed since last GPS update
//           if (s_last_update_dst < t->tm_isdst) {
// last line replaced by this vibe code for screenshot mode implementation:
          int8_t compare_dst =  s_screenshot_mode ? s_screenshot_dst : s_last_update_dst;
          if (compare_dst < t->tm_isdst) {
// end vibe code          
            sunset_time += 1;
// vibe code replacement for screenshot mode implementation:
//            } else if (s_last_update_dst > t->tm_isdst) {
           } else if (compare_dst > t->tm_isdst) {
              sunset_time -= 1;
            }

            // Fix time zone of sunset
            adjust_timezone(&sunset_time);

// Display sunset indicator
#if defined(PBL_ROUND)
// Along circular path
            int32_t sunset_angle = (TRIG_MAX_ANGLE * (sunset_time / 24));
            curr_y = radius * cos_lookup(sunset_angle) / TRIG_MAX_RATIO + center.y;
            curr_x = -radius * sin_lookup(sunset_angle) / TRIG_MAX_RATIO + center.x;
#else
// Along rounded rectangular path
            GPoint current_point = time_to_rectangular_coords((sunset_time * 60), bounds);
            curr_x = current_point.x;
            curr_y = current_point.y;
#endif
            graphics_context_set_fill_color(ctx, s_sunset_indicator_color);
            graphics_fill_circle(ctx, GPoint(curr_x, curr_y), s_sun_indicator_radius);
        }
    }
 
// Display current time indicator
#if defined(PBL_ROUND)
// Along circular path
    int32_t hour_angle = (TRIG_MAX_ANGLE * ((t->tm_hour * 60) + t->tm_min) / 1440);
    curr_y = radius * cos_lookup(hour_angle) / TRIG_MAX_RATIO + center.y;
    curr_x = -radius * sin_lookup(hour_angle) / TRIG_MAX_RATIO + center.x;
#else
// Along rounded rectangular path
    GPoint current_point = time_to_rectangular_coords((t->tm_hour * 60 + t->tm_min), bounds);
    curr_x = current_point.x;
    curr_y = current_point.y;
#endif
    graphics_context_set_stroke_width(ctx, s_time_indicator_thickness);
    graphics_context_set_stroke_color(ctx, s_time_indicator_color);
    graphics_draw_circle(ctx, GPoint(curr_x, curr_y), s_time_indicator_radius);

//DRAW_DATE_AND_TEXT:
DRAW_DATE_AND_TEXT:;
// Setup variable to adjust date and day of week justification per user config setting
    GTextAlignment date_alignment = GTextAlignmentCenter;
    int dow_x_offset = 0;
    int date_x_offset = 0;

    if (s_date_align == 1) {
        date_alignment = GTextAlignmentLeft;

        dow_x_offset =
            get_line_x_offset(
            bounds.size.h -
            (s_date_lower_margin + s_date_line_spacing + s_date_line_height),
            bounds.size.h -
            (s_date_lower_margin + s_date_line_spacing + s_date_line_height) +
            s_date_line_height,
            s_date_side_margin,
            false
          );
        
        date_x_offset =
            get_line_x_offset(
            bounds.size.h -
            (s_date_lower_margin + s_date_line_height),
            bounds.size.h -
            (s_date_lower_margin + s_date_line_height) +
            s_date_line_height,
            s_date_side_margin,
            false
          );
      
    } else if (s_date_align == 3) {

        date_alignment = GTextAlignmentRight;

        dow_x_offset =
            get_line_x_offset(
            bounds.size.h -
            (s_date_lower_margin + s_date_line_spacing + s_date_line_height),
            bounds.size.h -
            (s_date_lower_margin + s_date_line_spacing + s_date_line_height) +
            s_date_line_height,
            s_date_side_margin,
            true
          );

        date_x_offset =
            get_line_x_offset(
            bounds.size.h - s_date_lower_margin - s_date_line_height,
            bounds.size.h - s_date_lower_margin,
            s_date_side_margin,
            true
          );
      }
  
// Display day of week
    char temp_string[32];
    strftime(temp_string, sizeof(temp_string), "%A", t);
    graphics_context_set_text_color(ctx, s_text_day_color);
    graphics_draw_text(ctx, temp_string, s_date_font,
                       GRect(
                         dow_x_offset,
                         bounds.size.h - (s_date_lower_margin + s_date_line_spacing + s_date_line_height),
                         bounds.size.w,
                         s_date_line_height
                       ),
                       GTextOverflowModeWordWrap, date_alignment, NULL);

// Display date
    strcpy(temp_string, MONTHS[t->tm_mon]);
    strcat(temp_string, DAYS[t->tm_mday]);
    graphics_context_set_text_color(ctx, s_text_date_color);
    graphics_draw_text(ctx, temp_string, s_date_font,
                       GRect(
                         date_x_offset,
                         bounds.size.h - s_date_lower_margin - s_date_line_height,
                         bounds.size.w,
                         s_date_line_height
                       ),
                       GTextOverflowModeWordWrap, date_alignment, NULL);
}

// Text animation stopped handler
static void animation_stopped_handler(struct Animation *animation, bool finished, void *context) {

    if (!context) return;

    Line *line = (Line *)context;
    line->current_layer_is_primary = !line->current_layer_is_primary;

// SDK auto-destroys completed animations - just null our pointers
    line->current_animation = NULL;
    line->next_animation = NULL;
}

// troubleshoot vertical animation timing gliches
static void scroll_down_stopped(Animation *animation, bool finished, void *context) {
    GRect rect = layer_get_frame(s_scroll);
    if (finished) {
        rect.origin.y = s_time_line_spacing / 2;
    }
    layer_set_frame(s_scroll, rect);
    s_scroll_down = NULL;

// if next minute is 3-line, schedule the return trip
    if (finished) {
        struct tm *t = get_display_time();
        if (!next_minute_is_two_line(t)) {
            int delay_ms = 59000 - (1000 * t->tm_sec);
            if (delay_ms > 800) {
                make_scroll_up(t);
            }
        }
    }
}

// troubleshoot vertical animation timing gliches
static void scroll_up_stopped(Animation *animation, bool finished, void *context) {
    GRect rect = layer_get_frame(s_scroll);

    if (finished) {
        rect.origin.y = 0;
    }

    layer_set_frame(s_scroll, rect);
    s_scroll_up = NULL;
}

// determine if time is 2 lines or 3 lines, to control vertical positioning
static bool is_two_line_time(struct tm *t) {
    char line1[BUFFER_SIZE];
    char line2[BUFFER_SIZE];
    char line3[BUFFER_SIZE];

    time_to_3words(
        t->tm_hour,
        t->tm_min,
        line1,
        line2,
        line3,
        BUFFER_SIZE
    );

    return (line3[0] == 0);
}

static bool next_minute_is_two_line(struct tm *t) {
    struct tm next = *t;

    next.tm_min++;

    if (next.tm_min >= 60) {
        next.tm_min = 0;
        next.tm_hour = (next.tm_hour + 1) % 24;
    }

    return is_two_line_time(&next);
}

// setup margins for time text, when left/right justified
//   For round watches, calculate unique X offset for given Y position
#ifdef PBL_ROUND
static int get_round_side_inset_for_y(
    int rect_top,
    int rect_bottom,
    int margin
) {
    float radius = s_bounds.size.w / 2.0f;
    float center_y = s_bounds.size.h / 2.0f;
    float top_dy = my_fabs((float)rect_top - center_y);
    float bottom_dy = my_fabs((float)rect_bottom - center_y);
    float top_half_chord = my_sqrt(radius * radius - top_dy * top_dy);
    float bottom_half_chord = my_sqrt(radius * radius - bottom_dy * bottom_dy);
    float top_visible_edge = radius + top_half_chord;
    float bottom_visible_edge = radius + bottom_half_chord;
    float limiting_edge = top_visible_edge < bottom_visible_edge ? top_visible_edge : bottom_visible_edge;
    float inset = s_bounds.size.w - limiting_edge;
    return (int)(inset + margin);
}
#endif

// setup left/right justification based on user selection
int get_line_x_offset(
    int rect_top,
    int rect_bottom,
    int margin,
    bool align_right
) {
#ifdef PBL_ROUND
    // The text box is taller than the visible glyphs.  Use the middle 50% of the box (30%-80%) when calculating round-watch side margins.
    int height = rect_bottom - rect_top;

    int inset =
        get_round_side_inset_for_y(
            rect_top + ((height * 30) / 100),  // 30% from top
            rect_top + ((height * 80) / 100),  // 80% from top
            margin
        );
#else
    int inset = margin;
#endif
    return align_right ? -inset : inset;
}

// Animate a text line in from the right, sliding the current one out to the left
static void make_animations_for_layers(Line *line, TextLayer *current, TextLayer *next,
                                       int current_y,
                                       int next_y
                                      ){
    bool time_align_right =
        (s_time_align == 3 ||
           s_time_align == 6);

    bool time_align_center =
        (s_time_align == 2 ||
           s_time_align == 5);

        int current_x = time_align_center ? 0 :
        get_line_x_offset(
        current_y - (s_time_line_height / 2),
        current_y + (s_time_line_height / 2),
        s_time_side_margin,
        time_align_right
    );

    int next_x = time_align_center ? 0 :
        get_line_x_offset(
        next_y - (s_time_line_height / 2),
        next_y + (s_time_line_height / 2),
        s_time_side_margin,
        time_align_right
    );
  
// Position next layer off screen to the right, ready to slide in
    GRect next_from = layer_get_frame((Layer *)next);
    next_from.origin.x = time_align_right ? s_bounds.size.w : s_bounds.size.w + next_x;
    layer_set_frame((Layer *)next, next_from);
  
    GRect next_to = next_from;
    next_to.origin.x = next_x;

    line->next_animation = property_animation_create_layer_frame((Layer *)next, &next_from, &next_to);
    animation_set_duration(property_animation_get_animation(line->next_animation), s_horizontal_slide_duration);
    animation_set_curve(property_animation_get_animation(line->next_animation), AnimationCurveEaseOut);
    animation_schedule(property_animation_get_animation(line->next_animation));

// Animate current layer off screen to the left
    GRect current_from = layer_get_frame((Layer *)current);
    current_from.origin.x = current_x;
    layer_set_frame((Layer *)current, current_from);

    GRect current_to = current_from;
    current_to.origin.x = time_align_right ? -s_bounds.size.w + current_x : -s_bounds.size.w;

    line->current_animation = property_animation_create_layer_frame((Layer *)current, &current_from, &current_to);
    animation_set_duration(property_animation_get_animation(line->current_animation), s_horizontal_slide_duration);
    animation_set_curve(property_animation_get_animation(line->current_animation), AnimationCurveEaseOut);

    animation_set_handlers(property_animation_get_animation(line->current_animation), (AnimationHandlers) {
        .stopped = (AnimationStoppedHandler)animation_stopped_handler
    }, line);

    animation_schedule(property_animation_get_animation(line->current_animation));
}

// Update a text line with animation
static void update_line_to(Line *line, char line_str[2][BUFFER_SIZE], char *value) {
    TextLayer *next, *current;

    current = line->current_layer_is_primary ? line->current_layer : line->next_layer;
    next    = line->current_layer_is_primary ? line->next_layer    : line->current_layer;

// Update the inactive buffer slot
    if (current == line->current_layer) {
        memset(line_str[1], 0, BUFFER_SIZE);
        snprintf(line_str[1], BUFFER_SIZE, "%s", value);
        text_layer_set_text(next, line_str[1]);
    } else {
        memset(line_str[0], 0, BUFFER_SIZE);
        snprintf(line_str[0], BUFFER_SIZE, "%s", value);
        text_layer_set_text(next, line_str[0]);
    }

    GRect current_frame =
        layer_get_frame((Layer *)current);
    GRect scroll_frame =
        layer_get_frame((Layer *)s_scroll);

    int current_y =
        current_frame.origin.y +
        scroll_frame.origin.y +
        (current_frame.size.h / 2);

    int next_y = current_y;
  
    if (s_time_align > 3) {
        struct tm *t = get_display_time();
        bool current_is_two = is_two_line_time(t);
        bool next_is_two_line = next_minute_is_two_line(t);
        if (current_is_two && !next_is_two_line) {
            next_y -= s_time_line_spacing / 2;
          }
        if (!current_is_two && next_is_two_line) {
            next_y += s_time_line_spacing / 2;
          }
      }
    make_animations_for_layers(line, current, next, current_y, next_y);
}

// Check to see if the current text line needs to be updated
static bool need_to_update_line(Line *line, char line_str[2][BUFFER_SIZE], char *next_value) {
    char *current_str = line->current_layer_is_primary ? line_str[0] : line_str[1];
    return strcmp(current_str, next_value) != 0;
}

// "midnight" is longest word ("thirteen" is next), and would limit font sizes for rectangular watches,
//    so we're going to swap in a smaller font for that one minute a day only.
void apply_minute_font(bool midnight_mode) {

    GFont active_font =
        midnight_mode
        ? s_midnight_font
        : s_minute_font;

    text_layer_set_font(s_line2.current_layer, active_font);
    text_layer_set_font(s_line2.next_layer,    active_font);
}

// Properly apply margins when time text justification is set to left or right, rather than centered
static void reposition_static_line(Line *line) {
    TextLayer *current =
        line->current_layer_is_primary
        ? line->current_layer
        : line->next_layer;

    GRect r =
        layer_get_frame((Layer *)current);

    GRect scroll =
        layer_get_frame((Layer *)s_scroll);

    int top =
        r.origin.y + scroll.origin.y;

    int bottom =
        top + r.size.h;

    bool time_align_right =
        (s_time_align == 3 ||
           s_time_align == 6);

    bool time_align_center =
        (s_time_align == 2 ||
           s_time_align == 5);

    int x = time_align_center ? 0 :
        get_line_x_offset(
        top,
        bottom,
        s_time_side_margin,
        time_align_right
    );
  
    if (r.origin.x != x) {
        r.origin.x = x;
        layer_set_frame((Layer *)current, r);
    }
}

// Update screen based on new time (with animation)
static void display_time(struct tm *t) {
    char text_line1[BUFFER_SIZE];
    char text_line2[BUFFER_SIZE];
    char text_line3[BUFFER_SIZE];

    time_to_3words(t->tm_hour, t->tm_min, text_line1, text_line2, text_line3, BUFFER_SIZE);
 
// On some configurations we need a smaller font for "midnight", the longest word.  (Don't want to reduce font size globally for one minute a day.)
  bool midnight_mode =
        (t->tm_hour == 0 && t->tm_min == 0);
    apply_minute_font(midnight_mode);

    if (need_to_update_line(&s_line1, s_line1_str, text_line1)) {
        update_line_to(&s_line1, s_line1_str, text_line1);
      } else {
        reposition_static_line(&s_line1);
      }
    if (need_to_update_line(&s_line2, s_line2_str, text_line2)) {
        update_line_to(&s_line2, s_line2_str, text_line2);
      } else {
        reposition_static_line(&s_line2);
      }
    if (need_to_update_line(&s_line3, s_line3_str, text_line3)) {
      update_line_to(&s_line3, s_line3_str, text_line3);
      } else {
        reposition_static_line(&s_line3);
      }
}

// Update screen the first time we start the watchface
void display_initial_time(struct tm *t) {
    time_to_3words(t->tm_hour, t->tm_min, s_line1_str[0], s_line2_str[0], s_line3_str[0], BUFFER_SIZE);
    text_layer_set_text(s_line1.current_layer, s_line1_str[0]);
    text_layer_set_text(s_line2.current_layer, s_line2_str[0]);
    text_layer_set_text(s_line3.current_layer, s_line3_str[0]);
}

// animate in initial time display
void animate_initial_time_in() {
    Line *lines[] = {
        &s_line1,
        &s_line2,
        &s_line3
    };
    for (int i = 0; i < 3; i++) {
        TextLayer *layer = lines[i]->current_layer;
      
        GRect to = layer_get_frame((Layer *)layer);
        GRect from = to;

        bool time_align_right =
            (s_time_align == 3 ||
               s_time_align == 6);

        bool time_align_center =
            (s_time_align == 2 ||
               s_time_align == 5);

        int target_x = time_align_center ? 0 :
            get_line_x_offset(
            to.origin.y,
            to.origin.y + to.size.h,
            s_time_side_margin,
            time_align_right
            );
      
        from.origin.x = s_bounds.size.w + target_x;
        to.origin.x = target_x;

        layer_set_frame((Layer *)layer, from);

        PropertyAnimation *anim =
            property_animation_create_layer_frame((Layer *)layer, &from, &to);

        animation_set_duration(property_animation_get_animation(anim), s_horizontal_slide_duration);
        animation_set_curve(property_animation_get_animation(anim), AnimationCurveEaseOut);
        animation_schedule(
            property_animation_get_animation(anim)
        );
    }
}

// Animate down to recenter when going from 3 lines of text to only 2 lines
static void make_scroll_down() {
    if (s_scroll_down) {
        Animation *a = property_animation_get_animation(s_scroll_down);
        if (animation_is_scheduled(a)) {
              animation_unschedule(a);
          }
        s_scroll_down = NULL;
      }

    GRect from = layer_get_frame((Layer *)s_scroll);
    GRect to   = layer_get_frame((Layer *)s_scroll);

    to.origin.y   = s_time_line_spacing / 2;
    s_scroll_down = property_animation_create_layer_frame((Layer *)s_scroll, &from, &to);
    animation_set_duration(property_animation_get_animation(s_scroll_down), 800);
    animation_set_delay(property_animation_get_animation(s_scroll_down), 600);
    animation_set_curve(property_animation_get_animation(s_scroll_down), AnimationCurveEaseOut);

    animation_set_handlers(property_animation_get_animation(s_scroll_down),
    (AnimationHandlers) {
        .stopped = scroll_down_stopped
    }, NULL);
  
    animation_schedule(property_animation_get_animation(s_scroll_down));
}

// Animate back up to prepare for return of 3 lines of text.
// Sets delay based on number of seconds left in minute.
// Must not call this function if not enough time left in minute to do it!
static void make_scroll_up(struct tm *t) {
    if (s_scroll_up) {
        Animation *a = property_animation_get_animation(s_scroll_up);
        if (animation_is_scheduled(a)) {
            animation_unschedule(a);
        }
      s_scroll_up = NULL;
}
  
    GRect from = layer_get_frame((Layer *)s_scroll);
    GRect to   = from;
    to.origin.y = 0;
  
    s_scroll_up = property_animation_create_layer_frame((Layer *)s_scroll, &from, &to);
    animation_set_duration(property_animation_get_animation(s_scroll_up), 800);

// For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
#ifdef ENABLE_TIME_MACHINE
    animation_set_delay(property_animation_get_animation(s_scroll_up), 100);
#else

// vibe had this at 57000 for troubleshooting, but 59000 was my intent
    int delay_ms = 59000 - 1000 * t->tm_sec;
    if (delay_ms < 1) {
        delay_ms = 1;
      }
    animation_set_delay(
        property_animation_get_animation(s_scroll_up),
        delay_ms
      );
#endif

    animation_set_curve(property_animation_get_animation(s_scroll_up), AnimationCurveEaseOut);
    animation_set_handlers(property_animation_get_animation(s_scroll_up),
      (AnimationHandlers) {
          .stopped = scroll_up_stopped
      }, NULL);
  
    animation_schedule(property_animation_get_animation(s_scroll_up));
}

// Configure the first line of text
static void configure_line1_layer(TextLayer *textlayer, GTextAlignment time_alignment) {
    text_layer_set_font(textlayer, s_hour_font);
    text_layer_set_text_color(textlayer, s_text_line1_color);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, time_alignment);
}

// Configure the 2nd line of text
static void configure_line2_layer(TextLayer *textlayer, GTextAlignment time_alignment) {
    text_layer_set_font(textlayer, s_minute_font);
    text_layer_set_text_color(textlayer, s_text_line2_color);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, time_alignment);
}

// Configure the 3rd line of text
static void configure_line3_layer(TextLayer *textlayer, GTextAlignment time_alignment) {
    text_layer_set_font(textlayer, s_minute_font);
    text_layer_set_text_color(textlayer, s_text_line3_color);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, time_alignment);
}

// Set up default configuration settings (colors, single digit prefix, GPS disable), for when user hasn't changed anything yet
static void prime_settings() {
    if (!persist_exists(NO_GPS_STORED)) {
        persist_write_bool(NO_GPS_STORED, false);          // Don't disable GPS by default
    }
    if (!persist_exists(SHOW_O_PREFIX_STORED)) {
        persist_write_bool(SHOW_O_PREFIX_STORED, true);    // Show O' prefix by default
    }
    if (!persist_exists(SHOW_OH_PREFIX_STORED)) {
        persist_write_bool(SHOW_OH_PREFIX_STORED, false);  // Don't show Oh prefix by default
    }
    if (!persist_exists(ENLARGE_STORED)) {
        persist_write_bool(ENLARGE_STORED, true);          // Use larger fonts on high res watches by default
    }
    if (!persist_exists(TIME_ALIGN_STORED)) {
        persist_write_int(TIME_ALIGN_STORED, 5);      // middle center time text alignment by default
    }
    if (!persist_exists(DATE_ALIGN_STORED)) {
        persist_write_int(DATE_ALIGN_STORED, 2);      // centered date text alignment by default
    }
    if (!persist_exists(ANALOG_ELEMENTS_STORED)) {
        persist_write_bool(ANALOG_ELEMENTS_STORED, true);  // show analog elements by default
    }
    if (!persist_exists(BACKGROUND_COLOR_STORED)) {
        persist_write_int(BACKGROUND_COLOR_STORED, 0x000000);           // GColorBlack
    }
    if (!persist_exists(TEXT_LINE_1_COLOR_STORED)) {
        persist_write_int(TEXT_LINE_1_COLOR_STORED, 0xFFFFFF);          // GColorWhite
    }
    if (!persist_exists(TEXT_LINE_2_COLOR_STORED)) {
        persist_write_int(TEXT_LINE_2_COLOR_STORED, 0xFFFFFF);          // GColorWhite
    }
    if (!persist_exists(TEXT_LINE_3_COLOR_STORED)) {
        persist_write_int(TEXT_LINE_3_COLOR_STORED, 0xFFFFFF);          // GColorWhite
    }
    if (!persist_exists(TEXT_DAY_COLOR_STORED)) {
        persist_write_int(TEXT_DAY_COLOR_STORED, 0xFFFFFF);             // GColorWhite
    }
    if (!persist_exists(TEXT_DATE_COLOR_STORED)) {
        persist_write_int(TEXT_DATE_COLOR_STORED, 0xFFFFFF);            // GColorWhite
    }
    if (!persist_exists(TIME_INDICATOR_COLOR_STORED)) {
        persist_write_int(TIME_INDICATOR_COLOR_STORED, 0xAAAA00);       // GColorLimerick
    }
    if (!persist_exists(SUNRISE_INDICATOR_COLOR_STORED)) {
        persist_write_int(SUNRISE_INDICATOR_COLOR_STORED, 0xAAAAAA);    // GColorLightGray
    }
    if (!persist_exists(SUNSET_INDICATOR_COLOR_STORED)) {
        persist_write_int(SUNSET_INDICATOR_COLOR_STORED, 0xAAAAAA);     // GColorLightGray
    }
}

// Update color & other settings based on config page choices
static void update_settings() {
    s_background_color       = GColorFromHEX(persist_read_int(BACKGROUND_COLOR_STORED));
    s_text_line1_color       = GColorFromHEX(persist_read_int(TEXT_LINE_1_COLOR_STORED));
    s_text_line2_color       = GColorFromHEX(persist_read_int(TEXT_LINE_2_COLOR_STORED));
    s_text_line3_color       = GColorFromHEX(persist_read_int(TEXT_LINE_3_COLOR_STORED));
    s_text_day_color         = GColorFromHEX(persist_read_int(TEXT_DAY_COLOR_STORED));
    s_text_date_color        = GColorFromHEX(persist_read_int(TEXT_DATE_COLOR_STORED));
    s_time_indicator_color   = GColorFromHEX(persist_read_int(TIME_INDICATOR_COLOR_STORED));
    s_sunrise_indicator_color = GColorFromHEX(persist_read_int(SUNRISE_INDICATOR_COLOR_STORED));
    s_sunset_indicator_color = GColorFromHEX(persist_read_int(SUNSET_INDICATOR_COLOR_STORED));
    s_no_gps          = persist_read_bool(NO_GPS_STORED);
    s_analog_elements        = persist_read_bool(ANALOG_ELEMENTS_STORED);
    s_enlarge         = persist_read_bool(ENLARGE_STORED);
    s_time_align = persist_read_int(TIME_ALIGN_STORED);
    s_date_align = persist_read_int(DATE_ALIGN_STORED);
}

// App communication with phone; this is the main function that operates when a callback happens for the GPS or config update request
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

    int16_t new_latitude;
    int16_t new_longitude;
    int8_t use_new_data = 0;   // Assume using old data until proven otherwise
    int8_t new_ut_offset;
    bool start_demo_now = false;

// so screen is redrawn only for config changes that require it, not GPS updates.
    bool needs_visual_rebuild = false;
  
// Read first item
    Tuple *t = dict_read_first(iterator);

// For all items
    while (t != NULL) {

// Debug logging, reactivate if need to troubleshoot new keys
//        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received key=%d", (int)t->key);

        // Which key was received?
        switch (t->key) {
            case 0:
                // Key 0 is the first entry in the dictionary, skipped for values
                // as I believe that was the source of problems
                break;
            case KEY_LATITUDE:
                new_latitude = t->value->int32;
                break;
            case KEY_LONGITUDE:
                new_longitude = (int)t->value->int32;
                break;
            case KEY_USEOLDDATA:
                use_new_data = (int)t->value->int32;
                break;
            case KEY_UTCH:
                new_ut_offset = (int)t->value->int32;
                break;
            case KEY_SINGLE_PREFIX_TYPE:
                if (t->value->uint8 == 1) {
                    persist_write_bool(SHOW_O_PREFIX_STORED, false);
                    persist_write_bool(SHOW_OH_PREFIX_STORED, false);
                } else if (t->value->uint8 == 2) {
                    persist_write_bool(SHOW_O_PREFIX_STORED, true);
                    persist_write_bool(SHOW_OH_PREFIX_STORED, false);
                } else if (t->value->uint8 == 3) {
                    persist_write_bool(SHOW_O_PREFIX_STORED, false);
                    persist_write_bool(SHOW_OH_PREFIX_STORED, true);
                }
          // so screen redraws only when needed, not for location updates
                needs_visual_rebuild = true;
                break;
            case KEY_BACKGROUND_COLOR:
                persist_write_int(BACKGROUND_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TEXT_LINE_1_COLOR:
                persist_write_int(TEXT_LINE_1_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TEXT_LINE_2_COLOR:
                persist_write_int(TEXT_LINE_2_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TEXT_LINE_3_COLOR:
                persist_write_int(TEXT_LINE_3_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TEXT_DAY_COLOR:
                persist_write_int(TEXT_DAY_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TEXT_DATE_COLOR:
                persist_write_int(TEXT_DATE_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_TIME_INDICATOR_COLOR:
                persist_write_int(TIME_INDICATOR_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_SUNRISE_INDICATOR_COLOR:
                persist_write_int(SUNRISE_INDICATOR_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_SUNSET_INDICATOR_COLOR:
                persist_write_int(SUNSET_INDICATOR_COLOR_STORED, t->value->uint32);
                needs_visual_rebuild = true;
                break;
            case KEY_NOGPS:
                persist_write_bool(NO_GPS_STORED, (bool)t->value->uint8);
                break;
            case KEY_ANALOG_ELEMENTS:
                persist_write_bool(ANALOG_ELEMENTS_STORED, (bool)t->value->uint8);
                needs_visual_rebuild = true;
                break;
            case KEY_ENLARGE:
                persist_write_bool(ENLARGE_STORED, (bool)t->value->uint8);
                needs_visual_rebuild = true;
                break;
            case KEY_TIME_ALIGN:
                persist_write_int(TIME_ALIGN_STORED, (int)t->value->uint8);
                needs_visual_rebuild = true;
                break;
            case KEY_DATE_ALIGN:
                persist_write_int(DATE_ALIGN_STORED, (int)t->value->uint8);
                needs_visual_rebuild = true;
                break;
            case KEY_START_DEMO:
                start_demo_now = (bool)t->value->uint8;
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
                break;
        }

        // Look for next item
        t = dict_read_next(iterator);
    }

    // Transfer the incoming information into the stores
    if (use_new_data == 0) {
        // No new position data available, use old data.
        // Latitude, longitude, UTC offset, and DST indicator remain unchanged.
    } else {
        // New position data available, update global variables.
        // Lat, long, and UTC must remain as flexible variables.
        // Rise values can be directly accessed through storage.
        // Last update time can be updated to now as well.

        // For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
        //   or for screenshot mode
        struct tm *t = get_display_time();

        s_last_update_time   = (t->tm_hour) * 60 + (t->tm_min);
        s_last_update_date   = (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;  // Alterations account for time.h functions
        s_local_lat_deg      = (float)new_latitude / 100;
        s_local_lng_deg      = (float)new_longitude / 100;
        s_last_update_offset = new_ut_offset;
        s_last_update_dst    = t->tm_isdst;

        persist_write_int(LAT_STORED,       new_latitude);  // Already as an integer (must / 100 when retrieving)
        persist_write_int(LNG_STORED,       new_longitude);
        persist_write_int(UTC_STORED,       new_ut_offset);
        persist_write_int(LAST_TIME_STORED, s_last_update_time);
        persist_write_int(LAST_DATE_STORED, s_last_update_date);
        persist_write_int(DST_STORED,       s_last_update_dst);
    }

// Update variables with newly received settings, and apply color changes to background, back layer, and text layers
    update_settings();

    window_set_background_color(s_main_window, s_background_color);
    layer_mark_dirty(s_back_layer);
    text_layer_set_text_color(s_line1.current_layer, s_text_line1_color);
    text_layer_set_text_color(s_line1.next_layer,    s_text_line1_color);
    text_layer_set_text_color(s_line2.current_layer, s_text_line2_color);
    text_layer_set_text_color(s_line2.next_layer,    s_text_line2_color);
    text_layer_set_text_color(s_line3.current_layer, s_text_line3_color);
    text_layer_set_text_color(s_line3.next_layer,    s_text_line3_color);

// Rebuild layers with new settings and animate in
    if (needs_visual_rebuild) {
        setup_time_layers(false);
      }
    if (start_demo_now) {
        demo_start();
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {}

// Update graphics when timer ticks
static void time_timer_tick(struct tm *t, TimeUnits units_changed) {
    if (s_demo_running) {
    return;
    }
    
    if (s_screenshot_mode) {
        layer_mark_dirty(s_back_layer);
        return;
      }
 
 // For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
#ifdef ENABLE_TIME_MACHINE
    s_current_time = *t;
#endif

// Update location if it's never been determined, and at the top of the hour thereafter, but only if option to disable hasn't been selected in config.html
if (!s_no_gps && s_analog_elements && !s_screenshot_mode) {
    bool need_initial_fix =
        (s_last_update_time == 0);

    bool top_of_hour =
        (t->tm_min == 0);

    if (need_initial_fix || top_of_hour) {
        update_gps();
    }
}  
  
// Update background layer (indicators for current time, sunrise, sunset, day of week, and date)
    if (units_changed & MINUTE_UNIT) {
        layer_mark_dirty(s_back_layer);
    }

    // Update text time
    display_time(t);

// Adjust time text position between 2 and 3 line centering, unless user has selected "top" alignment
    bool allow_vertical_animation = (s_time_align > 3);

    if (allow_vertical_animation) {

        bool current_is_two_line =
            is_two_line_time(t);

        bool next_is_two_line =
            next_minute_is_two_line(t);

        bool prev_was_two_line = is_two_line_time(&(struct tm){
            .tm_hour = (t->tm_min == 0)
                ? ((t->tm_hour + 23) % 24)
                : t->tm_hour,
            .tm_min  = (t->tm_min == 0)
                ? 59
                : (t->tm_min - 1)
        });

        bool just_entered_two_line = current_is_two_line && !prev_was_two_line;

        GRect rect = layer_get_frame((Layer *)s_scroll);

        // Authoritative snap to correct Y — skipped for all 3->2 transitions,
        // which are animated by make_scroll_down() instead.
        if (!just_entered_two_line) {
            rect.origin.y =
                current_is_two_line
                    ? s_time_line_spacing / 2
                    : 0;
            layer_set_frame((Layer *)s_scroll, rect);
        }

        if (just_entered_two_line) {
            // 3->2 transition: animate scroll down.
            // scroll_down_stopped schedules the return scroll-up for one-minute islands.
            make_scroll_down();
        } else if (current_is_two_line && !next_is_two_line) {
            // Stable 2-line, next minute is 3-line: proactive scroll-up before tick.
            int delay_ms = 59000 - (1000 * t->tm_sec);
            if (delay_ms > 800) {
                make_scroll_up(t);
            }
        }
    }
}
  
// Setup parameters for various layouts, and load & unload fonts properly when layout changes
static void load_fonts_and_metrics() {
#if PBL_DISPLAY_WIDTH >= 200
// Unload previously loaded custom fonts safely
    if (s_custom_fonts_loaded) {
        if (s_hour_font) {
            fonts_unload_custom_font(s_hour_font);
            s_hour_font = NULL;
        }
        if (s_minute_font) {
            fonts_unload_custom_font(s_minute_font);
            s_minute_font = NULL;
        }
        if (s_midnight_font) {
            fonts_unload_custom_font(s_midnight_font);
            s_midnight_font = NULL;
          }
        if (s_date_font) {
              fonts_unload_custom_font(s_date_font);
              s_date_font = NULL;
          }
        s_custom_fonts_loaded = false;
    }
#endif

// Set element sizes and offsets for original resolution devices, with larger sizes specified for higher resolution devices, unless user config override is set.
//  See "pivot table of config values.xlsx" document for help planning these values; easier to compare how particular values change from build to build
    // Here are the default sizes, used for the old watches, or for new higher res watches when the configuration option to "enlarge fonts" is deactivated.
    s_hour_font     = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
    s_minute_font   = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
    //    For smallest sizes watches (aplite, basalt, diorite, flint), there isn't enough room for "midnight", so we will replace with smaller font.
    //    There is room for it on chalk, so we will not replace with smaller font in that case.
    //       The custom smaller font didn't work on aplite (the OG Pebble), so we'll use an alternative system font in that case only
#if defined(PBL_PLATFORM_APLITE)
   s_midnight_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);  // midnight font, alite only
#elif PBL_DISPLAY_WIDTH < 180
    s_midnight_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_33));  // midnight font, basalt, diorite, flint
    s_custom_fonts_loaded = true;
#else
    s_midnight_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);  // midnight font, chaulk (no change from non-midnight minute font)
#endif
    s_date_font   = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);  // aplite, basalt, chalk, diorite, flint
    s_time_line_height         = 50;
    s_time_line_spacing        = 37;
#if defined(PBL_ROUND)
    s_top_time_margin          = 10;  // chaulk
    s_middle_time_offset       = 14;
    s_time_side_margin         = 3;
    s_date_side_margin         = 3;
    s_date_lower_margin        = 3;
#else
    s_top_time_margin          = -9; // aplite, basalt, diorite, flint
    s_middle_time_offset       = 18;
    s_time_side_margin         = 0;
    s_date_side_margin         = 3;
    s_date_lower_margin        = -2;
#endif
    s_date_line_height         = 30; // aplite, basalt, chalk, diorite, flint
    s_date_line_spacing        = 19;
    s_time_indicator_radius    = 9;
    s_time_indicator_thickness = 2;
    s_sun_indicator_radius     = 6;
    s_rect_corner_radius       = 8;

    // If user hasn't disabled the option, use larger elements for watches with horizontal resolution greater than 200.
    //   As of 2026, this includes Pebble Time 2 (Emery) and Pebble Time Round 2 (Gabbro).
#if PBL_DISPLAY_WIDTH >= 200
    if (s_enlarge) {
#if PBL_DISPLAY_WIDTH >= 260
        // Let's go even bigger with SOME of these options for watches with horizontal resolution greater than 260.
        //   As of 2026, this is only Pebble Time Round 2 (Gabbro).
        s_hour_font   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_bold_56));
        s_minute_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_56));
        s_midnight_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_56));
        s_time_line_height        = 66;
        s_time_line_spacing       = 50;
#if defined(PBL_ROUND)
        s_top_time_margin        = 19;  // this is the Gabbro value
        s_middle_time_offset     = 18;
        s_time_side_margin       = 5;
        s_date_side_margin       = 9;
        s_date_lower_margin      = 7;
#else
        s_top_time_margin        = -7; // value for hypothetical future rectangular watch with higher resoluion than Emery
        s_middle_time_offset     = 35;
        s_time_side_margin       = 6;
        s_date_side_margin       = 6;
        s_date_lower_margin      = 5;
#endif
#else
        // Here are the fonts for watches with resolution greater than or equal to 200, but less than 260.
        // As of 2026, this is only Pebble Time 2 (Emery).
        s_hour_font     = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_bold_51));
        s_minute_font   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_51));
        s_midnight_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_46));
        s_time_line_height        = 59;
        s_time_line_spacing       = 45;
#if defined(PBL_ROUND)
        s_top_time_margin         = 15;  // value for hypothetical future round watch with resolution between Gabbro & Chaulk
        s_middle_time_offset      = 16;
        s_time_side_margin        = 2;
        s_date_side_margin        = 9;
        s_date_lower_margin       = 7;
#else
        s_top_time_margin          = -8; // this is the Emery value
        s_middle_time_offset       = 30;
        s_time_side_margin         = 4;
        s_date_side_margin         = 6;
        s_date_lower_margin        = 5;
#endif
#endif
        // Here are the rest of the options for watches with horizontal resolution greater than 200.
        //  (These don't get altered further for watches with horizontal resolution greater than 260.) 
        //  As of 2026, this includes Pebble Time 2 (Emery) and Pebble Time Round 2 (Gabbro).
        s_custom_fonts_loaded      = true;
        s_date_font                = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_Alt_Gothic_ATF_Demi_32));
        s_date_line_height         = 34; // I think my edits got lost, recheck
        s_date_line_spacing        = 29;
        s_time_indicator_radius    = 14;
        s_time_indicator_thickness = 4;
        s_sun_indicator_radius     = 9;
        s_rect_corner_radius       = 12;
    }
#endif
}

// Configure main window
static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);
    window_set_background_color(s_main_window, s_background_color);
    s_back_layer = layer_create(s_bounds);
    layer_set_update_proc(s_back_layer, back_update_proc);
    layer_add_child(window_layer, s_back_layer);
    load_fonts_and_metrics();
}

// Deconfigure main window
static void main_window_unload(Window *window) {
    layer_destroy(s_back_layer);
}

// Build (or rebuild) all text layers for time display
void setup_time_layers(bool animate){

  // Update fonts and metrics after user config change
    load_fonts_and_metrics();
    cancel_all_watch_animations();

    // Destroy existing text layers if they exist
    if (s_line1.current_layer) text_layer_destroy(s_line1.current_layer);
    if (s_line1.next_layer)    text_layer_destroy(s_line1.next_layer);
    if (s_line2.current_layer) text_layer_destroy(s_line2.current_layer);
    if (s_line2.next_layer)    text_layer_destroy(s_line2.next_layer);
    if (s_line3.current_layer) text_layer_destroy(s_line3.current_layer);
    if (s_line3.next_layer)    text_layer_destroy(s_line3.next_layer);

    s_line1.current_layer = NULL;  s_line1.next_layer = NULL;
    s_line2.current_layer = NULL;  s_line2.next_layer = NULL;
    s_line3.current_layer = NULL;  s_line3.next_layer = NULL;
    s_line1.current_layer_is_primary = true;
    s_line2.current_layer_is_primary = true;
    s_line3.current_layer_is_primary = true;

    if (s_scroll) {
        layer_remove_from_parent(s_scroll);
        layer_destroy(s_scroll);
        s_scroll = NULL;
    }

    Layer *root = window_get_root_layer(s_main_window);
    s_bounds = layer_get_bounds(root);
    s_scroll = layer_create(s_bounds);

    // Setup variables to adjust time text justification per user config setting
    GTextAlignment time_alignment = (s_time_align == 1 || s_time_align == 4)
    ? GTextAlignmentLeft
    : ((s_time_align == 2 || s_time_align == 5)
         ? GTextAlignmentCenter
         : GTextAlignmentRight);

// line Y positions
   int line1_y;
   int line2_y;
   int line3_y;
    if (s_time_align < 4) {
        line1_y = s_top_time_margin;
        } else {
        line1_y = (s_bounds.size.h / 2) - s_middle_time_offset - s_time_line_spacing - (s_time_line_height / 2);
        }
        line2_y = line1_y + s_time_line_spacing;
        line3_y = line1_y + 2 * s_time_line_spacing;
      
// corresponding X offsets
    bool time_align_right =
        (s_time_align == 3 ||
           s_time_align == 6);

    bool time_align_center =
        (s_time_align == 2 ||
           s_time_align == 5);

    int line1_x = time_align_center ? 0 : get_line_x_offset(line1_y, line1_y + s_time_line_height, s_time_side_margin, time_align_right);
    int line2_x = time_align_center ? 0 : get_line_x_offset(line2_y, line2_y + s_time_line_height, s_time_side_margin, time_align_right);
    int line3_x = time_align_center ? 0 : get_line_x_offset(line3_y, line3_y + s_time_line_height, s_time_side_margin, time_align_right);

// 1st line layer
    s_line1.current_layer = text_layer_create(
        GRect(line1_x, line1_y, s_bounds.size.w, s_time_line_height));
    s_line1.next_layer = text_layer_create(
        GRect(s_bounds.size.w + line1_x, line1_y,
              s_bounds.size.w, s_time_line_height));
    configure_line1_layer(s_line1.current_layer, time_alignment);
    configure_line1_layer(s_line1.next_layer,    time_alignment);

// 2nd line layer
    s_line2.current_layer = text_layer_create(
        GRect(line2_x, line2_y, s_bounds.size.w, s_time_line_height));
    s_line2.next_layer = text_layer_create(
        GRect(s_bounds.size.w + line2_x, line2_y,
              s_bounds.size.w, s_time_line_height));
    configure_line2_layer(s_line2.current_layer, time_alignment);
    configure_line2_layer(s_line2.next_layer,    time_alignment);

// 3rd line layer
    s_line3.current_layer = text_layer_create(
        GRect(line3_x, line3_y, s_bounds.size.w, s_time_line_height));
    s_line3.next_layer = text_layer_create(
        GRect(s_bounds.size.w + line3_x, line3_y,
              s_bounds.size.w, s_time_line_height));

    configure_line3_layer(s_line3.current_layer, time_alignment);
    configure_line3_layer(s_line3.next_layer,    time_alignment);

// Load layers
    layer_add_child(root, s_scroll);
    layer_add_child(s_scroll, (Layer *)s_line1.current_layer);
    layer_add_child(s_scroll, (Layer *)s_line1.next_layer);
    layer_add_child(s_scroll, (Layer *)s_line2.current_layer);
    layer_add_child(s_scroll, (Layer *)s_line2.next_layer);
    layer_add_child(s_scroll, (Layer *)s_line3.current_layer);
    layer_add_child(s_scroll, (Layer *)s_line3.next_layer);

// Force all lines to animate in fresh
    memset(s_line1_str, 0, sizeof(s_line1_str));
    memset(s_line2_str, 0, sizeof(s_line2_str));
    memset(s_line3_str, 0, sizeof(s_line3_str));

// Configure text time on init
    struct tm *now_tm = get_display_time();

    apply_minute_font(
        now_tm->tm_hour == 0 &&
        now_tm->tm_min == 0
      );

  display_initial_time(now_tm);

    if (animate) {
        animate_initial_time_in();
      } else {
      
// Ensure all layers are positioned onscreen
        Line *lines[] = {
            &s_line1,
            &s_line2,
            &s_line3
        };

        for (int i = 0; i < 3; i++) {

            TextLayer *layers[2] = {
                lines[i]->current_layer,
                lines[i]->next_layer
            };

            for (int j = 0; j < 2; j++) {

                GRect r =
                    layer_get_frame((Layer *)layers[j]);

                r.origin.x = time_align_center ? 0 : get_line_x_offset(r.origin.y, r.origin.y + r.size.h, s_time_side_margin, time_align_right);
                layer_set_frame((Layer *)layers[j], r);
            }
        }
    }
  
// Vibe code: prime buffers to match what was just displayed
    time_to_3words(now_tm->tm_hour, now_tm->tm_min, s_line1_str[0], s_line2_str[0], s_line3_str[0], BUFFER_SIZE);
    memcpy(s_line1_str[1], s_line1_str[0], BUFFER_SIZE);
    memcpy(s_line2_str[1], s_line2_str[0], BUFFER_SIZE);
    memcpy(s_line3_str[1], s_line3_str[0], BUFFER_SIZE);

    // If current time is a 2-line display, pop down to centered position immediately
    if (s_time_align > 3)

// for 2 to 3 line vertical transitions
    {
        bool current_is_two_line =
            is_two_line_time(now_tm);

        GRect rect = layer_get_frame((Layer *)s_scroll);

        rect.origin.y =
            current_is_two_line
                ? s_time_line_spacing / 2
                : 0;

        layer_set_frame((Layer *)s_scroll, rect);

 // If we're currently in a 2-line time and next minute becomes 3-line, schedule upward animation before tick.
        if (current_is_two_line &&
            !next_minute_is_two_line(now_tm)) {

            int delay_ms = 59000 - (1000 * now_tm->tm_sec);

            if (delay_ms > 800) {
                make_scroll_up(now_tm);
            }
        }     
    }
}

//  screenshot mode, for displaying particular time, date & location without updates
void start_screenshot_mode(void) {

    s_screenshot_mode = true;

// Pretend GPS exists
    s_last_update_date = 1;

    cancel_all_watch_animations();

    setup_time_layers(false);
    layer_mark_dirty(s_back_layer);
}

// Cancel upcoming animations when entering demo mode
void cancel_all_watch_animations(void) {
    if (s_scroll_down) {
        Animation *a = property_animation_get_animation(s_scroll_down);
        if (animation_is_scheduled(a)) {
            animation_unschedule(a);
        }
        s_scroll_down = NULL;
    }
    if (s_scroll_up) {
        Animation *a = property_animation_get_animation(s_scroll_up);
        if (animation_is_scheduled(a)) {
            animation_unschedule(a);
        }
        s_scroll_up = NULL;
    }
    Line *lines[] = {
        &s_line1,
        &s_line2,
        &s_line3
    };
    for (int i = 0; i < 3; i++) {
        if (lines[i]->current_animation) {
            Animation *a =
                property_animation_get_animation(
                    lines[i]->current_animation
                );
            if (animation_is_scheduled(a)) {
                animation_unschedule(a);
            }

            lines[i]->current_animation = NULL;
        }
        if (lines[i]->next_animation) {
            Animation *a =
                property_animation_get_animation(
                    lines[i]->next_animation
                );
            if (animation_is_scheduled(a)) {
                animation_unschedule(a);
            }
            lines[i]->next_animation = NULL;
        }
    }
}

// Initialize watchface
static void init() {

// Vibe code: "This makes teardown/rebuild logic deterministic."
    memset(&s_line1, 0, sizeof(Line));
    memset(&s_line2, 0, sizeof(Line));
    memset(&s_line3, 0, sizeof(Line));
    s_scroll      = NULL;
    s_scroll_up   = NULL;
    s_scroll_down = NULL;

// Register the callbacks for when a GPS request is made
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

// Open AppMessage
    app_message_open(256, 256);  // Increased size from "APP_MESSAGE_OUTBOX_SIZE_MINIMUM" to "256" in hopes new messages won't get truncated

// Initialize global location and time values from persistent storage
    s_local_lat_deg      = (float)persist_read_int(LAT_STORED) / 100;
    s_local_lng_deg      = (float)persist_read_int(LNG_STORED) / 100;
    s_last_update_offset = (int8_t)persist_read_int(UTC_STORED);
    s_last_update_date   = (int32_t)persist_read_int(LAST_DATE_STORED);
    s_last_update_time   = (int16_t)persist_read_int(LAST_TIME_STORED);
    s_last_update_dst    = (int8_t)persist_read_int(DST_STORED);

// Prime persistent storage keys with default values
    prime_settings();

// Update color parameters with chosen values from config page
    update_settings();

// Configure main window
    s_main_window = window_create();
    window_set_background_color(s_main_window, s_background_color);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load   = main_window_load,
        .unload = main_window_unload,
    });

// setup so clicking button aborts demo mode  
    window_set_click_config_provider(s_main_window, click_config_provider);
    window_stack_push(s_main_window, true);
  
    Layer *root = window_get_root_layer(s_main_window);
    s_bounds = layer_get_bounds(root);
    setup_time_layers(true);
  
#ifdef ENABLE_SCREENSHOT_MODE
    start_screenshot_mode();
#endif 
  
// Register for minute ticks
// For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
#ifdef ENABLE_TIME_MACHINE
    time_machine_tick_timer_service_subscribe(
        TM_TICK_UNIT,
        time_timer_tick
    );
    time_t now = time(NULL);
    s_current_time = *localtime(&now);

    time_machine_init(
        &s_current_time,
        TM_UNIT,
        TM_INTERVAL_MS
    );
#else
    tick_timer_service_subscribe(
        MINUTE_UNIT,
        time_timer_tick
    );
#endif
}

// Deinitialize watchface
static void deinit() {
    window_destroy(s_main_window);

// For accelerated time testing with "Time machine" utility from https://github.com/MorrisTimm/pebble-time-machine
// Turned off this original line:
//   tick_timer_service_unsubscribe();
#ifdef ENABLE_TIME_MACHINE
    time_machine_tick_timer_service_unsubscribe();
#else
    tick_timer_service_unsubscribe();
#endif

    app_message_deregister_callbacks();  // Destroy the callbacks for clean up
}

// Main routine
int main() {
    init();
    app_event_loop();
    deinit();
}