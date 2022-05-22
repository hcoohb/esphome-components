import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, light
from esphome.const import (
    CONF_ID,
    CONF_BATTERY_LEVEL,
    DEVICE_CLASS_BATTERY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    CONF_ILLUMINANCE,
    ICON_BRIGHTNESS_5,
    UNIT_PERCENT,
)


CODEOWNERS = ["@hcoohb"]

yeelight_bt_ns = cg.esphome_ns.namespace("yeelight_bt")
Yeelight_bt = yeelight_bt_ns.class_("Yeelight_bt", ble_client.BLEClientNode, cg.Component, light.LightOutput)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Yeelight_bt),
            # cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            #     unit_of_measurement=UNIT_PERCENT,
            #     device_class=DEVICE_CLASS_BATTERY,
            #     accuracy_decimals=0,
            #     entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            # ),
            # cv.Optional(CONF_ILLUMINANCE): sensor.sensor_schema(
            #     unit_of_measurement=UNIT_PERCENT,
            #     icon=ICON_BRIGHTNESS_5,
            #     accuracy_decimals=0,
            # ),
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(light.BRIGHTNESS_ONLY_LIGHT_SCHEMA)
    # .extend(cv.polling_component_schema("120s"))
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield ble_client.register_ble_node(var, config)
    yield light.register_light(var, config)

    # if CONF_BATTERY_LEVEL in config:
    #     sens = yield sensor.new_sensor(config[CONF_BATTERY_LEVEL])
    #     cg.add(var.set_battery(sens))

    # if CONF_ILLUMINANCE in config:
    #     sens = yield sensor.new_sensor(config[CONF_ILLUMINANCE])
    #     cg.add(var.set_illuminance(sens))