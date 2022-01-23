import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_CHANNEL
from . import otio_ns, OtioComponent, CONF_OTIO_ID

DEPENDENCIES = ['otio']

OtioLowBatteryBinarySensor = otio_ns.class_('OtioLowBatteryBinarySensor', binary_sensor.BinarySensor )


CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(OtioLowBatteryBinarySensor),
    cv.GenerateID(CONF_OTIO_ID): cv.use_id(OtioComponent),
    cv.Optional(CONF_CHANNEL, default=1): cv.int_range(min=1,max=3),
    cv.Optional(binary_sensor.CONF_DEVICE_CLASS, default='battery'): cv.one_of(*binary_sensor.DEVICE_CLASSES, lower=True, space='_'), # Set a default class
})



def to_code(config):
    hub = yield cg.get_variable(config[CONF_OTIO_ID])
    rhs = hub.Pget_binary_sensor_by_channel(config[CONF_CHANNEL])
    var = cg.Pvariable(config[CONF_ID], rhs)
    yield binary_sensor.register_binary_sensor(var, config)