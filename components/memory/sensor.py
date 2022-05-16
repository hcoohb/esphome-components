import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import DEVICE_CLASS_EMPTY, STATE_CLASS_MEASUREMENT

UNIT_BYTE = 'B'
ICON_MEMORY = 'mdi:memory'

memory_ns = cg.esphome_ns.namespace('memory')
MemorySensor = memory_ns.class_('MemorySensor', sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        MemorySensor,
        unit_of_measurement=UNIT_BYTE,
        device_class=DEVICE_CLASS_EMPTY,
        icon=ICON_MEMORY,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
    )
    .extend(cv.polling_component_schema('60s'))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)