#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <Wire.h>

namespace esphome {
namespace mpu6050_tap {

class MPU6050TapSensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_interrupt_pin(int pin) { this->interrupt_pin_ = pin; }
  void set_sensitivity(uint8_t sensitivity) {
    this->x_threshold_ = sensitivity;
    this->y_threshold_ = sensitivity;
    this->z_threshold_ = sensitivity;
  }
  void set_duration(uint8_t duration) { this->duration_ = duration; }
  void set_latency(uint8_t latency) { this->latency_ = latency; }
  void set_window(uint8_t window) { this->window_ = window; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_tap_detected_();  // Called by ISR

  binary_sensor::BinarySensor *get_double_tap_binary_sensor() const { return this->double_tap_binary_sensor_; }
  void set_double_tap_binary_sensor(binary_sensor::BinarySensor *double_tap) {
    this->double_tap_binary_sensor_ = double_tap;
  }

 protected:
  void write_register(uint8_t reg, uint8_t value);
  uint8_t read_register(uint8_t reg);

  int interrupt_pin_;
  uint8_t x_threshold_;
  uint8_t y_threshold_;
  uint8_t z_threshold_;
  uint8_t duration_;  // Tap duration
  uint8_t latency_;   // Time between taps for double tap
  uint8_t window_;    // Window for second tap
  volatile bool tap_detected_ = false;
  binary_sensor::BinarySensor *double_tap_binary_sensor_{nullptr};
};

}  // namespace mpu6050_tap
}  // namespace esphome