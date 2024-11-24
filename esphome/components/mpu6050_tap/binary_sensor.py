from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_INTERRUPT_PIN,
    CONF_THEN,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_MOTION,
)

DEPENDENCIES = ["i2c"]

CONF_SENSITIVITY = "sensitivity"
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
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)

# Direction action schema for both single and double taps
DirectionTrigger = mpu6050_ns.class_("DirectionTrigger", automation.Trigger.template())

DIRECTION_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(DirectionTrigger),
        cv.Required(CONF_THEN): automation.validate_automation(),
    }
)

DIRECTION_ACTIONS = {
    cv.Optional(CONF_UP): DIRECTION_ACTION_SCHEMA,
    cv.Optional(CONF_DOWN): DIRECTION_ACTION_SCHEMA,
    cv.Optional(CONF_LEFT): DIRECTION_ACTION_SCHEMA,
    cv.Optional(CONF_RIGHT): DIRECTION_ACTION_SCHEMA,
}

# Main configuration schema
CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        MPU6050TapSensor, device_class=DEVICE_CLASS_MOTION
    )
    .extend(
        {
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
            cv.Optional(CONF_ON_SINGLE_TAP): DIRECTION_ACTIONS,
            cv.Optional(CONF_ON_DOUBLE_TAP): DIRECTION_ACTIONS,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def setup_direction_triggers(config, key, get_trigger):
    if key in config:
        conf = config[key]
        if CONF_UP in conf:
            await automation.build_automation(
                get_trigger("up"),
                [],
                conf[CONF_UP],
            )
        if CONF_DOWN in conf:
            await automation.build_automation(
                get_trigger("down"),
                [],
                conf[CONF_DOWN],
            )
        if CONF_LEFT in conf:
            await automation.build_automation(
                get_trigger("left"),
                [],
                conf[CONF_LEFT],
            )
        if CONF_RIGHT in conf:
            await automation.build_automation(
                get_trigger("right"),
                [],
                conf[CONF_RIGHT],
            )


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    # Configure basic settings
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    if CONF_SENSITIVITY in config:
        cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    if CONF_DURATION in config:
        cg.add(var.set_duration(config[CONF_DURATION]))
    if CONF_DOUBLE_TAP_WINDOW in config:
        cg.add(
            var.set_double_tap_window(config[CONF_DOUBLE_TAP_WINDOW].total_milliseconds)
        )

    # Setup triggers
    await setup_direction_triggers(
        config,
        CONF_ON_SINGLE_TAP,
        var.get_single_tap_trigger,
    )
    await setup_direction_triggers(
        config,
        CONF_ON_DOUBLE_TAP,
        var.get_double_tap_trigger,
    )
