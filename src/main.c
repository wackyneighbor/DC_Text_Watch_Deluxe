#include <pebble.h>
#include "my_math.h"
#include "suncalc.h"
#include "num2words-en.h"
#define BUFFER_SIZE 44

static const char* const MONTHS[] = {
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
	
static const char* const DAYS[] = {
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

typedef struct {
	TextLayer *currentLayer;
	TextLayer *nextLayer;	
	PropertyAnimation *currentAnimation;
	PropertyAnimation *nextAnimation;
} Line;

Line line1;
Line line2;
Line line3;

GRect bounds;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];

static Window *s_main_window;

Layer *back_layer;
Layer *scroll;
PropertyAnimation *scroll_down;
PropertyAnimation *scroll_up;

static bool PoppedDownNow;
static bool PoppedDownAtInit;

static GColor BACKGROUND_COLOR;
static GColor TEXT_LINE_1_COLOR;
static GColor TEXT_LINE_2_COLOR;
static GColor TEXT_LINE_3_COLOR;
static GColor TEXT_DAY_COLOR;
static GColor TEXT_DATE_COLOR;
static GColor TIME_INDICATOR_COLOR;
static GColor SUNRISE_INDICATOR_COLOR;
static GColor SUNSET_INDICATOR_COLOR;

// Global variable declarations
static float localLat_deg;		// Declare the global variable of the last known latitude
static float localLng_deg;		// Declare the global variable of the last known longitude
static uint16_t lastUpdateTime;	// Declare a global variable for the last GPS update time
static uint32_t lastUpdateDate;	// Declare a global variable for the last GPS update date
static int8_t lastUpdateOffset;	// Declare a global variable for the number of hours offset from UTC at time of last GPS update
static int8_t lastUpdateDST;	// Declare a global variable for if DST was active at last GPS update

// Javascript dictionary keys for GPS get & config updates
enum {
	KEY_LATITUDE = 1,
	KEY_LONGITUDE = 2,
	KEY_USEOLDDATA = 3,
	KEY_UTCh = 4,
	KEY_SINGLE_PREFIX_TYPE = 5,
	KEY_BACKGROUND_COLOR = 6,
	KEY_TEXT_LINE_1_COLOR = 7,
	KEY_TEXT_LINE_2_COLOR = 8,
	KEY_TEXT_LINE_3_COLOR = 9,
	KEY_TEXT_DAY_COLOR = 10,
	KEY_TEXT_DATE_COLOR = 11,
	KEY_TIME_INDICATOR_COLOR = 12,
	KEY_SUNRISE_INDICATOR_COLOR = 13,
	KEY_SUNSET_INDICATOR_COLOR = 14
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
	SHOW_Oh_PREFIX_STORED = 7,
	BACKGROUND_COLOR_STORED = 8,
	TEXT_LINE_1_COLOR_STORED = 9,
	TEXT_LINE_2_COLOR_STORED = 10,
	TEXT_LINE_3_COLOR_STORED = 11,
	TEXT_DAY_COLOR_STORED = 12,
	TEXT_DATE_COLOR_STORED = 13,
	TIME_INDICATOR_COLOR_STORED = 14,
	SUNRISE_INDICATOR_COLOR_STORED = 15,
	SUNSET_INDICATOR_COLOR_STORED = 16
};

// GPS update procedure
static void update_GPS() {
	// Begin dictionary
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	// Add a key-value pair
	dict_write_uint8(iter, 0, 0);
	// Send the message!
	app_message_outbox_send();
}

// time zone correction needed for sunrise and sunset times
void adjustTimezone(float* time) {
	*time += lastUpdateOffset;
	if (*time > 24) *time -= 24;
	if (*time < 0) *time += 24;
}

// Draw time indicators
static void back_update_proc(Layer *layer, GContext *ctx) {

	time_t now = time(NULL);
	struct tm *t   = localtime(&now);
	
	GRect bounds = layer_get_bounds(layer);
	GPoint center = grect_center_point(&bounds);

	int currY;
	int currX;
	int radius = bounds.size.w/2;

	// Update location if it's never been determined, and at the top of the hour thereafter
	if(t->tm_min == 0 || lastUpdateTime == 0){
	update_GPS();
	}
	
	// Display sunrise & sunset indicators, but only if location has been succesfully determined
	if(lastUpdateDate != 0){
	
		// Calculate sunrise & set times
		float sunriseTime = calcSunRise(t->tm_year, t->tm_mon+1, t->tm_mday,  localLat_deg, localLng_deg, 91.0f);
		float sunsetTime = calcSunSet(t->tm_year, t->tm_mon+1, t->tm_mday, localLat_deg, localLng_deg, 91.0f);

		// Only proceed with drawing sunrise indicator if there is a sunrise today.
		if (sunriseTime != 100){
		
			// Correct sunrise time in case Daylight Savings Time was changed since last GPS update
			if (lastUpdateDST < t->tm_isdst){
				sunriseTime+=1;
			} else if (lastUpdateDST > t->tm_isdst){
				sunriseTime-=1;
			};
	
			// Fix time zone of sunrise
			adjustTimezone(&sunriseTime);

			// Display sunrise indicator
			int32_t sunrise_angle = (TRIG_MAX_ANGLE * (sunriseTime/24));
			currY = radius * cos_lookup(sunrise_angle) / TRIG_MAX_RATIO + center.y;
			currX = -radius * sin_lookup(sunrise_angle) / TRIG_MAX_RATIO + center.x;
			graphics_context_set_fill_color(ctx, SUNRISE_INDICATOR_COLOR); graphics_fill_circle(ctx, GPoint(currX, currY), 6);
		}
		
		// Only proceed with drawing sunset indicator if there is a sunset today.
		if (sunsetTime != 100){
		
			// Correct sunset time in case Daylight Savings Time was changed since last GPS update
			if (lastUpdateDST < t->tm_isdst){
				sunsetTime+=1;
			} else if (lastUpdateDST > t->tm_isdst){
				sunsetTime-=1;
			};
	
			// Fix time zone of sunrise
			adjustTimezone(&sunsetTime);

			// Display sunset indicator
			int32_t sunset_angle = (TRIG_MAX_ANGLE * (sunsetTime/24));
			currY = radius * cos_lookup(sunset_angle) / TRIG_MAX_RATIO + center.y;
			currX = -radius * sin_lookup(sunset_angle) / TRIG_MAX_RATIO + center.x;
			graphics_context_set_fill_color(ctx, SUNSET_INDICATOR_COLOR); graphics_fill_circle(ctx, GPoint(currX, currY), 6);
		}
	}

	// Display current time indicator
	int32_t hour_angle = (TRIG_MAX_ANGLE * ((t->tm_hour * 60) + t->tm_min)/1440);
	currY = radius * cos_lookup(hour_angle) / TRIG_MAX_RATIO + center.y;
	currX = -radius * sin_lookup(hour_angle) / TRIG_MAX_RATIO + center.x;
	graphics_context_set_stroke_width(ctx, 2);
	graphics_context_set_stroke_color(ctx, TIME_INDICATOR_COLOR);
    graphics_draw_circle(ctx, GPoint(currX, currY), 9);

	// Display day of week
	char tempstring;
	strftime(&tempstring, 10, "%A", t);
	graphics_context_set_text_color(ctx, TEXT_DAY_COLOR);
	graphics_draw_text(ctx, &tempstring, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
					   GRect(0, bounds.size.h - 48, 180, 30), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

	// Display date
    strcpy(&tempstring, MONTHS[t->tm_mon]);
    strcat(&tempstring, DAYS[t->tm_mday]);
	graphics_context_set_text_color(ctx, TEXT_DATE_COLOR);
	graphics_draw_text(ctx, &tempstring, fonts_get_system_font(FONT_KEY_GOTHIC_24), 
					   GRect(0, bounds.size.h - 30, 180, 30), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// Text Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context) {
	Layer *current = (Layer *)context;
	GRect rect = layer_get_frame(current);
	rect.origin.x = bounds.size.w;
	layer_set_frame(current, rect);
}

// Animate text line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next) {
	if (line->nextAnimation != NULL)
		property_animation_destroy(line->nextAnimation);

	if (line->currentAnimation != NULL)
		property_animation_destroy(line->currentAnimation);

	GRect rect = layer_get_frame((Layer *)next);
	rect.origin.x -= bounds.size.w;
	
	line->nextAnimation = property_animation_create_layer_frame((Layer *)next, NULL, &rect);
	animation_set_duration(property_animation_get_animation(line->nextAnimation), 400);
	animation_set_curve(property_animation_get_animation(line->nextAnimation), AnimationCurveEaseOut);

	animation_schedule(property_animation_get_animation(line->nextAnimation));
	
	GRect rect2 = layer_get_frame((Layer *)current);
	rect2.origin.x -= bounds.size.w;
	
	line->currentAnimation = property_animation_create_layer_frame((Layer *)current, NULL, &rect2);
	animation_set_duration(property_animation_get_animation(line->currentAnimation), 400);
	animation_set_curve(property_animation_get_animation(line->currentAnimation), AnimationCurveEaseOut);
	
	animation_set_handlers(property_animation_get_animation(line->currentAnimation), (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, current);
	
	animation_schedule(property_animation_get_animation(line->currentAnimation));
}

// Update text line
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value) {
	TextLayer *next, *current;
	
	GRect rect = layer_get_frame((Layer *)line->currentLayer);
	current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
	next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;
	
	// Update correct text only
	if (current == line->currentLayer) {
		memset(lineStr[1], 0, BUFFER_SIZE);
		memcpy(lineStr[1], value, strlen(value));
		text_layer_set_text(next, lineStr[1]);
	} else {
		memset(lineStr[0], 0, BUFFER_SIZE);
		memcpy(lineStr[0], value, strlen(value));
		text_layer_set_text(next, lineStr[0]);
	}
	
	makeAnimationsForLayers(line, current, next);
}

// Check to see if the current text line needs to be updated
bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue) {
	char *currentStr;
	GRect rect = layer_get_frame((Layer *)line->currentLayer);
	currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

	if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
		(strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
		return true;
	}
	return false;
}

// Update screen based on new time
void display_time(struct tm *t) {
	// The current time text will be stored in the following 3 strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	char textLine3[BUFFER_SIZE];
	
	time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);
	
	if (needToUpdateLine(&line1, line1Str, textLine1)) {
		updateLineTo(&line1, line1Str, textLine1);	
	}
	if (needToUpdateLine(&line2, line2Str, textLine2)) {
		updateLineTo(&line2, line2Str, textLine2);	
	}
	if (needToUpdateLine(&line3, line3Str, textLine3)) {
		updateLineTo(&line3, line3Str, textLine3);	
	}
}

// Update screen without animation first time we start the watchface
void display_initial_time(struct tm *t) {
	time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);
        
	text_layer_set_text(line1.currentLayer, line1Str[0]);
	text_layer_set_text(line2.currentLayer, line2Str[0]);
	text_layer_set_text(line3.currentLayer, line3Str[0]);
}

// Pop down to center before initial display when only 2 lines of text
void makePopDown(){
	
	GRect rect = layer_get_bounds((Layer *)scroll);
	rect.origin.y = 21;
	layer_set_bounds(scroll, rect);
}

// Animate down to recenter when going from 3 lines of text to only 2 lines of text
void makeScrollDown(){

	GRect from = layer_get_bounds((Layer *)scroll);
	GRect to = layer_get_bounds((Layer *)scroll);

	if(PoppedDownAtInit == true){
		from.origin.y = -21;
		to.origin.y = 0;
	} else {
		from.origin.y = 0;
		to.origin.y = 21;
	}
	
	scroll_down = property_animation_create_layer_frame((Layer *)scroll, &from, &to);
	animation_set_duration(property_animation_get_animation(scroll_down), 800);
	animation_set_delay(property_animation_get_animation(scroll_down), 600);
	animation_set_curve(property_animation_get_animation(scroll_down), AnimationCurveEaseOut);
	animation_schedule(property_animation_get_animation(scroll_down));
}

// Animate back up to prepare for return of 3 lines of text
//     sets delay based on number of seconds left in minute
//     must not call this function if not enough time left in minute to do it...
void makeScrollUp(struct tm *t){

	GRect from = layer_get_bounds((Layer *)scroll);
	GRect to = layer_get_bounds((Layer *)scroll);

	GRect rect = layer_get_bounds((Layer *)scroll);
	if(rect.origin.y == 21){
		from.origin.y = 0;
		to.origin.y = -21;
	} else {
		from.origin.y = 21;
		to.origin.y = 0;
	}
	
	scroll_down = property_animation_create_layer_frame((Layer *)scroll, &from, &to);
	animation_set_duration(property_animation_get_animation(scroll_down), 800);
	animation_set_delay(property_animation_get_animation(scroll_down), (59000-1000*t->tm_sec));
	animation_set_curve(property_animation_get_animation(scroll_down), AnimationCurveEaseOut);
	animation_schedule(property_animation_get_animation(scroll_down));

	// reset PoppedDown indicator
	PoppedDownNow = false;
}

// Configure the first line of text
void configureLine1Layer(TextLayer *textlayer, bool right) {
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_color(textlayer, TEXT_LINE_1_COLOR);
	text_layer_set_background_color(textlayer, GColorClear);
        if (right) {
          text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
        } else {
          text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
        }
}

// Configure for the 2nd line of text
void configureLine2Layer(TextLayer *textlayer, bool right) {
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	text_layer_set_text_color(textlayer, TEXT_LINE_2_COLOR);
	text_layer_set_background_color(textlayer, GColorClear);
        if (right) {
          text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
        } else {
          text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
        }
}

// Configure for the 3nd line of text
void configureLine3Layer(TextLayer *textlayer, bool right) {
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	text_layer_set_text_color(textlayer, TEXT_LINE_3_COLOR);
	text_layer_set_background_color(textlayer, GColorClear);
        if (right) {
          text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
        } else {
          text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
        }
}

// Set up default configuration settings (colors & single digit prefix),
// for when user hasn't changed anything yet
static void prime_settings(){

	if (persist_exists(SHOW_O_PREFIX_STORED) == false){
		persist_write_bool(SHOW_O_PREFIX_STORED, true); // Show O' prefix by default
	}
	if (persist_exists(SHOW_Oh_PREFIX_STORED) == false){
		persist_write_bool(SHOW_Oh_PREFIX_STORED, false); // Don't show Oh prefix by default
	}
	if (persist_exists(BACKGROUND_COLOR_STORED) == false){
		persist_write_int(BACKGROUND_COLOR_STORED, 0x000000); //GColorBlack
	}
	if (persist_exists(TEXT_LINE_1_COLOR_STORED) == false){
		persist_write_int(TEXT_LINE_1_COLOR_STORED, 0xFFFFFF); //GColorWhite
	}
	if (persist_exists(TEXT_LINE_2_COLOR_STORED) == false){
		persist_write_int(TEXT_LINE_2_COLOR_STORED, 0xFFFFFF); //GColorWhite
	}
	if (persist_exists(TEXT_LINE_3_COLOR_STORED) == false){
		persist_write_int(TEXT_LINE_3_COLOR_STORED, 0xFFFFFF); //GColorWhite
	}
	if (persist_exists(TEXT_DAY_COLOR_STORED) == false){
		persist_write_int(TEXT_DAY_COLOR_STORED, 0xFFFFFF); //GColorWhite
	}
	if (persist_exists(TEXT_DATE_COLOR_STORED) == false){
		persist_write_int(TEXT_DATE_COLOR_STORED, 0xFFFFFF); //GColorWhite
	}
	if (persist_exists(TIME_INDICATOR_COLOR_STORED) == false){
		persist_write_int(TIME_INDICATOR_COLOR_STORED, 0xAAAA00); //GColorLimerick
	}
	if (persist_exists(SUNRISE_INDICATOR_COLOR_STORED) == false){
		persist_write_int(SUNRISE_INDICATOR_COLOR_STORED, 0xAAAAAA); //GColorLightGray
	}
	if (persist_exists(SUNSET_INDICATOR_COLOR_STORED) == false){
		persist_write_int(SUNSET_INDICATOR_COLOR_STORED, 0xAAAAAA); //GColorLightGray
	}
}

// Update color settings settings based on config page choices.
static void update_settings(){
	BACKGROUND_COLOR = GColorFromHEX(persist_read_int(BACKGROUND_COLOR_STORED));
	TEXT_LINE_1_COLOR = GColorFromHEX(persist_read_int(TEXT_LINE_1_COLOR_STORED));
	TEXT_LINE_2_COLOR = GColorFromHEX(persist_read_int(TEXT_LINE_2_COLOR_STORED));
	TEXT_LINE_3_COLOR = GColorFromHEX(persist_read_int(TEXT_LINE_3_COLOR_STORED));
	TEXT_DAY_COLOR = GColorFromHEX(persist_read_int(TEXT_DAY_COLOR_STORED));
	TEXT_DATE_COLOR = GColorFromHEX(persist_read_int(TEXT_DATE_COLOR_STORED));
	TIME_INDICATOR_COLOR = GColorFromHEX(persist_read_int(TIME_INDICATOR_COLOR_STORED));
	SUNRISE_INDICATOR_COLOR = GColorFromHEX(persist_read_int(SUNRISE_INDICATOR_COLOR_STORED));
	SUNSET_INDICATOR_COLOR = GColorFromHEX(persist_read_int(SUNSET_INDICATOR_COLOR_STORED));
}

// App communication with phone 
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// This is the main function that operates when a callback happens for the GPS or config update request
	// Store incoming information
	int16_t newLatitude;
	int16_t newLongitude;
	int8_t useNewData = 0;		//Assume using old data until proven otherwise
	int8_t newUToffset;

	// Read first item
	Tuple *t;
	t = dict_read_first(iterator);

	// For all items
	while(t != NULL) {
		// Which key was received?
		switch(t->key) {
			case 0:
			// Key 0 is the first entry in the dictionary, skipped for values as I believe that was the source of problems..
			break;
			case KEY_LATITUDE:
			newLatitude = t->value->int32;
			break;
			case KEY_LONGITUDE:
			newLongitude = (int)t->value->int32;
			break;
      		case KEY_USEOLDDATA:
        	useNewData = (int)t->value->int32;
        	break;
      		case KEY_UTCh:
        	newUToffset = (int)t->value->int32;
			break;
			case KEY_SINGLE_PREFIX_TYPE:
			if (t->value->uint8==1){
				persist_write_bool(SHOW_O_PREFIX_STORED, false);
				persist_write_bool(SHOW_Oh_PREFIX_STORED, false);
			} else if (t->value->uint8==2){
				persist_write_bool(SHOW_O_PREFIX_STORED, true);
				persist_write_bool(SHOW_Oh_PREFIX_STORED, false);		
			} else if (t->value->uint8==3){
				persist_write_bool(SHOW_O_PREFIX_STORED, false);
				persist_write_bool(SHOW_Oh_PREFIX_STORED, true);		
			}
			break;
			case KEY_BACKGROUND_COLOR:
			persist_write_int(BACKGROUND_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TEXT_LINE_1_COLOR:
			persist_write_int(TEXT_LINE_1_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TEXT_LINE_2_COLOR:
			persist_write_int(TEXT_LINE_2_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TEXT_LINE_3_COLOR:
			persist_write_int(TEXT_LINE_3_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TEXT_DAY_COLOR:
			persist_write_int(TEXT_DAY_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TEXT_DATE_COLOR:
			persist_write_int(TEXT_DATE_COLOR_STORED, t->value->uint32);
			break;
			case KEY_TIME_INDICATOR_COLOR:
			persist_write_int(TIME_INDICATOR_COLOR_STORED, t->value->uint32);
			break;
			case KEY_SUNRISE_INDICATOR_COLOR:
			persist_write_int(SUNRISE_INDICATOR_COLOR_STORED, t->value->uint32);
			break;
			case KEY_SUNSET_INDICATOR_COLOR:
			persist_write_int(SUNSET_INDICATOR_COLOR_STORED, t->value->uint32);
			break;
			default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
		}
		
		// Look for next item
		t = dict_read_next(iterator);
	}	//Transer the incoming information into the stores

	// Set the latitude and longitude global variables
	if (useNewData == 0) {
		// No new position data available, use old data
		// Latitude, longitude, UTC offset, and DST indicator remain unchanged

	} else {
		// New position data available, update global variables. Lat, long, and UTC must remain as flexible variables,
		// Rise values can be directly accessed through storage
		// Last update time can be updated to now as well
		
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		
		lastUpdateTime = (t->tm_hour) * 60 + (t->tm_min);
		lastUpdateDate = (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;    //Alterations account for time.h functions
		localLat_deg = (float)newLatitude/100;
		localLng_deg = (float)newLongitude/100;
		lastUpdateOffset = newUToffset;
		lastUpdateDST = t->tm_isdst;
				
		persist_write_int(LAT_STORED, newLatitude);    //Already as an integer (must / 100 when retrieving)
		persist_write_int(LNG_STORED, newLongitude);
		persist_write_int(UTC_STORED, newUToffset);
		persist_write_int(LAST_TIME_STORED, lastUpdateTime);
		persist_write_int(LAST_DATE_STORED, lastUpdateDate);
		persist_write_int(DST_STORED, lastUpdateDST);
	}

	// Update variables with newly received settings, and apply color changes to background, back layer, and text layers
	update_settings();
	window_set_background_color(s_main_window, BACKGROUND_COLOR);
	layer_mark_dirty(back_layer);
	text_layer_set_text_color(line1.currentLayer, TEXT_LINE_1_COLOR);
	text_layer_set_text_color(line1.nextLayer, TEXT_LINE_1_COLOR);
	text_layer_set_text_color(line2.currentLayer, TEXT_LINE_2_COLOR);
	text_layer_set_text_color(line2.nextLayer, TEXT_LINE_2_COLOR);
	text_layer_set_text_color(line3.currentLayer, TEXT_LINE_3_COLOR);
	text_layer_set_text_color(line3.nextLayer, TEXT_LINE_1_COLOR);
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {}

// Update graphics when timer ticks
static void time_timer_tick(struct tm *t, TimeUnits units_changed) {

	// Update back layer (indicators for current time, sunrise, sunset, day of week, and date)
	if (units_changed & MINUTE_UNIT ) {
		layer_mark_dirty(back_layer);  
	}
	
	// Update text time
	display_time(t);
	
	// Recenter screen if last time was 3 lines, but new time is 2 lines
	// Don't do this if time was just initialized already centered
	// Consider that there are fewer times where there are only 2 lines when "Oh" option is activated
	if (persist_read_bool(SHOW_Oh_PREFIX_STORED) == false){
		if(t->tm_min == 0 || t->tm_min == 15 || t->tm_min == 20 || t->tm_min == 30 || t->tm_min == 40 || t->tm_min == 50){
			if(PoppedDownNow == false){
				makeScrollDown();
			}
		}
	
		// Prepare for next time being 3 lines, if current time is 2 lines
		if(t->tm_min == 13 || t->tm_min == 16 || t->tm_min == 20 || t->tm_min == 30 || t->tm_min == 40 || t->tm_min == 50){
			makeScrollUp(t);
			}
		} else {

		// last time was 3 lines, new time is 2 lines, for when "Oh" option activated
		if(t->tm_min == 0 || t->tm_min == 10 || t->tm_min == 15 || t->tm_min == 20 || t->tm_min == 30 || t->tm_min == 40 || t->tm_min == 50){
			if(PoppedDownNow == false){
				makeScrollDown();
			}
		}
	
		// Prepare for next time being 3 lines, if current time is 2 lines, for when "Oh" option activated
		if(t->tm_min == 0 || t->tm_min == 13 || t->tm_min == 16 || t->tm_min == 20 || t->tm_min == 30 || t->tm_min == 40 || t->tm_min == 50){
			makeScrollUp(t);
			}
	}
}

// Configure main window
static void main_window_load(Window *window) {

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	window_set_background_color(s_main_window, BACKGROUND_COLOR);
	
	back_layer = layer_create(bounds);
	layer_set_update_proc(back_layer, back_update_proc);

	layer_add_child(window_layer, back_layer);  
}

// Deconfigure main window
static void main_window_unload(Window *window) {
   
  layer_destroy(back_layer);
}

// Initialize watchface
static void init() {
	
	// Register the callbacks for when a GPS request is made
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	// Open AppMessage
	app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

	// Initialize some of the global values
	localLat_deg = (float)persist_read_int(LAT_STORED) / 100;
	localLng_deg = (float)persist_read_int(LNG_STORED) / 100;
	lastUpdateOffset = (int8_t)persist_read_int(UTC_STORED);
	lastUpdateDate = (int32_t)persist_read_int(LAST_DATE_STORED);
	lastUpdateTime = (int16_t)persist_read_int(LAST_TIME_STORED);
	lastUpdateDST = (int8_t)persist_read_int(DST_STORED);

	// Prime persistent storage keys with default values
	prime_settings();
	
	// Update colors parameters with chosen values from config page
	update_settings();
	
	// Configure main window
	s_main_window = window_create();
	window_set_background_color(s_main_window, BACKGROUND_COLOR);
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});

	window_stack_push(s_main_window, true);

	Layer *root = window_get_root_layer(s_main_window);
		bounds = layer_get_bounds(root);
	int offset = (bounds.size.h - 145) / 2;
	
	scroll = layer_create(bounds);
	
	// 1st line layer
	line1.currentLayer = text_layer_create(GRect(bounds.origin.x, offset, bounds.size.w, 50));
	line1.nextLayer = text_layer_create(GRect(bounds.size.w, offset, bounds.size.w, 50));
	configureLine1Layer(line1.currentLayer, false);
	configureLine1Layer(line1.nextLayer, false);

	// 2nd line layer
	line2.currentLayer = text_layer_create(
                        GRect(0, 37 + offset, bounds.size.w, 50));
	line2.nextLayer = text_layer_create(
                        GRect(bounds.size.w, 37 + offset, bounds.size.w, 50));
	configureLine2Layer(line2.currentLayer, false);
	configureLine2Layer(line2.nextLayer, false);

	// 3rd line layer
	line3.currentLayer = text_layer_create(
                        GRect(0, 74 + offset, bounds.size.w, 50));
	line3.nextLayer = text_layer_create(
                        GRect(bounds.size.w, 74 + offset, bounds.size.w, 50));
	configureLine3Layer(line3.currentLayer, false);
	configureLine3Layer(line3.nextLayer, false);

	// Configure text time on init
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	display_time(t);
	

	// Load layers
	layer_add_child(root, scroll);
	layer_add_child(scroll, (Layer *)line1.currentLayer);
	layer_add_child(scroll, (Layer *)line1.nextLayer);
	layer_add_child(scroll, (Layer *)line2.currentLayer);
	layer_add_child(scroll, (Layer *)line2.nextLayer);
	layer_add_child(scroll, (Layer *)line3.currentLayer);
	layer_add_child(scroll, (Layer *)line3.nextLayer);

	// Register for minute ticks
	tick_timer_service_subscribe(MINUTE_UNIT, time_timer_tick);
	
	// inititialize PoppedDown indicators
	PoppedDownNow = false;
	PoppedDownAtInit = false;
	
// If initial display of time is only 2 lines of text, display centered
//  Consider that fewer times have only 2 lines if "Oh" options is activated
	if (persist_read_bool(SHOW_Oh_PREFIX_STORED) == false){
		if(t->tm_min == 0 || t->tm_min == 1 || t->tm_min == 2 || t->tm_min == 3 || t->tm_min == 4 || t->tm_min == 5 ||
		   t->tm_min == 6 || t->tm_min == 7 || t->tm_min == 8 || t->tm_min == 9 || t->tm_min == 10 || t->tm_min == 11 ||
		   t->tm_min == 12 || t->tm_min == 13 || t->tm_min == 15 || t->tm_min == 16 || t->tm_min == 20 ||
		   t->tm_min == 30 || t->tm_min == 40 || t->tm_min == 50){
			makePopDown();
			
			// signal that this has been done, so an extra animation isn't triggered, and the down animation knows the right
			// starting place
			PoppedDownNow = true;
			PoppedDownAtInit = true;
		}
	} else {
		if(t->tm_min == 0 || t->tm_min == 10 || t->tm_min == 11 || t->tm_min == 12 || t->tm_min == 13 ||
		   t->tm_min == 15 || t->tm_min == 16 || t->tm_min == 20 || t->tm_min == 30 || t->tm_min == 40 ||
		   t->tm_min == 50){
			makePopDown();
			
			// signal that this has been done, so an extra animation isn't triggered, and the down animation knows the right
			// starting place
			PoppedDownNow = true;
			PoppedDownAtInit = true;
		}
	}
}

// Deinitialize watchface
static void deinit() {
	window_destroy(s_main_window);
	tick_timer_service_unsubscribe();
	app_message_deregister_callbacks();    //Destroy the callbacks for clean up
	text_layer_destroy(line1.currentLayer);
	text_layer_destroy(line1.nextLayer);
	text_layer_destroy(line2.currentLayer);
	text_layer_destroy(line2.nextLayer);
	text_layer_destroy(line3.currentLayer);
	text_layer_destroy(line3.nextLayer);
	layer_destroy(scroll);
}

// Main routine
int main() {
	init();
	app_event_loop();
	deinit();
}