from esphome import automation  # Import the automation system to get the Trigger class
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN

DEPENDENCIES = ["i2c"]

# Create a namespace for the mpu6050_tap component
mpu6050_tap_ns = cg.esphome_ns.namespace("mpu6050_tap")
MPU6050TapSensor = mpu6050_tap_ns.class_(
    "MPU6050TapSensor", binary_sensor.BinarySensor, cg.Component
)
DirectionTrigger = mpu6050_tap_ns.class_(
    "DirectionTrigger", automation.Trigger
)  # Import the Trigger class from automation

CONF_SENSITIVITY = "sensitivity"
CONF_DURATION = "duration"
CONF_ON_SINGLE_TAP = "on_single_tap"
CONF_ON_DOUBLE_TAP = "on_double_tap"

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
                    cv.Optional("up"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("down"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("left"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("right"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                }
            ),
            cv.Optional(CONF_ON_DOUBLE_TAP): cv.Schema(
                {
                    cv.Optional("up"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("down"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("left"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
                    cv.Optional("right"): automation.validate_automation(
                        cv.declare_id(DirectionTrigger)
                    ),
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

    # Set up basic configuration
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_duration(config[CONF_DURATION]))

    # Handle on_single_tap actions explicitly
    if CONF_ON_SINGLE_TAP in config:
        single_tap = config[CONF_ON_SINGLE_TAP]

        if "up" in single_tap:
            trigger = cg.new_Pvariable(single_tap["up"][CONF_ID])
            await automation.build_automation(trigger, [], single_tap["up"])
            cg.add(var.register_single_tap_up_callback(trigger))

        if "down" in single_tap:
            trigger = cg.new_Pvariable(single_tap["down"][CONF_ID])
            await automation.build_automation(trigger, [], single_tap["down"])
            cg.add(var.register_single_tap_down_callback(trigger))

        if "left" in single_tap:
            trigger = cg.new_Pvariable(single_tap["left"][CONF_ID])
            await automation.build_automation(trigger, [], single_tap["left"])
            cg.add(var.register_single_tap_left_callback(trigger))

        if "right" in single_tap:
            trigger = cg.new_Pvariable(single_tap["right"][CONF_ID])
            await automation.build_automation(trigger, [], single_tap["right"])
            cg.add(var.register_single_tap_right_callback(trigger))

    # Handle on_double_tap actions explicitly
    if CONF_ON_DOUBLE_TAP in config:
        double_tap = config[CONF_ON_DOUBLE_TAP]

        if "up" in double_tap:
            trigger = cg.new_Pvariable(double_tap["up"][CONF_ID])
            await automation.build_automation(trigger, [], double_tap["up"])
            cg.add(var.register_double_tap_up_callback(trigger))

        if "down" in double_tap:
            trigger = cg.new_Pvariable(double_tap["down"][CONF_ID])
            await automation.build_automation(trigger, [], double_tap["down"])
            cg.add(var.register_double_tap_down_callback(trigger))

        if "left" in double_tap:
            trigger = cg.new_Pvariable(double_tap["left"][CONF_ID])
            await automation.build_automation(trigger, [], double_tap["left"])
            cg.add(var.register_double_tap_left_callback(trigger))

        if "right" in double_tap:
            trigger = cg.new_Pvariable(double_tap["right"][CONF_ID])
            await automation.build_automation(trigger, [], double_tap["right"])
            cg.add(var.register_double_tap_right_callback(trigger))
