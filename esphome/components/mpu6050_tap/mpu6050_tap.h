#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <Wire.h>
#include <queue>

namespace esphome {
namespace mpu6050_tap {

enum TapDirection { TAP_UP, TAP_DOWN, TAP_LEFT, TAP_RIGHT, TAP_UNKNOWN };

struct TapEvent {
  uint32_t timestamp;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
};

class DirectionTrigger : public Trigger<> {};

class MPU6050TapSensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_interrupt_pin(int pin) { this->interrupt_pin_ = pin; }
  void set_sensitivity(uint8_t sensitivity) { this->tap_threshold_ = sensitivity; }
  void set_duration(uint8_t duration) { this->tap_duration_ = duration; }
  void set_double_tap_window(uint32_t window_ms) { this->double_tap_window_ms_ = window_ms; }

  Trigger<> *get_single_tap_trigger(const std::string &direction) {
    if (direction == "up")
      return &this->single_tap_up_trigger_;
    if (direction == "down")
      return &this->single_tap_down_trigger_;
    if (direction == "left")
      return &this->single_tap_left_trigger_;
    if (direction == "right")
      return &this->single_tap_right_trigger_;
    return nullptr;
  }

  Trigger<> *get_double_tap_trigger(const std::string &direction) {
    if (direction == "up")
      return &this->double_tap_up_trigger_;
    if (direction == "down")
      return &this->double_tap_down_trigger_;
    if (direction == "left")
      return &this->double_tap_left_trigger_;
    if (direction == "right")
      return &this->double_tap_right_trigger_;
    return nullptr;
  }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_tap_detected_();  // Called by ISR

 protected:
  static const size_t TAP_QUEUE_SIZE = 4;

  void write_register(uint8_t reg, uint8_t value);
  uint8_t read_register(uint8_t reg);
  void read_accel_data_(int16_t *x, int16_t *y, int16_t *z);
  TapDirection detect_tap_direction_(const TapEvent &event);
  void process_tap_event_(const TapEvent &event);
  void execute_callbacks_(bool is_double_tap, TapDirection dir);

  int interrupt_pin_;
  uint8_t tap_threshold_;
  uint8_t tap_duration_;
  uint32_t double_tap_window_ms_;

  volatile uint8_t tap_queue_head_{0};
  volatile uint8_t tap_queue_tail_{0};
  TapEvent tap_queue_[TAP_QUEUE_SIZE];

  TapEvent last_tap_;
  bool waiting_for_double_tap_{false};

  // Direction triggers
  DirectionTrigger single_tap_up_trigger_{};
  DirectionTrigger single_tap_down_trigger_{};
  DirectionTrigger single_tap_left_trigger_{};
  DirectionTrigger single_tap_right_trigger_{};

  DirectionTrigger double_tap_up_trigger_{};
  DirectionTrigger double_tap_down_trigger_{};
  DirectionTrigger double_tap_left_trigger_{};
  DirectionTrigger double_tap_right_trigger_{};
};

}  // namespace mpu6050_tap
}  // namespace esphome