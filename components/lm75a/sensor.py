import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, UNIT_CELSIUS, ICON_THERMOMETER

DEPENDENCIES = ['i2c']

lm75a_ns = cg.esphome_ns.namespace('lm75a')

LM75AComponent = lm75a_ns.class_('LM75AComponent', sensor.Sensor, cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_CELSIUS, ICON_THERMOMETER, 1).extend({
    cv.GenerateID(): cv.declare_id(LM75AComponent),}).extend(cv.polling_component_schema('60s')).extend(i2c.i2c_device_schema(0x48))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    yield i2c.register_i2c_device(var, config)
