#include "mgos.h"
#include "mgos_mqtt.h"
#include "mgos_rpc.h"

#define BEEP_PIN 14
#define BEEP_OFF_LEVEL true
#define MAINTENANCE_PIN 0

static int alarm = 0;
static int mqtt_enabled = 0;
static int maintenance = 0;

void alarm_beep() {
    if (alarm){
        mgos_gpio_toggle(BEEP_PIN);
    }
    else
    {
        mgos_gpio_write(BEEP_PIN, BEEP_OFF_LEVEL);
    }
}

int ports_state(){
    int listen_ports = mgos_sys_config_get_device_listen_ports();
    int result = 0;
    for (int pin = 0; pin < 16; pin++){
        if((listen_ports >> pin) && 1) {
            int pin_state = mgos_gpio_read(pin);
            result = result | ((pin_state & 1) << pin);
        }
    }

    return result;
}

void send_heartbeat(){
    if(!mqtt_enabled) {
        return;
    }
    char topic[100], message[100];
    struct json_out out = JSON_OUT_BUF(message, sizeof(message));
    snprintf(topic, sizeof(topic), "/%s/m", mgos_sys_config_get_device_id());
    json_printf(&out, "{alarm: %d, gpio: %d}", alarm, ports_state());
    mgos_mqtt_pub(topic, message, strlen(message), 1, false);
}

static void on_port_changed(int pin, void *arg) {
    send_heartbeat();
    (void) pin;
    (void) arg;
}

void init_ports(){
    int listen_ports = mgos_sys_config_get_device_listen_ports();
    for(int pin = 0; pin < 16; pin ++){
        if((listen_ports >> pin) && 1) {
            mgos_gpio_set_button_handler(pin, MGOS_GPIO_PULL_NONE, MGOS_GPIO_INT_EDGE_ANY, 50, on_port_changed, NULL);
        }
    }
}

void alarm_rpc(struct mg_rpc_request_info *ri, void *cb_arg,
               struct mg_rpc_frame_info *fi,
               struct mg_str args) {
    int enabled = 0;
    json_scanf(args.p, args.len, ri->args_fmt, &enabled);
    alarm = enabled;
    mg_rpc_send_responsef(ri, NULL);
    ri = NULL;

    (void) cb_arg;
    (void) fi;
}

static void mqtt_status(struct mg_connection *con, int ev, void *data, void *user_data) {
    switch(ev){
        case MG_EV_MQTT_CONNACK:
            mqtt_enabled = 1;
            break;
        case MG_EV_CLOSE: // we received a message
            mqtt_enabled = 0;
            break;
    }
    (void) con;
    (void) data;
    (void) user_data;
}

static void maintenance_handler(int pin, void *arg) {
    if(!maintenance){
        maintenance = 1;
        mgos_sys_config_set_http_enable(true);
        mgos_sys_config_set_wifi_ap_enable(true);
        char *err = NULL;
        save_cfg(&mgos_sys_config, &err); /* Writes conf9.json */
        printf("Saving configuration: %s\n", err ? err : "no error");
        free(err);
        mgos_system_restart();
    }
    (void) pin;
    (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
    mgos_mqtt_add_global_handler(mqtt_status, NULL);

    init_ports();
    mgos_set_timer(500 /* ms */, true /* repeat */, alarm_beep, NULL);

    // alarm rpc handler
    struct mg_rpc *rpc = mgos_rpc_get_global();
    mg_rpc_add_handler(rpc, "alarm", "{enabled: %d}", alarm_rpc, NULL);

    maintenance = mgos_sys_config_get_http_enable();
    if(!maintenance){
        mgos_gpio_set_button_handler(MAINTENANCE_PIN, MGOS_GPIO_PULL_NONE, MGOS_GPIO_INT_EDGE_ANY, 50 /* debounce_ms */,
                                     maintenance_handler, NULL);
    }

    return MGOS_APP_INIT_SUCCESS;
}
