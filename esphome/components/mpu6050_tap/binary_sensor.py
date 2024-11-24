from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN, DEVICE_CLASS_MOTION

DEPENDENCIES = ["i2c"]

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

mpu6050_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component, i2c.I2CDevice
)

# Direction action schema for both single and double taps
DIRECTION_ACTIONS = {
    cv.Optional(CONF_UP): automation.validate_automation(),
    cv.Optional(CONF_DOWN): automation.validate_automation(),
    cv.Optional(CONF_LEFT): automation.validate_automation(),
    cv.Optional(CONF_RIGHT): automation.validate_automation(),
}

# Main configuration schema
CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_MOTION)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(MPU6050TapSensor),
            cv.Required(CONF_INTERRUPT_PIN): cv.int_,
            cv.Optional(CONF_SENSITIVITY, default=0x40): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_DURATION, default=0x01): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(
                CONF_DOUBLE_TAP_WINDOW, default="200ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ON_SINGLE_TAP): cv.Schema(DIRECTION_ACTIONS),
            cv.Optional(CONF_ON_DOUBLE_TAP): cv.Schema(DIRECTION_ACTIONS),
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
    cg.add(var.set_duration(config[CONF_DURATION]))

    if CONF_DOUBLE_TAP_WINDOW in config:
        cg.add(
            var.set_double_tap_window(config[CONF_DOUBLE_TAP_WINDOW].total_milliseconds)
        )

    # Register single tap triggers
    if CONF_ON_SINGLE_TAP in config:
        conf = config[CONF_ON_SINGLE_TAP]
        if CONF_UP in conf:
            await automation.build_automation(
                var.get_single_tap_up_trigger(), [], conf[CONF_UP]
            )
        if CONF_DOWN in conf:
            await automation.build_automation(
                var.get_single_tap_down_trigger(), [], conf[CONF_DOWN]
            )
        if CONF_LEFT in conf:
            await automation.build_automation(
                var.get_single_tap_left_trigger(), [], conf[CONF_LEFT]
            )
        if CONF_RIGHT in conf:
            await automation.build_automation(
                var.get_single_tap_right_trigger(), [], conf[CONF_RIGHT]
            )

    # Register double tap triggers
    if CONF_ON_DOUBLE_TAP in config:
        conf = config[CONF_ON_DOUBLE_TAP]
        if CONF_UP in conf:
            await automation.build_automation(
                var.get_double_tap_up_trigger(), [], conf[CONF_UP]
            )
        if CONF_DOWN in conf:
            await automation.build_automation(
                var.get_double_tap_down_trigger(), [], conf[CONF_DOWN]
            )
        if CONF_LEFT in conf:
            await automation.build_automation(
                var.get_double_tap_left_trigger(), [], conf[CONF_LEFT]
            )
        if CONF_RIGHT in conf:
            await automation.build_automation(
                var.get_double_tap_right_trigger(), [], conf[CONF_RIGHT]
            )
