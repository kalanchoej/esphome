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

class MPU6050TapSensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_interrupt_pin(int pin) { this->interrupt_pin_ = pin; }
  void set_sensitivity(uint8_t sensitivity) { this->tap_threshold_ = sensitivity; }
  void set_duration(uint8_t duration) { this->tap_duration_ = duration; }
  void set_double_tap_window(uint32_t window_ms) { this->double_tap_window_ms_ = window_ms; }

  void add_on_single_tap_up_callback(std::function<void()> callback) {
    this->single_tap_up_callback_.add(std::move(callback));
  }
  void add_on_single_tap_down_callback(std::function<void()> callback) {
    this->single_tap_down_callback_.add(std::move(callback));
  }
  void add_on_single_tap_left_callback(std::function<void()> callback) {
    this->single_tap_left_callback_.add(std::move(callback));
  }
  void add_on_single_tap_right_callback(std::function<void()> callback) {
    this->single_tap_right_callback_.add(std::move(callback));
  }

  void add_on_double_tap_up_callback(std::function<void()> callback) {
    this->double_tap_up_callback_.add(std::move(callback));
  }
  void add_on_double_tap_down_callback(std::function<void()> callback) {
    this->double_tap_down_callback_.add(std::move(callback));
  }
  void add_on_double_tap_left_callback(std::function<void()> callback) {
    this->double_tap_left_callback_.add(std::move(callback));
  }
  void add_on_double_tap_right_callback(std::function<void()> callback) {
    this->double_tap_right_callback_.add(std::move(callback));
  }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_tap_detected_();  // Called by ISR

 protected:
  static const size_t TAP_QUEUE_SIZE = 4;  // Size of circular buffer for tap events

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

  // ISR-safe circular buffer for tap events
  volatile uint8_t tap_queue_head_{0};
  volatile uint8_t tap_queue_tail_{0};
  TapEvent tap_queue_[TAP_QUEUE_SIZE];

  TapEvent last_tap_;
  bool waiting_for_double_tap_{false};

  CallbackManager<void()> single_tap_up_callback_{};
  CallbackManager<void()> single_tap_down_callback_{};
  CallbackManager<void()> single_tap_left_callback_{};
  CallbackManager<void()> single_tap_right_callback_{};

  CallbackManager<void()> double_tap_up_callback_{};
  CallbackManager<void()> double_tap_down_callback_{};
  CallbackManager<void()> double_tap_left_callback_{};
  CallbackManager<void()> double_tap_right_callback_{};
};

}  // namespace mpu6050_tap
}  // namespace esphome