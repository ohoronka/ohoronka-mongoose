#include "mgos.h"
#include "mgos_mqtt.h"
#include "mgos_rpc.h"

#define BEEP_PIN 14
#define BEEP_OFF_LEVEL true

static int alarm = 0;
//static int maintenance = 0;

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

enum mgos_app_init_result mgos_app_init(void) {
    init_ports();
    mgos_set_timer(500 /* ms */, true /* repeat */, alarm_beep, NULL);

    // alarm rpc handler
    struct mg_rpc *rpc = mgos_rpc_get_global();
    mg_rpc_add_handler(rpc, "alarm", "{enabled: %d}", alarm_rpc, NULL);

    return MGOS_APP_INIT_SUCCESS;
}
