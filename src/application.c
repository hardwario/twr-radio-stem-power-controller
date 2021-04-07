#include <application.h>

#define SEGMENTS (1 + 10) // lector + students
#define LED_STRIP_COUNT 144
#define LED_STRIP_TYPE TWR_LED_STRIP_TYPE_RGBW
#define APPLICATION_TASK_ID 0

// LED instance
twr_led_t led;
bool led_state = false;

// Button instance
twr_button_t button;

// Thermometer instance
twr_tmp112_t tmp112;

// Led strip
static uint32_t led_strip_dma_buffer[LED_STRIP_COUNT * LED_STRIP_TYPE * 2];
const twr_led_strip_buffer_t led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = LED_STRIP_COUNT,
    .buffer = led_strip_dma_buffer
};
twr_led_strip_t led_strip;
uint32_t segment_color[SEGMENTS]; 

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    static uint16_t button_click_count = 0;

    // Log button event
    twr_log_info("Button event: %i", event);

    // Check event source
    if (event == TWR_BUTTON_EVENT_CLICK) {
        // Toggle LED pin state
        twr_led_pulse(&led, 2000);
        // Publish message on radio
        button_click_count++;
        twr_radio_pub_push_button(&button_click_count);

    } else if (event == TWR_BUTTON_EVENT_HOLD) {
        // set debug color
        static bool test_on = false;
        
        test_on = !test_on;
        for (uint8_t segment = 0; segment < SEGMENTS; segment++) {
            segment_color[segment] = test_on ? 0x00001000 : 0;
        }
        twr_scheduler_plan_now(APPLICATION_TASK_ID);
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    if (event == TWR_TMP112_EVENT_UPDATE) {
        float celsius;
        // Read temperature
        twr_tmp112_get_temperature_celsius(self, &celsius);
        twr_log_debug("Temperature: %.2f °C", celsius);
        // Publish message on radio
        twr_radio_pub_temperature(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE, &celsius);
    }
}

void twr_radio_node_on_state_get(uint64_t *id, uint8_t state_id)
{
    (void) id;

    twr_led_pulse(&led, 30);

    if (state_id == TWR_RADIO_NODE_STATE_POWER_MODULE_RELAY) {
        bool state = twr_module_power_relay_get_state();
        twr_radio_pub_state(TWR_RADIO_PUB_STATE_POWER_MODULE_RELAY, &state);
    }
    else if (state_id == TWR_RADIO_NODE_STATE_LED) {
        twr_radio_pub_state(TWR_RADIO_PUB_STATE_LED, &led_state);
    }
}

void twr_radio_node_on_state_set(uint64_t *id, uint8_t state_id, bool *state)
{
    (void) id;

    if (state_id == TWR_RADIO_NODE_STATE_POWER_MODULE_RELAY) {
        twr_led_pulse(&led, 30);
        twr_module_power_relay_set_state(*state);
        twr_radio_pub_state(TWR_RADIO_PUB_STATE_POWER_MODULE_RELAY, state);
    }
    else if (state_id == TWR_RADIO_NODE_STATE_LED) {
        led_state = *state;
        twr_led_set_mode(&led, led_state ? TWR_LED_MODE_ON : TWR_LED_MODE_OFF);
        twr_radio_pub_state(TWR_RADIO_PUB_STATE_LED, &led_state);
    }
}

uint8_t hex_to_u8(const char *hex)
{
    uint8_t high = (*hex <= '9') ? *hex - '0' : toupper(*hex) - 'A' + 10;
    uint8_t low = (*(hex+1) <= '9') ? *(hex+1) - '0' : toupper(*(hex+1)) - 'A' + 10;
    return (high << 4) | low;
}

void led_strip_color_set(uint64_t *id, const char *topic, void *value, void *param)
{
     twr_led_pulse(&led, 30);

    uint32_t color = 0;
    char *str = (char *) value;
    uint32_t segment = (uint32_t) param;
    int length = strlen(str);

    twr_log_debug("Receive %d \"%s\"", length, str);

    if (((length == 7) || (length == 11)) && (str[0] == '#')) {
        str++;
        length--;
    }

    uint8_t *pcolor = (uint8_t *) &color;

    if ((length == 6) || (length == 10 && str[6] == '(' && str[9] == ')')) {
        pcolor[3] = hex_to_u8(str + 0);
        pcolor[2] = hex_to_u8(str + 2);
        pcolor[1] = hex_to_u8(str + 4);
        pcolor[0] =  (length == 10) ? hex_to_u8(str + 7) : 0x00;
    }

    twr_log_debug("Segment: %ld color: %08lx", segment, color);

    segment_color[segment] = color;
    twr_scheduler_plan_now(APPLICATION_TASK_ID);
}

void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, 0);
    twr_led_pulse(&led, 2000);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, 0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize thermometer on core module
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, 30 * 1000);

    // Initialize power module
    twr_module_power_init();

    // Initialize led-strip on power module
    twr_led_strip_init(&led_strip, twr_module_power_get_led_strip_driver(), &led_strip_buffer);
    memset(segment_color, 0, sizeof(segment_color));

    static const twr_radio_sub_t subs[] = {
        {"led-strip/0/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 0},
        {"led-strip/1/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 1},
        {"led-strip/2/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 2},
        {"led-strip/3/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 3},
        {"led-strip/4/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 4},
        {"led-strip/5/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 5},
        {"led-strip/6/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 6},
        {"led-strip/7/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 7},
        {"led-strip/8/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 8},
        {"led-strip/9/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 9},
        {"led-strip/10/color/set", TWR_RADIO_SUB_PT_STRING, led_strip_color_set, (void *) 10},
    };

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));
    // Send radio pairing request
    twr_radio_pairing_request("stem-power-controller", VERSION);
}

void application_task(void)
{
    if (!twr_led_strip_is_ready(&led_strip))
    {
        twr_scheduler_plan_current_now();

        return;
    }

    int position = 0;

    for (uint8_t segment = 0; segment < SEGMENTS; segment++) {
        uint32_t color = segment_color[segment];
        int length = segment == 0 ? 4 : 12;

        // twr_log_debug("%d, %d, %08X", segment, length, color);
        for (int i = 0; i < length; i++){
            twr_led_strip_set_pixel(&led_strip, position++, color);
        }

        twr_led_strip_set_pixel(&led_strip, position++, 0);
        twr_led_strip_set_pixel(&led_strip, position++, 0);
    }

    twr_led_strip_write(&led_strip);

    twr_scheduler_plan_current_relative(500);
}
