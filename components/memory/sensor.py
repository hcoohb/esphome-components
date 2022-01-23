import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

UNIT_BYTE = 'B'
ICON_MEMORY = 'mdi:memory'

memory_ns = cg.esphome_ns.namespace('memory')
MemorySensor = memory_ns.class_('MemorySensor', sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_BYTE, ICON_MEMORY, 0).extend({
    cv.GenerateID(): cv.declare_id(MemorySensor),
}).extend(cv.polling_component_schema('60s'))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)