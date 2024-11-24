from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN, DEVICE_CLASS_MOTION

DEPENDENCIES = ["i2c"]

mpu6050_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component, i2c.I2CDevice
)

# Configuration constants
CONF_SENSITIVITY = "sensitivity"
CONF_TIMING = "timing"
CONF_DURATION = "duration"
CONF_DOUBLE_TAP_WINDOW = "double_tap_window"
CONF_ON_SINGLE_TAP = "on_single_tap"
CONF_ON_DOUBLE_TAP = "on_double_tap"
CONF_UP = "up"
CONF_DOWN = "down"
CONF_LEFT = "left"
CONF_RIGHT = "right"

# Direction action schema for both single and double taps
DIRECTION_ACTIONS = {
    cv.Optional(CONF_UP): automation.validate_automation(),
    cv.Optional(CONF_DOWN): automation.validate_automation(),
    cv.Optional(CONF_LEFT): automation.validate_automation(),
    cv.Optional(CONF_RIGHT): automation.validate_automation(),
}

# Main configuration schema
CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        MPU6050TapSensor,
        device_class=DEVICE_CLASS_MOTION,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(MPU6050TapSensor),
            cv.Required(CONF_INTERRUPT_PIN): cv.int_,
            cv.Optional(CONF_SENSITIVITY, default=0x40): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_TIMING): cv.Schema(
                {
                    cv.Optional(CONF_DURATION, default=0x01): cv.hex_int_range(
                        min=0x00, max=0xFF
                    ),
                    cv.Optional(
                        CONF_DOUBLE_TAP_WINDOW, default="200ms"
                    ): cv.positive_time_period_milliseconds,
                }
            ),
            cv.Optional(CONF_ON_SINGLE_TAP): DIRECTION_ACTIONS,
            cv.Optional(CONF_ON_DOUBLE_TAP): DIRECTION_ACTIONS,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x68))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
    await i2c.register_i2c_device(var, config)

    # Configure basic settings
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))

    # Configure timing parameters if present
    if CONF_TIMING in config:
        timing = config[CONF_TIMING]
        if CONF_DURATION in timing:
            cg.add(var.set_duration(timing[CONF_DURATION]))
        if CONF_DOUBLE_TAP_WINDOW in timing:
            cg.add(
                var.set_double_tap_window(
                    timing[CONF_DOUBLE_TAP_WINDOW].total_milliseconds
                )
            )

    # Register single tap direction callbacks
    if CONF_ON_SINGLE_TAP in config:
        for direction in (CONF_UP, CONF_DOWN, CONF_LEFT, CONF_RIGHT):
            if direction in config[CONF_ON_SINGLE_TAP]:
                await automation.build_automation(
                    var.get_single_tap_trigger(direction),
                    [],
                    config[CONF_ON_SINGLE_TAP][direction],
                )

    # Register double tap direction callbacks
    if CONF_ON_DOUBLE_TAP in config:
        for direction in (CONF_UP, CONF_DOWN, CONF_LEFT, CONF_RIGHT):
            if direction in config[CONF_ON_DOUBLE_TAP]:
                await automation.build_automation(
                    var.get_double_tap_trigger(direction),
                    [],
                    config[CONF_ON_DOUBLE_TAP][direction],
                )
