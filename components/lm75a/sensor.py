import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core
from esphome.components import i2c, sensor
from esphome.const import UNIT_CELSIUS, ICON_THERMOMETER, DEVICE_CLASS_TEMPERATURE, STATE_CLASS_MEASUREMENT

DEPENDENCIES = ['i2c']

lm75a_ns = cg.esphome_ns.namespace('lm75a')
LM75AComponent = lm75a_ns.class_('LM75AComponent', sensor.Sensor, cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        LM75AComponent,
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT
    )
    .extend(cv.polling_component_schema('60s'))
    .extend(i2c.i2c_device_schema(0x48))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
