import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, light
from esphome.const import (
    CONF_ID,
    CONF_OUTPUT_ID,
)


CODEOWNERS = ["@hcoohb"]
DEPENDENCIES = ["light"]

yeelight_bt_ns = cg.esphome_ns.namespace("yeelight_bt")
Yeelight_bt = yeelight_bt_ns.class_("Yeelight_bt", ble_client.BLEClientNode, cg.Component, light.LightOutput)

CONFIG_SCHEMA = (
    # cv.Schema(
    #     {
    #         cv.GenerateID(): cv.declare_id(Yeelight_bt),
    #     }
    # )
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend({
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(Yeelight_bt),
    })
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
    # .extend(light.BRIGHTNESS_ONLY_LIGHT_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    yield cg.register_component(var, config)
    yield ble_client.register_ble_node(var, config)
    yield light.register_light(var, config)
