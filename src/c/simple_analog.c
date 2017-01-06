#include "simple_analog.h"

#include "pebble.h"

/* ---------------------------
  WatchFace For Pebble Basalt

sss
 vvv

 ------------------------------
*/
  
  
static Window *window;
static Layer *s_date_layer = NULL, *s_hands_layer = NULL, *s_watch_layer = NULL,*s_weather_layer = NULL,*s_measure_layer = NULL;
static TextLayer *s_day_label = NULL, *s_num_label = NULL,*s_batt_label = NULL,*s_weather_label = NULL,*s_measure_label = NULL,*s_weather_text_layer = NULL;

// bitmaplayer
static BitmapLayer *s_simple_bg_layer = NULL;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_weather_icon_layer = NULL;
static GBitmap *s_bmp_clear,*s_bmp_cloudy,*s_bmp_rainy,*s_bmp_snow,*s_bmp_thunder,*s_bmp_ready;



static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[10], s_day_buffer[32],s_batt_buffer[10];
static char s_weather_buffer[20]={"----"};
static char s_measure_buffer[20]={"---.-°C"};

// HttpRequest Send
static void send_cmd(void);
// Accelalation
static void tap_handler(AccelAxisType axis, int32_t direction) ;

static void bg_update_proc(Layer *layer, GContext *ctx) 
{  
//  graphics_context_set_fill_color(ctx, GColorBlack);
// test   graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
}

// ---------------------------------------------------------
// clock update draw
// ---------------------------------------------------------
static void hands_update_proc(Layer *layer, GContext *ctx) 
{
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

// ---------------------------------------------------------
// update normal info
// ---------------------------------------------------------
static void date_update_proc(Layer *layer, GContext *ctx) 
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);  
  // merge date
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(0, 2, 92, 22), 3,GCornersAll);
  graphics_draw_round_rect(ctx, GRect(0, 2, 92, 22), 3);
  strftime(s_day_buffer, sizeof(s_day_buffer), "%02m/%02d-%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  
  // battery
  BatteryChargeState batt;
  batt = battery_state_service_peek();  
  if( batt.is_charging == true )
  {
    graphics_context_set_fill_color(ctx, GColorRajah);
  }
  else if( batt.is_plugged == true )
  {
    graphics_context_set_fill_color(ctx, GColorRed);
  }
  graphics_fill_rect(ctx, GRect(144-51, 2, 51, 22), 3,GCornersAll);
  graphics_draw_round_rect(ctx, GRect(144-51, 2, 51, 22), 3);
  
  snprintf(s_batt_buffer, sizeof(s_batt_buffer), "%3d%%", batt.charge_percent);
  text_layer_set_text(s_batt_label, s_batt_buffer);
  
  // time
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(20, 168-29, 104, 29), 3,GCornersAll);
  graphics_draw_round_rect(ctx, GRect(20, 168-29, 104, 29), 3);
  strftime(s_num_buffer, sizeof(s_num_buffer), "%H:%M", t);
  text_layer_set_text(s_num_label, s_num_buffer);
  
  
}
// ---------------------------------------------------------
// time update handler
// ---------------------------------------------------------
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) 
{
  // update time  
  layer_mark_dirty(window_get_root_layer(window));
}

// ---------------------------------------------------------
// ------------------- create temparature ------------------ 
// ---------------------------------------------------------
static void create_temp(Layer* pParentLayer)
{
    // temp
  if( s_measure_label != NULL )
  {
    return;
  }
  
  // temp text
  s_measure_label = text_layer_create(GRect(2, (168/2), 144-(4), 32));  
#ifdef PBL_COLOR 
  text_layer_set_background_color(s_measure_label, GColorRajah );
  text_layer_set_text_color(s_measure_label, GColorBlack);  
#else
  text_layer_set_background_color(s_measure_label, GColorBlack);
  text_layer_set_text_color(s_measure_label, GColorWhite);
#endif
  text_layer_set_overflow_mode(s_measure_label, GTextOverflowModeFill );
  text_layer_set_text(s_measure_label, s_measure_buffer);
  text_layer_set_font(s_measure_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_measure_label,GTextAlignmentCenter);
  layer_add_child(pParentLayer, text_layer_get_layer(s_measure_label)); 
  
  
  // weather text
  s_weather_text_layer = text_layer_create(GRect(32, (168/2) - 32, 144-(36), 32));  
#ifdef PBL_COLOR 
  text_layer_set_background_color(s_weather_text_layer, GColorRajah );
  text_layer_set_text_color(s_weather_text_layer, GColorBlack);  
#else
  text_layer_set_background_color(s_weather_text_layer, GColorBlack);
  text_layer_set_text_color(s_weather_text_layer, GColorWhite);
#endif
//  text_layer_set_overflow_mode(s_weather_text_layer, GTextOverflowModeFill );
  text_layer_set_text(s_weather_text_layer, s_weather_buffer);
  text_layer_set_font(s_weather_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_weather_text_layer,GTextAlignmentCenter);
  layer_add_child(pParentLayer, text_layer_get_layer(s_weather_text_layer));   
}
// ---------------------------------------------------------
// create weather icon
// ---------------------------------------------------------
static void create_weather_icon(Layer* pParentLayer, GBitmap* pBmp)
{ 
  if(s_weather_icon_layer != NULL )
  {
    return;  
  }
  
  s_weather_icon_layer = bitmap_layer_create(GRect(2, (168/2) - 32, 32, 32));  
  bitmap_layer_set_bitmap(s_weather_icon_layer, pBmp);
  layer_add_child(pParentLayer, bitmap_layer_get_layer(s_weather_icon_layer)); 
}

// ---------------------------------------------------------
// hide weather window
// ---------------------------------------------------------
void hidden_weather_windows(void* pData)
{
    if(s_measure_label != NULL)
    {
      layer_set_hidden(text_layer_get_layer(s_measure_label), true);          
    }
  
    if( s_weather_icon_layer != NULL)
    {
      layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), true);          
    }
  
  
    if( s_weather_text_layer != NULL)
    {
        layer_set_hidden(text_layer_get_layer(s_weather_text_layer), true);  
    }
}


// ---------------------------------------------------------
// window setup proc
// ---------------------------------------------------------
static void window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Pictute
  // Watch board
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WATCH_FACE);
  s_simple_bg_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_simple_bg_layer, s_background_bitmap);
  bitmap_layer_set_background_color(s_simple_bg_layer,GColorBlack);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_simple_bg_layer));

  // Weather icons setup
  s_bmp_clear = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_CLEAR);
  s_bmp_rainy = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_RAINY);
  s_bmp_thunder = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_THUNDER);
  s_bmp_cloudy = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_CLOUDY);
  s_bmp_snow = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_SNOWY);
  s_bmp_ready = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_READY);  
    
  // clear instant layer
  s_weather_icon_layer = NULL;
  s_measure_label = NULL;

  // Date Layer Setup
  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  // day
  s_day_label = text_layer_create(GRect(0, -4, 92, 26));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_text_color(s_day_label, GColorWhite);
#else
  text_layer_set_text_color(s_day_label, GColorWhite);
#endif
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_day_label,GTextAlignmentCenter);
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));
  
  // Battery
  s_batt_label = text_layer_create(GRect(144-51, -4, 51, 26));
  text_layer_set_text(s_batt_label, s_batt_buffer);
  text_layer_set_background_color(s_batt_label, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_text_color(s_batt_label, GColorWhite);
#else
  text_layer_set_text_color(s_batt_label, GColorWhite);  
#endif
  text_layer_set_font(s_batt_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_batt_label,GTextAlignmentCenter);
  layer_add_child(s_date_layer, text_layer_get_layer(s_batt_label));  
    
  // Time
  s_num_label = text_layer_create(GRect(20, 168-33, 104, 32));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_text_color(s_num_label, GColorWhite);
#else
  text_layer_set_text_color(s_num_label, GColorWhite);  
#endif
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_num_label,GTextAlignmentCenter);
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  // analog time
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  // create temporary layer & hide
  create_temp(s_hands_layer);
  create_weather_icon(s_hands_layer, s_bmp_clear);
  layer_set_hidden(text_layer_get_layer(s_measure_label),true);
  layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), true);
  layer_set_hidden(text_layer_get_layer(s_weather_text_layer), true);  
    
}

static void window_unload(Window *window) 
{
  layer_destroy(bitmap_layer_get_layer(s_simple_bg_layer));
  layer_destroy(s_watch_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

// ---------------------------------------------------------
// initialize
// ---------------------------------------------------------
static void init() 
{
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);

  accel_tap_service_subscribe(tap_handler);
}

static void deinit() 
{
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  // 加速度センサの購読やめる
  accel_tap_service_unsubscribe();
  
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

// --------------------------------------
// Http Request proc
// --------------------------------------
enum WeatherKey {
  MESSAGE_TEMP_KEY = 0x00,         // TUPLE_CSTRING 
  MESSAGE_WEATHER_KEY = 0x01         // TUPLE_CSTRING 
};
 
static AppSync sync;
static uint8_t sync_buffer[64];
static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);

}

// split temp & weather from src string
static bool splitTemp(const char* pSrc, char** pTemp, char** pWeather, unsigned int* pWeatherCount)
{
  bool isFound = false;
  for(*pWeatherCount = 0; *pWeatherCount < strlen(pSrc); (*pWeatherCount)++)
  {
    if(pSrc[*pWeatherCount] == ',')
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "found , : %d", *pWeatherCount);
      isFound = true;
      break;
    }
  }

  // return separated text from source text
  if(isFound != false)
  {
    *pWeather =  (char*)&pSrc[0];    
    *pTemp = (char*)&pSrc[ (*pWeatherCount)+1 ];// Get Temp     
  }
  
  return isFound;
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Received --- !!");

  // error Check
  if( new_tuple == NULL || new_tuple->value == NULL || new_tuple->value->cstring == NULL || strlen(new_tuple->value->cstring) == 0)
  {
    return;
  }
  
  switch (key) {
    case MESSAGE_WEATHER_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "App Received key Weather: %s", new_tuple->value->cstring);
    
      // if weather is clear
      if( strncmp(new_tuple->value->cstring,"800",sizeof("800")) == 0)  
      {
       bitmap_layer_set_bitmap(s_weather_icon_layer, s_bmp_clear);        
      }
      else
      {
        switch(new_tuple->value->cstring[0])
        {
          case '2':  // Snow
          // fall through
          case '9':// Snow
            bitmap_layer_set_bitmap(s_weather_icon_layer, s_bmp_snow);
          break;
          case '3':// rain
          // fall through
          case '5':// rain
            bitmap_layer_set_bitmap(s_weather_icon_layer, s_bmp_rainy);
          break;
          case '7':// cloud
          // fall through
          case '8':// cloud
            bitmap_layer_set_bitmap(s_weather_icon_layer, s_bmp_cloudy);
          break;
          default:
            bitmap_layer_set_bitmap(s_weather_icon_layer, s_bmp_ready);
            break;
        }
        
      }
//      app_timer_register(5000, hidden_weather_windows, NULL );
      break;
    case MESSAGE_TEMP_KEY:
      // temp
      APP_LOG(APP_LOG_LEVEL_DEBUG, "App Received key temp: %s", new_tuple->value->cstring);
      
    
      // split temp and weather
      char* pTemp = NULL;
      char* pWeather = NULL;
      unsigned int weatherCount = 0;
      if( splitTemp(new_tuple->value->cstring, &pTemp, &pWeather, &weatherCount) != false)
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "disp temp & weather: %s _ %s", pWeather, pTemp);
  
        snprintf(s_measure_buffer, sizeof(s_measure_buffer),"%s",pTemp);        
        snprintf(s_weather_buffer, weatherCount+1, "%s",pWeather);    
      }
     // old text_layer_set_text(s_measure_label, s_measure_buffer);
      // display temporary window
      layer_set_hidden(text_layer_get_layer(s_measure_label), false);
      layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), false);   
      layer_set_hidden(text_layer_get_layer(s_weather_text_layer), false);  
    
      // start clear time
      app_timer_register(5000, hidden_weather_windows, NULL );
      break;
  }
  layer_mark_dirty(window_get_root_layer(window));
}
 
static void send_cmd(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "send_cmd - !!");

  Tuplet value = TupletInteger(1, 1);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    return;
  } 
  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  app_message_outbox_send();
}
     
// initialize Message
void initMessage()
{
  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
  Tuplet initial_values[] = {
    TupletCString(MESSAGE_WEATHER_KEY, ""),
    TupletCString(MESSAGE_TEMP_KEY, ""),    
  };
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);  
}  


// --------------------------------------
// Accelalation
// --------------------------------------
static void tap_handler(AccelAxisType axis, int32_t direction) 
{


  char acc[32];
  snprintf(acc, 32, "App Acc = %d", (int)direction);
  APP_LOG(APP_LOG_LEVEL_DEBUG, acc);

  // 天気を取得する
  send_cmd();
 }
// --------------------------------------
// closed loop
// --------------------------------------

int main() 
{
  init();
  initMessage();
  
  app_event_loop();
  deinit();
}
