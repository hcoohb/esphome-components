from esphome.components import remote_base as rb
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_CHANNEL,
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    CONF_BATTERY_LEVEL,
    CONF_FORCE_UPDATE,
)



CONFIG_SCHEMA = cv.Schema({
})



(
    NexusData,
    NexusBinarySensor,
    NexusTrigger,
    NexusAction,
    NexusDumper,
) = rb.declare_protocol("Nexus")

NEXUS_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CHANNEL): cv.All(cv.uint8_t, cv.Range(min=1, max=4)),
        cv.Required(CONF_ADDRESS): cv.All(cv.uint8_t, cv.Range(min=0, max=255)),
        cv.Optional(CONF_TEMPERATURE, default="25.5"): cv.All(
            cv.float_, cv.Range(min=-204.8, max=204.7)
        ),
        cv.Optional(CONF_HUMIDITY, default="42"): cv.All(
            cv.uint8_t, cv.Range(min=0, max=255)
        ),
        cv.Optional(CONF_BATTERY_LEVEL, default="true"): cv.boolean,
        cv.Optional(CONF_FORCE_UPDATE, default="false"): cv.boolean,
    }
)


@rb.register_binary_sensor("nexus", NexusBinarySensor, NEXUS_SCHEMA)
def nexus_binary_sensor(var, config):
    cg.add(
        var.set_data(
            cg.StructInitializer(
                NexusData,
                ("channel", config[CONF_CHANNEL]),
                ("address", config[CONF_ADDRESS]),
            )
        )
    )


@rb.register_trigger("nexus", NexusTrigger, NexusData)
def nexus_trigger(var, config):
    pass


@rb.register_dumper("nexus", NexusDumper)
def nexus_dumper(var, config):
    pass


@rb.register_action("nexus", NexusAction, NEXUS_SCHEMA)
async def nexus_action(var, config, args):
    template_ = await cg.templatable(config[CONF_CHANNEL], args, cg.uint8)
    cg.add(var.set_channel(template_))
    template_ = await cg.templatable(config[CONF_ADDRESS], args, cg.uint8)
    cg.add(var.set_address(template_))
    template_ = await cg.templatable(config[CONF_TEMPERATURE], args, cg.float_)
    cg.add(var.set_temperature(template_))
    template_ = await cg.templatable(config[CONF_HUMIDITY], args, cg.uint8)
    cg.add(var.set_humidity(template_))
    template_ = await cg.templatable(config[CONF_BATTERY_LEVEL], args, cg.bool_)
    cg.add(var.set_battery_level(template_))
    template_ = await cg.templatable(config[CONF_FORCE_UPDATE], args, cg.bool_)
    cg.add(var.set_force_update(template_))


