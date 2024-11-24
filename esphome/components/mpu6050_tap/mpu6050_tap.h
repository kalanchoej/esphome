#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/automation.h"
#include <Wire.h>

namespace esphome {
namespace mpu6050_tap {

// Enumeration for tap directions
enum TapDirection { TAP_UP, TAP_DOWN, TAP_LEFT, TAP_RIGHT, TAP_UNKNOWN };

// Trigger class for different tap events
class DirectionTrigger : public Trigger<> {};

class MPU6050TapSensor : public binary_sensor::BinarySensor, public Component {
 public:
  // Setters for configuration parameters
  void set_interrupt_pin(int pin) { this->interrupt_pin_ = pin; }
  void set_sensitivity(uint8_t sensitivity) { this->sensitivity_ = sensitivity; }
  void set_duration(uint8_t duration) { this->duration_ = duration; }

  // Getter methods for triggers
  DirectionTrigger *get_single_tap_trigger(const std::string &direction);
  DirectionTrigger *get_double_tap_trigger(const std::string &direction);

  // Callback registration methods for single taps
  void register_single_tap_up_callback(DirectionTrigger *callback) { this->single_tap_up_trigger_ = callback; }
  void register_single_tap_down_callback(DirectionTrigger *callback) { this->single_tap_down_trigger_ = callback; }
  void register_single_tap_left_callback(DirectionTrigger *callback) { this->single_tap_left_trigger_ = callback; }
  void register_single_tap_right_callback(DirectionTrigger *callback) { this->single_tap_right_trigger_ = callback; }

  // Callback registration methods for double taps
  void register_double_tap_up_callback(DirectionTrigger *callback) { this->double_tap_up_trigger_ = callback; }
  void register_double_tap_down_callback(DirectionTrigger *callback) { this->double_tap_down_trigger_ = callback; }
  void register_double_tap_left_callback(DirectionTrigger *callback) { this->double_tap_left_trigger_ = callback; }
  void register_double_tap_right_callback(DirectionTrigger *callback) { this->double_tap_right_trigger_ = callback; }

  // Overridden methods from Component
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_tap_detected_();  // Called by ISR

 protected:
  // Helper methods
  void write_register(uint8_t reg, uint8_t value);
  uint8_t read_register(uint8_t reg);
  void configure_mpu6050_();
  TapDirection detect_tap_direction_(const int16_t accel_x, const int16_t accel_y, const int16_t accel_z);
  void execute_callbacks_(bool is_double_tap, TapDirection dir);

  // Configuration parameters
  int interrupt_pin_;
  uint8_t sensitivity_;
  uint8_t duration_;

  // Trigger storage
  DirectionTrigger single_tap_triggers_[4];
  DirectionTrigger double_tap_triggers_[4];

  // Flag set by ISR
  volatile bool tap_detected_ = false;

  // Variables for double tap detection
  uint32_t last_tap_time_ = 0;
  bool awaiting_double_tap_ = false;
  TapDirection last_tap_direction_ = TAP_UNKNOWN;

  // Direction triggers for single taps
  DirectionTrigger *single_tap_up_trigger_ = nullptr;
  DirectionTrigger *single_tap_down_trigger_ = nullptr;
  DirectionTrigger *single_tap_left_trigger_ = nullptr;
  DirectionTrigger *single_tap_right_trigger_ = nullptr;

  // Direction triggers for double taps
  DirectionTrigger *double_tap_up_trigger_ = nullptr;
  DirectionTrigger *double_tap_down_trigger_ = nullptr;
  DirectionTrigger *double_tap_left_trigger_ = nullptr;
  DirectionTrigger *double_tap_right_trigger_ = nullptr;
};

}  // namespace mpu6050_tap
}  // namespace esphome
