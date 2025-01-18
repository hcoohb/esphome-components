from esphome.components import remote_base as rb
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_DATA
)



CONFIG_SCHEMA = cv.Schema({
})


# LucciAir
(
    LucciAirData,
    LucciAirBinarySensor,
    LucciAirTrigger,
    LucciAirAction,
    LucciAirDumper,
) = rb.declare_protocol("LucciAir")

LUCCIAIR_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ADDRESS): cv.uint16_t,
        cv.Required(CONF_DATA): cv.uint8_t,
    }
)


@rb.register_binary_sensor("lucci_air", LucciAirBinarySensor, LUCCIAIR_SCHEMA)
def lucci_air_binary_sensor(var, config):
    cg.add(
        var.set_data(
            cg.StructInitializer(
                LucciAirData,
                ("address", config[CONF_ADDRESS]),
                ("data", config[CONF_DATA]),
            )
        )
    )


@rb.register_trigger("lucci_air", LucciAirTrigger, LucciAirData)
def lucci_air_trigger(var, config):
    pass


@rb.register_dumper("lucci_air", LucciAirDumper)
def lucci_air_dumper(var, config):
    pass


@rb.register_action("lucci_air", LucciAirAction, LUCCIAIR_SCHEMA)
async def lucci_air_action(var, config, args):
    template_ = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    cg.add(var.set_address(template_))
    template_ = await cg.templatable(config[CONF_DATA], args, cg.uint8)
    cg.add(var.set_data(template_))
