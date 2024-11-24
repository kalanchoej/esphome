import voluptuous as vol

from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_THEN, DEVICE_CLASS_MOTION

# Define dependencies
DEPENDENCIES = ["i2c"]

# Define custom configuration keys
CONF_INTERRUPT_PIN = "interrupt_pin"
CONF_SENSITIVITY = "sensitivity"
CONF_DURATION = "duration"
CONF_DOUBLE_TAP_WINDOW = "double_tap_window"
CONF_ON_SINGLE_TAP = "on_single_tap"
CONF_ON_DOUBLE_TAP = "on_double_tap"
CONF_UP = "up"
CONF_DOWN = "down"
CONF_LEFT = "left"
CONF_RIGHT = "right"

# Namespace
mpu6050_tap_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_tap_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)

# Direction triggers
DirectionTrigger = mpu6050_tap_ns.class_(
    "DirectionTrigger", automation.Trigger.template()
)

# Validation schema for direction actions
DIRECTION_ACTIONS = {
    cv.Optional(CONF_UP): automation.validate_automation(),
    cv.Optional(CONF_DOWN): automation.validate_automation(),
    cv.Optional(CONF_LEFT): automation.validate_automation(),
    cv.Optional(CONF_RIGHT): automation.validate_automation(),
}

# Main configuration schema
CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        MPU6050TapSensor, device_class=DEVICE_CLASS_MOTION
    )
    .extend(
        {
            vol.Required(CONF_INTERRUPT_PIN): cv.int_,
            vol.Optional(CONF_SENSITIVITY, default=0x40): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            vol.Optional(CONF_DURATION, default=0x01): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            vol.Optional(
                CONF_DOUBLE_TAP_WINDOW, default="200ms"
            ): cv.positive_time_period_milliseconds,
            vol.Optional(CONF_ON_SINGLE_TAP): cv.Schema(DIRECTION_ACTIONS),
            vol.Optional(CONF_ON_DOUBLE_TAP): cv.Schema(DIRECTION_ACTIONS),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def setup_direction_triggers(config, key, get_trigger, var):
    if key not in config:
        return

    conf = config[key]
    for direction, actions in conf.items():
        trigger = get_trigger(direction)
        if trigger is not None and CONF_THEN in actions:
            # Pass trigger, actions, and config to build_automation
            await automation.build_automation(trigger, actions[CONF_THEN], config)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    # Configure basic settings
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_duration(config[CONF_DURATION]))
    cg.add(var.set_double_tap_window(config[CONF_DOUBLE_TAP_WINDOW].total_milliseconds))

    # Setup triggers for single and double taps
    await setup_direction_triggers(
        config,
        CONF_ON_SINGLE_TAP,
        var.get_single_tap_trigger,
        var,
    )
    await setup_direction_triggers(
        config,
        CONF_ON_DOUBLE_TAP,
        var.get_double_tap_trigger,
        var,
    )
