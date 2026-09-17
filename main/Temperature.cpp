#include "Temperature.hpp"

#include "onewire_bus.h"
#include "ds18b20.h"

static onewire_bus_handle_t onewire_bus = NULL;
static ds18b20_device_handle_t ds18b20_dev = NULL;


bool ds18b20_init(int pin)
{
    onewire_bus_config_t bus_config {};
    bus_config.bus_gpio_num = pin;
    bus_config.flags.en_pull_up = 0;

    onewire_bus_rmt_config_t rmt_config {};
    rmt_config.max_rx_bytes = 10; // 1byte ROM command + 8byte ROM number + 1byte device command

    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &onewire_bus) != ESP_OK) {
        return false;
    }

    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t device;
    ds18b20_config_t ds_cfg = {};
    const bool success = (
        onewire_new_device_iter(onewire_bus, &iter) == ESP_OK and
        onewire_device_iter_get_next(iter, &device) == ESP_OK and
        ds18b20_new_device_from_enumeration(&device, &ds_cfg, &ds18b20_dev) == ESP_OK
    );

    onewire_del_device_iter(iter);
    return success;
}

bool ds18b20_read(float& value)
{
    return (
        ds18b20_trigger_temperature_conversion_for_all(onewire_bus) == ESP_OK and
        ds18b20_get_temperature(ds18b20_dev, &value) == ESP_OK
    );
}
