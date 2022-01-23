import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, remote_receiver, remote_base
from esphome.components.remote_base import CONF_RECEIVER_ID
from esphome.const import CONF_ID

DEPENDENCIES = ['remote_receiver']
AUTO_LOAD = ['sensor', 'binary_sensor']

CONF_OTIO_ID = 'otio_id'

otio_ns = cg.esphome_ns.namespace('otio')
OtioComponent = otio_ns.class_('OtioComponent', cg.Component, remote_base.RemoteReceiverListener)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OtioComponent),
    cv.Required(CONF_RECEIVER_ID): cv.use_id(remote_receiver.RemoteReceiverComponent),
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    receiver = yield cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))
