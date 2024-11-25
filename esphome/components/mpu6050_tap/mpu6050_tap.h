#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <Wire.h>

namespace esphome {
namespace mpu6050_tap {

class MPU6050TapSensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_interrupt_pin(int pin) { this->interrupt_pin_ = pin; }
  void set_sensitivity(uint8_t sensitivity) { this->sensitivity_ = sensitivity; }
  void set_duration(uint8_t duration) { this->duration_ = duration; }
  void set_double_tap_delay(uint16_t delay) { this->double_tap_delay_ = delay; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_tap_detected_();  // Called by ISR

 protected:
  void write_register(uint8_t reg, uint8_t value);
  uint8_t read_register(uint8_t reg);

  int interrupt_pin_;
  uint8_t sensitivity_;
  uint8_t duration_;
  uint16_t double_tap_delay_;
  volatile bool tap_detected_ = false;  // Flag for ISR to main loop communication
  uint32_t last_tap_time_ = 0;          // Time of the last detected tap
};

}  // namespace mpu6050_tap
}  // namespace esphome
