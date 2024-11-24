import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN, DEVICE_CLASS_MOTION

DEPENDENCIES = ["i2c"]

mpu6050_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)

CONF_SENSITIVITY = "sensitivity"
CONF_DURATION = "duration"
CONF_LATENCY = "latency"  # Time between taps for double tap
CONF_WINDOW = "window"  # Window for second tap
CONF_DOUBLE_TAP = "double_tap"  # Double tap binary sensor

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_MOTION)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(MPU6050TapSensor),
            cv.Required(CONF_INTERRUPT_PIN): cv.int_,
            cv.Optional(CONF_SENSITIVITY, default=0x20): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_DURATION, default=0x10): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_LATENCY, default=0x20): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_WINDOW, default=0x50): cv.hex_int_range(
                min=0x00, max=0xFF
            ),
            cv.Optional(CONF_DOUBLE_TAP): binary_sensor.BINARY_SENSOR_SCHEMA.extend(),
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

    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_duration(config[CONF_DURATION]))
    cg.add(var.set_latency(config[CONF_LATENCY]))
    cg.add(var.set_window(config[CONF_WINDOW]))

    if CONF_DOUBLE_TAP in config:
        double_tap = await binary_sensor.new_binary_sensor(config[CONF_DOUBLE_TAP])
        cg.add(var.set_double_tap_binary_sensor(double_tap))
