import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, remote_receiver, remote_base
from esphome.components.remote_base import CONF_RECEIVER_ID
from esphome.const import CONF_SENSOR, CONF_ID, ICON_THERMOMETER, UNIT_CELSIUS

AUTO_LOAD = ['sensor', 'remote_base']


otio_ns = cg.esphome_ns.namespace('otio')
OTIOSensor = otio_ns.class_('OTIO', cg.Component, sensor.Sensor, remote_base.RemoteReceiverListener)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_CELSIUS, ICON_THERMOMETER, 1).extend(
    {cv.GenerateID(): cv.declare_id(OTIOSensor),}).extend({
    cv.Required(CONF_RECEIVER_ID): cv.use_id(remote_receiver.RemoteReceiverComponent),
})



def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    receiver = yield cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))

