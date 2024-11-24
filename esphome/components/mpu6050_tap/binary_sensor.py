from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN

DEPENDENCIES = ["i2c"]

mpu6050_tap_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_tap_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)

CONF_SENSITIVITY = "sensitivity"
CONF_DURATION = "duration"
CONF_ON_SINGLE_TAP = "on_single_tap"
CONF_ON_DOUBLE_TAP = "on_double_tap"

TAP_DIRECTIONS = ["up", "down", "left", "right"]

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(MPU6050TapSensor)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(MPU6050TapSensor),
            cv.Required(CONF_INTERRUPT_PIN): cv.int_,
            cv.Optional(CONF_SENSITIVITY, default=0x20): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_DURATION, default=0x01): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_ON_SINGLE_TAP): cv.Schema(
                {
                    direction: cv.ensure_list(automation.validate_automation())
                    for direction in TAP_DIRECTIONS
                }
            ),
            cv.Optional(CONF_ON_DOUBLE_TAP): cv.Schema(
                {
                    direction: cv.ensure_list(automation.validate_automation())
                    for direction in TAP_DIRECTIONS
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x68))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_duration(config[CONF_DURATION]))

    # Handle on_single_tap actions
    if CONF_ON_SINGLE_TAP in config:
        single_tap = config[CONF_ON_SINGLE_TAP]
        for direction in TAP_DIRECTIONS:
            if direction in single_tap:
                for action in single_tap[direction]:
                    register_trigger = getattr(
                        var, f"register_single_tap_{direction}_callback"
                    )
                    await automation.build_automation(
                        register_trigger, action, config=action
                    )

    # Handle on_double_tap actions
    if CONF_ON_DOUBLE_TAP in config:
        double_tap = config[CONF_ON_DOUBLE_TAP]
        for direction in TAP_DIRECTIONS:
            if direction in double_tap:
                for action in double_tap[direction]:
                    register_trigger = getattr(
                        var, f"register_double_tap_{direction}_callback"
                    )
                    await automation.build_automation(
                        register_trigger, action, config=action
                    )
