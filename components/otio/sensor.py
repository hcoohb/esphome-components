import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, ICON_THERMOMETER, UNIT_CELSIUS, CONF_CHANNEL, UNIT_PERCENT, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_HUMIDITY, CONF_HUMIDITY, CONF_TEMPERATURE, STATE_CLASS_MEASUREMENT
from . import otio_ns, OtioComponent, CONF_OTIO_ID

DEPENDENCIES = ['otio']

OtioSensor = otio_ns.class_('OtioSensor', sensor.Sensor)
# OtioHumiditySensor = otio_ns.class_('OtioHumiditySensor', sensor.Sensor)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OtioSensor),
        cv.GenerateID(CONF_OTIO_ID): cv.use_id(OtioComponent),
        cv.Optional(CONF_CHANNEL, default=1): cv.int_range(min=1,max=3),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
        )
    }
)

# CONFIG_SCHEMA = cv.Schema(
#     {
#         cv.GenerateID(): cv.declare_id(DHT),
#         cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
#         cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
#             unit_of_measurement=UNIT_CELSIUS,
#             accuracy_decimals=1,
#             device_class=DEVICE_CLASS_TEMPERATURE,
#             state_class=STATE_CLASS_MEASUREMENT,
#         ),
#         cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
#             unit_of_measurement=UNIT_PERCENT,
#             accuracy_decimals=0,
#             device_class=DEVICE_CLASS_HUMIDITY,
#             state_class=STATE_CLASS_MEASUREMENT,
#         ),
#         cv.Optional(CONF_MODEL, default="auto detect"): cv.enum(
#             DHT_MODELS, upper=True, space="_"
#         ),
#     }
# ).extend(cv.polling_component_schema("60s"))


def to_code(config):
    # hub = yield cg.get_variable(config[CONF_OTIO_ID])
    # rhs = hub.Pget_temp_sensor_by_channel(config[CONF_CHANNEL])
    # var = cg.Pvariable(config[CONF_ID], rhs)
    # yield sensor.register_sensor(var, config)

    hub = yield cg.get_variable(config[CONF_OTIO_ID])
    # rhs = hub.Pget_temp_sensor_by_channel(config[CONF_CHANNEL])
    # var = cg.Pvariable(config[CONF_ID], rhs)
    # yield sensor.register_sensor(var, config)

    if CONF_TEMPERATURE in config:
        rhs = hub.Pget_temp_sensor_by_channel(config[CONF_CHANNEL])
        var = cg.Pvariable(config[CONF_TEMPERATURE][CONF_ID], rhs)
        yield sensor.register_sensor(var, config[CONF_TEMPERATURE])

    if CONF_HUMIDITY in config:
        rhs = hub.Pget_hum_sensor_by_channel(config[CONF_CHANNEL])
        var = cg.Pvariable(config[CONF_HUMIDITY][CONF_ID], rhs)
        yield sensor.register_sensor(var, config[CONF_HUMIDITY])





    # var = cg.new_Pvariable(config[CONF_ID])
    # await cg.register_component(var, config)
    # if CONF_TEMPERATURE in config:
    #     # sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
    #     sens = cg.new_Pvariable(config[CONF_TEMPERATURE][CONF_ID])
    # await register_sensor(sens, config[CONF_TEMPERATURE])
    #     cg.add(var.set_temperature_sensor(sens))