import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, ICON_THERMOMETER, UNIT_CELSIUS, CONF_CHANNEL
from . import otio_ns, OtioComponent, CONF_OTIO_ID

DEPENDENCIES = ['otio']

OtioTemperatureSensor = otio_ns.class_('OtioTemperatureSensor', sensor.Sensor)


CONFIG_SCHEMA = sensor.sensor_schema(UNIT_CELSIUS, ICON_THERMOMETER, 1).extend({
    cv.GenerateID(): cv.declare_id(OtioTemperatureSensor),
    cv.GenerateID(CONF_OTIO_ID): cv.use_id(OtioComponent),
    cv.Optional(CONF_CHANNEL, default=1): cv.int_range(min=1,max=3),
})



def to_code(config):
    hub = yield cg.get_variable(config[CONF_OTIO_ID])
    rhs = hub.Pget_sensor_by_channel(config[CONF_CHANNEL])
    var = cg.Pvariable(config[CONF_ID], rhs)
    yield sensor.register_sensor(var, config)