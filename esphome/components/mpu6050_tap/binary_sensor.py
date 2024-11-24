from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_INTERRUPT_PIN
from esphome.coroutine import coroutine_with_priority

DEPENDENCIES = ["i2c"]

# Namespace and classes
mpu6050_tap_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_tap_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)
DirectionTrigger = mpu6050_tap_ns.class_("DirectionTrigger", automation.Trigger)

# Configuration keys
CONF_SENSITIVITY = "sensitivity"
CONF_DURATION = "duration"
CONF_ON_SINGLE_TAP = "on_single_tap"
CONF_ON_DOUBLE_TAP = "on_double_tap"
TAP_DIRECTIONS = ["up", "down", "left", "right"]

# Schema for tap directions
DIRECTIONAL_SCHEMA = cv.Schema(
    {
        cv.Optional("up"): automation.validate_automation(
            {cv.GenerateID(): cv.declare_id(DirectionTrigger)}
        ),
        cv.Optional("down"): automation.validate_automation(
            {cv.GenerateID(): cv.declare_id(DirectionTrigger)}
        ),
        cv.Optional("left"): automation.validate_automation(
            {cv.GenerateID(): cv.declare_id(DirectionTrigger)}
        ),
        cv.Optional("right"): automation.validate_automation(
            {cv.GenerateID(): cv.declare_id(DirectionTrigger)}
        ),
    }
)

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
            cv.Optional(CONF_ON_SINGLE_TAP): DIRECTIONAL_SCHEMA,
            cv.Optional(CONF_ON_DOUBLE_TAP): DIRECTIONAL_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x68))
)


@coroutine_with_priority(100.0)
async def to_code(config):
    cg.add_define("USE_BINARY_SENSOR")
    cg.add_global(mpu6050_tap_ns.using)
