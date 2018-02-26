#include "mgos.h"
#include "mgos_mqtt.h"

static int alarm = 0;
//static int maintenance = 0;

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

enum mgos_app_init_result mgos_app_init(void) {
  init_ports();
  return MGOS_APP_INIT_SUCCESS;
}
