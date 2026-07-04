#include <pebble.h>
#include "sliding_time.h"
#include "demo.h"
#include "suncalc.h"
#include "num2words-en.h"

#define BUFFER_SIZE 44
#define DEMO_FRAME_MS 33

// sun label layout tuning
static int s_demo_sun_dot_gap = 14;
static int s_demo_sun_label_width = 144;

// demo timing controls
static const float DEMO_INTRO_SECONDS        = 1.5f;
static const float DEMO_DAY_SECONDS          = 6.0f;
static const float DEMO_YEAR_SECONDS         = 14.0f;
static const float DEMO_TRANSITION_SECONDS   = 0.6f;

// derived frame counts
#define DEMO_INTRO_FRAMES \
    ((int)((DEMO_INTRO_SECONDS * 1000.0f) / DEMO_FRAME_MS))

#define DEMO_DAY_FRAMES \
    ((int)((DEMO_DAY_SECONDS * 1000.0f) / DEMO_FRAME_MS))

#define DEMO_YEAR_FRAMES \
    ((int)((DEMO_YEAR_SECONDS * 1000.0f) / DEMO_FRAME_MS))

#define DEMO_TRANSITION_FRAMES \
    ((int)((DEMO_TRANSITION_SECONDS * 1000.0f) / DEMO_FRAME_MS))

#define DEMO_TOTAL_FRAMES \
    (DEMO_INTRO_FRAMES + \
     DEMO_TRANSITION_FRAMES + \
     DEMO_DAY_FRAMES + \
     DEMO_TRANSITION_FRAMES + \
     DEMO_YEAR_FRAMES + \
     DEMO_TRANSITION_FRAMES)

// shared globals from main.c
extern Window *s_main_window;
extern Layer *s_back_layer;
extern Layer *s_scroll;
extern GRect s_bounds;
extern bool s_demo_running;
extern GColor s_background_color;
extern Line s_line1;
extern Line s_line2;
extern Line s_line3;
extern char s_line1_str[2][BUFFER_SIZE];
extern char s_line2_str[2][BUFFER_SIZE];
extern char s_line3_str[2][BUFFER_SIZE];
extern GColor s_text_day_color;
extern GColor s_text_date_color;
extern GColor s_time_indicator_color;
extern GColor s_sunrise_indicator_color;
extern GColor s_sunset_indicator_color;
extern GFont s_date_font;
extern int s_time_indicator_radius;
extern int s_time_indicator_thickness;
extern int s_sun_indicator_radius;
extern int s_time_line_spacing;
extern int s_date_line_height;
extern int s_date_line_spacing;
extern int s_date_lower_margin;
extern int s_date_side_margin;
extern int s_date_align;
extern int s_time_align;
extern int s_horizontal_slide_duration;
static int s_cached_chyron_width_real = 0;
static int s_cached_chyron_width_fake = 0;
extern float s_local_lat_deg;
extern float s_local_lng_deg;
extern int8_t s_last_update_offset;
extern int8_t s_last_update_dst;
extern uint32_t s_last_update_date;
extern const char* const MONTHS[];
extern const char* const DAYS[];
extern void animate_initial_time_in(void);
extern int get_line_x_offset(int rect_top, int rect_bottom, int margin, bool align_right);
extern bool s_screenshot_mode;
extern float s_screenshot_lat_deg;
extern float s_screenshot_lng_deg;
extern void display_initial_time(struct tm *t);
extern void adjust_timezone(float *time);
extern void cancel_all_watch_animations(void);
extern void light_enable_interaction(void);
extern void light_enable(bool enable);
extern void setup_time_layers(bool animate);
extern void apply_minute_font(bool midnight_mode);
extern GPoint time_to_rectangular_coords(int input_time, GRect bounds);

// local state
static Layer *s_demo_layer = NULL;
static AppTimer *s_demo_timer = NULL;
static TextLayer *s_chyron_layer = NULL;
static int s_demo_frame = 0;

// helpers
static void draw_time_indicator(
    GContext *ctx,
    float time_hours,
    int radius_override,
    GPoint center_override,
    bool use_override
) {

#if defined(PBL_ROUND)
    GPoint center =
        use_override
        ? center_override
        : grect_center_point(&s_bounds);

  int radius =
      use_override
      ? radius_override
      : s_bounds.size.w / 2;

    int32_t angle =
        (TRIG_MAX_ANGLE * time_hours) / 24;

    int y =
        radius * cos_lookup(angle) / TRIG_MAX_RATIO
        + center.y;

    int x =
        -radius * sin_lookup(angle) / TRIG_MAX_RATIO
        + center.x;
#else
    GPoint p =
        time_to_rectangular_coords(
            (int)(time_hours * 60),
            s_bounds
        );

    int x = p.x;
    int y = p.y;

#endif

    graphics_context_set_stroke_width(
        ctx,
        use_override
        ? s_time_indicator_thickness
        : s_time_indicator_thickness
      );
  
    graphics_context_set_stroke_color(
        ctx,
        s_time_indicator_color
      );

    graphics_draw_circle(
        ctx,
        GPoint(x, y),
        s_time_indicator_radius
    );
}

static void draw_sun_dot(
    GContext *ctx,
    float time_hours,
    GColor color,
    bool is_sunrise
) {

    int curr_x;
    int curr_y;

#if defined(PBL_ROUND)
    GPoint center = grect_center_point(&s_bounds);
    int radius = s_bounds.size.w / 2;
    
    int32_t angle =
        (TRIG_MAX_ANGLE * (time_hours / 24));

    curr_y =
        radius * cos_lookup(angle) / TRIG_MAX_RATIO
        + center.y;

    curr_x =
        -radius * sin_lookup(angle) / TRIG_MAX_RATIO
        + center.x;
#else
    GPoint p =
        time_to_rectangular_coords(
            (int)(time_hours * 60),
            s_bounds
        );

    curr_x = p.x;
    curr_y = p.y;
#endif

    graphics_context_set_fill_color(ctx, color);

    graphics_fill_circle(
        ctx,
        GPoint(curr_x, curr_y),
        s_sun_indicator_radius
    );

    graphics_context_set_text_color(ctx, color);

    int label_height = 
        s_date_line_height;

    int label_y =
        curr_y - (label_height / 2);

    if (label_y < 0) {
        label_y = 0;
      }

    if (label_y > s_bounds.size.h - label_height) {
        label_y = s_bounds.size.h - label_height;
      }

    int label_x;
    int label_width;
    GTextAlignment alignment;

    if (is_sunrise) {

// sunrise text to RIGHT of dot
        label_x =
            curr_x +
            s_demo_sun_dot_gap;

        label_width =
            s_demo_sun_label_width;

        alignment = GTextAlignmentLeft;

    } else {

// sunset text to LEFT of dot
        label_width =
            s_demo_sun_label_width;

        label_x =
            curr_x
            - s_demo_sun_dot_gap
            - label_width;

        alignment = GTextAlignmentRight;
    }
  
    graphics_draw_text(
        ctx,
        is_sunrise ? "sunrise" : "sunset",
        s_date_font,
        GRect(
            label_x,
            label_y,
            label_width,
            label_height
        ),
        GTextOverflowModeWordWrap,
        alignment,
        NULL
    );
}

// main renderer
static void demo_update_proc(
    Layer *layer,
    GContext *ctx
) {
// During intro, don't fill — let the real text layers show through.
// For all other phases, fill with background color.
    if (s_demo_frame >= DEMO_INTRO_FRAMES) {
        graphics_context_set_fill_color(
            ctx,
            s_background_color
        );

        graphics_fill_rect(
            ctx,
            s_bounds,
            0,
            GCornerNone
        );
    }

// intro animation
    if (s_demo_frame < DEMO_INTRO_FRAMES) {
        float p =
            (float)s_demo_frame /
            (float)DEMO_INTRO_FRAMES;

        GPoint screen_center =
            grect_center_point(&s_bounds);

        int start_radius = (int)(s_bounds.size.w * 1.3f);

        int end_radius =
            s_time_indicator_radius;

        int current_radius =
            start_radius -
            ((start_radius - end_radius) * p);

        int start_thickness = (s_time_indicator_thickness * start_radius) / s_time_indicator_radius;
        int end_thickness   = s_time_indicator_thickness;

        int thickness =
            start_thickness -
            (int)((start_thickness - end_thickness) * p);

        GPoint moving_center = GPoint(
            screen_center.x,
            screen_center.y +
            ((0 - screen_center.y) * p)
        );

        graphics_context_set_stroke_width(
            ctx,
            thickness
        );

        graphics_context_set_stroke_color(
            ctx,
            s_time_indicator_color
        );

        graphics_draw_circle(
            ctx,
            moving_center,
            current_radius - (thickness / 2)
        );
        return;
    }

// phase 2
    int phase2_start =
        DEMO_INTRO_FRAMES +
        DEMO_TRANSITION_FRAMES;

    int phase2_end =
        phase2_start +
        DEMO_DAY_FRAMES;

    if (s_demo_frame < phase2_end) {

        int elapsed =
            s_demo_frame - phase2_start;

// hold at noon during transition pause
        if (elapsed < DEMO_TRANSITION_FRAMES) {
            draw_time_indicator(
                ctx,
                12.0f,
                0,
                grect_center_point(&s_bounds),
                false
              );
            return;          }

        elapsed -= DEMO_TRANSITION_FRAMES;

        float p =
            (float)elapsed /
            (float)(DEMO_DAY_FRAMES - DEMO_TRANSITION_FRAMES);
      
        // exactly one full 24h cycle
        int total_minutes =
            (int)(p * 24.0f * 60.0f);

        int hour =
            (12 + total_minutes / 60) % 24;

        int minute =
            total_minutes % 60;

      draw_time_indicator(
            ctx,
            hour + minute / 60.0f,
            0,
            grect_center_point(&s_bounds),
            false
          );

// Round minute to nearest 10 to match what was previously drawn
        int display_minute = ((minute + 5) / 10) * 10;
        int display_hour = hour;
        if (display_minute >= 60) {
            display_minute = 0;
            display_hour = (display_hour + 1) % 24;
        }

        char l1[BUFFER_SIZE], l2[BUFFER_SIZE], l3[BUFFER_SIZE];
        time_to_3words(display_hour, display_minute, l1, l2, l3, BUFFER_SIZE);
        bool is_midnight = (display_hour == 0 && display_minute == 0);;

// Update text layers directly, no animation
        snprintf(s_line1_str[0], BUFFER_SIZE, "%s", l1);
        snprintf(s_line2_str[0], BUFFER_SIZE, "%s", l2);
        snprintf(s_line3_str[0], BUFFER_SIZE, "%s", l3);
        snprintf(s_line1_str[1], BUFFER_SIZE, "%s", l1);
        snprintf(s_line2_str[1], BUFFER_SIZE, "%s", l2);
        snprintf(s_line3_str[1], BUFFER_SIZE, "%s", l3);
        text_layer_set_text(s_line1.current_layer, s_line1_str[0]);
        text_layer_set_text(s_line2.current_layer, s_line2_str[0]);
        text_layer_set_text(s_line3.current_layer, s_line3_str[0]);
        apply_minute_font(is_midnight);

        return;
    }

// transition before yearly phase
    int yearly_start =
        phase2_end +
        DEMO_TRANSITION_FRAMES;

    if (s_demo_frame < yearly_start) {

// Hold final daily time indicator on screen
        draw_time_indicator(
            ctx,
            12.0f,
            0,
            grect_center_point(&s_bounds),
            false
        );
        return;
      }

// yearly phase
// Ensure text layers stay hidden during yearly phase, even if frames were skipped while app was obscured.
    if (s_demo_frame >= yearly_start) {
        layer_set_hidden(s_scroll, true);
      }
 
// Compiled binary became too big for OG Pebble (aplite) and watchface would crash immediately.  To reduce code size,
// we're going to skip the yearly phase of the demo on that platform only, and put up a message instead.
#ifdef PBL_PLATFORM_APLITE
    graphics_context_set_fill_color(ctx, s_background_color);
    graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);

    graphics_context_set_text_color(ctx, s_text_date_color);

    graphics_draw_text(
        ctx,
        "Sorry, OG Pebbles don't have enough memory for the sunrise/sunset seasonal drift segment of the demo.",
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
//        GRect(5, 10, s_bounds.size.w - 10, s_bounds.size.h - 20),
        GRect(0, 0, s_bounds.size.w, s_bounds.size.h),
      GTextOverflowModeWordWrap,
        GTextAlignmentCenter,
        NULL
    );
  
#else
    float lat = s_screenshot_mode ? s_screenshot_lat_deg : s_local_lat_deg;
    float lng = s_screenshot_mode ? s_screenshot_lng_deg : s_local_lng_deg;

    bool using_simulated_location = s_screenshot_mode ? false : (s_last_update_date == 0);

    if (using_simulated_location) {
        lat = 25;
        lng = 0;
      }

int yearly_frame =
    s_demo_frame - yearly_start;

// hold final yearly frame during outro pause
    if (yearly_frame > DEMO_YEAR_FRAMES - 1) {
        yearly_frame = DEMO_YEAR_FRAMES - 1;
      }

    float yearly_progress =
        (float)yearly_frame /
        (float)DEMO_YEAR_FRAMES;

    float sim_day_float =
        yearly_progress * 730.0f;

// clamp to valid range
    if (sim_day_float < 0) {
        sim_day_float = 0;
     }
    if (sim_day_float > 729) {
        sim_day_float = 729;
      }

    int text_width =
    using_simulated_location
    ? s_cached_chyron_width_fake
    : s_cached_chyron_width_real;

    int start_x = s_bounds.size.w;
    int end_x = -text_width;

    int chyron_x =
        start_x +
        (int)((end_x - start_x) * yearly_progress);

    if (s_chyron_layer) {
        GRect frame =
            layer_get_frame(
                text_layer_get_layer(s_chyron_layer)
            );

        frame.origin.x = chyron_x;

        layer_set_frame(
            text_layer_get_layer(s_chyron_layer),
            frame
        );
      }

    int sim_day =
        (int)sim_day_float;
  
    time_t now = time(NULL);
    struct tm *current_tm = localtime(&now);

    int base_year =
      current_tm->tm_year + 1900;
    int year =
      base_year + (sim_day / 365);

    int day_of_year =
        sim_day % 365;

    int month = 1;
    int day = 1 + day_of_year;

    static const int month_lengths[] = {
        31,28,31,30,31,30,
        31,31,30,31,30,31
    };

    while (
        month < 12 &&
        day > month_lengths[month - 1]
    ) {
        day -= month_lengths[month - 1];
        month++;
    }
  
    float sunrise =
        calcSunRise(
            year,
            month,
            day,
            lat,
            lng,
            91.0f
        );

    float sunset =
        calcSunSet(
            year,
            month,
            day,
            lat,
            lng,
            91.0f
        );

    adjust_timezone(&sunrise);
    adjust_timezone(&sunset);

    bool dst_on =
        (month > 3 && month < 11);

    if (month == 3 && day >= 9) {
        dst_on = true;
    }

    if (month == 11 && day >= 2) {
        dst_on = false;
    }

    if (dst_on) {
        sunrise += 1;
        sunset += 1;
    }

    bool valid_sunrise =
        (sunrise >= 0 && sunrise < 24);

    bool valid_sunset =
         (sunset >= 0 && sunset < 24);

    if (valid_sunrise) {
        draw_sun_dot(
            ctx,
            sunrise,
            s_sunrise_indicator_color,
            true
          );
    }

    if (valid_sunset) {
        draw_sun_dot(
            ctx,
            sunset,
            s_sunset_indicator_color,
            false
          );
    }
  
    char date_buf[32];
    char year_buf[16];

    GTextAlignment demo_date_alignment;
    if (s_date_align == 1) {
        demo_date_alignment = GTextAlignmentLeft;
    } else if (s_date_align == 3) {
        demo_date_alignment = GTextAlignmentRight;
    } else {
        demo_date_alignment = GTextAlignmentCenter;
    }

  int upper_x_offset = 0;
  int lower_x_offset = 0;
  
  if (s_date_align == 1) {
    upper_x_offset =
        get_line_x_offset(
            s_bounds.size.h -
            (s_date_lower_margin +
             s_date_line_spacing +
             s_date_line_height),

            s_bounds.size.h -
            (s_date_lower_margin +
             s_date_line_spacing),

            s_date_side_margin,
            false
        );

    lower_x_offset =
        get_line_x_offset(
            s_bounds.size.h -
            s_date_line_height -
            s_date_lower_margin,

            s_bounds.size.h -
            s_date_lower_margin,

            s_date_side_margin,
            false
        );

} else if (s_date_align == 3) {

    upper_x_offset =
        get_line_x_offset(
            s_bounds.size.h -
            (s_date_lower_margin +
             s_date_line_spacing +
             s_date_line_height),

            s_bounds.size.h -
            (s_date_lower_margin +
             s_date_line_spacing),

            s_date_side_margin,
            true
        );

    lower_x_offset =
        get_line_x_offset(
            s_bounds.size.h -
            s_date_line_height -
            s_date_lower_margin,

            s_bounds.size.h -
            s_date_lower_margin,

            s_date_side_margin,
            true
        );
}
  
      graphics_context_set_text_color(ctx, s_text_day_color);
      snprintf(
        date_buf,
        sizeof(date_buf),
        "%s%s",
        MONTHS[month - 1],
        DAYS[day]
    );

    graphics_draw_text(
        ctx,
        date_buf,
        s_date_font,
        GRect(
            upper_x_offset,
            s_bounds.size.h - (s_date_lower_margin + s_date_line_spacing + s_date_line_height),
            s_bounds.size.w,
            s_date_line_height
          ),
        GTextOverflowModeTrailingEllipsis,
        demo_date_alignment,
        NULL
      );
  
      graphics_context_set_text_color(ctx, s_text_date_color);
      snprintf(
        year_buf,
        sizeof(year_buf),
        "%d",
        year
    );

    graphics_draw_text(
        ctx,
        year_buf,
        s_date_font,
        GRect(
            lower_x_offset,
            s_bounds.size.h - (s_date_lower_margin + s_date_line_height),
            s_bounds.size.w,
            s_date_line_height
          ),
        GTextOverflowModeTrailingEllipsis,
        demo_date_alignment,
        NULL
      );
#endif
}

//keep backlight on for a moment after demo ends
static void delayed_backlight_off(void *data) {
    light_enable(false);
}

static void deferred_demo_abort(void *data) {
    demo_abort();
}

// timer
static void demo_timer_callback(void *data) {

    s_demo_timer = NULL;
    if (!s_demo_running) {
        return;
    }

    s_demo_frame++;

// Fire slide-in animation so it finishes exactly when intro ends
    int slide_trigger_frame =
        DEMO_INTRO_FRAMES -
        (s_horizontal_slide_duration / DEMO_FRAME_MS);

    if (s_demo_frame == slide_trigger_frame) {
        layer_set_hidden(s_scroll, false);
        animate_initial_time_in();
    }

    if (s_demo_layer) {
        layer_mark_dirty(s_demo_layer);
    }
  
    if (s_demo_frame >= DEMO_TOTAL_FRAMES) {
        s_demo_running = false;
        app_timer_register(
            1,
            deferred_demo_abort,
            NULL
          );

        return;
    }

    s_demo_timer =
        app_timer_register(
            DEMO_FRAME_MS,
            demo_timer_callback,
            NULL
        );
}

bool demo_is_running(void) {
    return s_demo_running;
}

void demo_start(void) {
  
    cancel_all_watch_animations();
  
    if (s_demo_running) {
        s_demo_running = false;
      if (s_demo_timer) {
          app_timer_cancel(s_demo_timer);
          s_demo_timer = NULL;
      }
      
        if (s_demo_layer) {
            layer_remove_from_parent(s_demo_layer);
            layer_destroy(s_demo_layer);
            s_demo_layer = NULL;
        }
      }
  
    s_demo_running = true;

// keep backlight on entire demo
    light_enable(true);
  
// Rebuild layers with correct positioning, no animation yet
    setup_time_layers(false);

// Cache chyron text widths once (expensive operation)
    const char *message_real =
        "This demo is using your actual location, and includes simulated jumps for daylight savings time (\"spring forward\" & \"fall back\"). Note sunrise varies over shorter range than sunset.";

    const char *message_fake =
        "Sunrise & sunset times based on simulated location, as yours has not yet been determined. Includes simulated jumps for daylight savings time (\"spring forward\" & \"fall back\"). Note sunrise varies over shorter range than sunset.";

    s_cached_chyron_width_real =
        graphics_text_layout_get_content_size(
            message_real,
            s_date_font,
            GRect(0, 0, 4000, 24),
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft
        ).w;

    s_cached_chyron_width_fake =
        graphics_text_layout_get_content_size(
            message_fake,
            s_date_font,
            GRect(0, 0, 4000, 24),
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft
        ).w;
  
    bool using_simulated_location = s_screenshot_mode ? false : (s_last_update_date == 0);
  
    const char *chyron_message = using_simulated_location ? message_fake : message_real;

    int chyron_width = using_simulated_location ? s_cached_chyron_width_fake : s_cached_chyron_width_real;

    if (s_chyron_layer) {
        text_layer_destroy(s_chyron_layer);
        s_chyron_layer = NULL;
    }

    s_chyron_layer = text_layer_create(
        GRect(
            s_bounds.size.w,
            (s_bounds.size.h / 2) - 15,
            chyron_width + 20,
            s_date_line_height + 6
        )
    );

    text_layer_set_background_color(
        s_chyron_layer,
        GColorClear
    );

    text_layer_set_text_color(
        s_chyron_layer,
        s_text_date_color
    );

    text_layer_set_font(
        s_chyron_layer,
        s_date_font
    );

    text_layer_set_text(
        s_chyron_layer,
        chyron_message
    );

    text_layer_set_overflow_mode(
        s_chyron_layer,
        GTextOverflowModeTrailingEllipsis
    );

    text_layer_set_text_alignment(
        s_chyron_layer,
        GTextAlignmentLeft
    );
  
    s_demo_frame = 0;

    layer_set_hidden(s_back_layer, true);

// Demo always uses 2-line time layout, so force scroll layer to centered 2-line position (unless user config is for top justified text).
    bool demo_use_vertical_center = (s_time_align > 3);

    if (demo_use_vertical_center) {
      GRect rect = layer_get_frame(s_scroll);
      rect.origin.y = s_time_line_spacing / 2;
      layer_set_frame(s_scroll, rect);
      }
  
// Keep text layers hidden until slide-in fires
    layer_set_hidden(s_scroll, true);
  
// Set all layers to "twelve noon" text
    char l1[BUFFER_SIZE], l2[BUFFER_SIZE], l3[BUFFER_SIZE];
    time_to_3words(12, 0, l1, l2, l3, BUFFER_SIZE);
    snprintf(s_line1_str[0], BUFFER_SIZE, "%s", l1);
    snprintf(s_line2_str[0], BUFFER_SIZE, "%s", l2);
    snprintf(s_line3_str[0], BUFFER_SIZE, "%s", l3);
    snprintf(s_line1_str[1], BUFFER_SIZE, "%s", l1);
    snprintf(s_line2_str[1], BUFFER_SIZE, "%s", l2);
    snprintf(s_line3_str[1], BUFFER_SIZE, "%s", l3);
    text_layer_set_text(s_line1.current_layer, s_line1_str[0]);
    text_layer_set_text(s_line2.current_layer, s_line2_str[0]);
    text_layer_set_text(s_line3.current_layer, s_line3_str[0]);

    s_demo_layer =
        layer_create(s_bounds);

    layer_set_update_proc(
        s_demo_layer,
        demo_update_proc
    );

    Layer *root =
        window_get_root_layer(
            s_main_window
        );

    layer_add_child(
        root,
        s_demo_layer
    );
  
    layer_add_child(
        root,
        text_layer_get_layer(s_chyron_layer)
    );
  
// Keep text layers on top of demo layer
    layer_remove_from_parent(s_scroll);
    layer_add_child(root, s_scroll);

    layer_mark_dirty(
        s_demo_layer
    );

    s_demo_timer =
        app_timer_register(
            DEMO_FRAME_MS,
            demo_timer_callback,
            NULL
        );
}

void demo_abort(void) {
    s_demo_running = false;

      if (s_demo_timer) {
        AppTimer *t = s_demo_timer;
        s_demo_timer = NULL;
        app_timer_cancel(t);
    }

    if (s_demo_layer) {
        layer_remove_from_parent(
            s_demo_layer
        );

        layer_destroy(
            s_demo_layer
        );

        s_demo_layer = NULL;
    }
 
    if (s_chyron_layer) {
        text_layer_destroy(s_chyron_layer);
        s_chyron_layer = NULL;
   }

    layer_set_hidden(
        s_back_layer,
        false
    );

    layer_set_hidden(
        s_scroll,
        false
    );

    layer_mark_dirty(
        s_back_layer
    );

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    if (t) {
        setup_time_layers(true);
    }
  
// turn off backlight a moment after demo ends.
    app_timer_register(
        800,
        delayed_backlight_off,
      NULL
      );
}