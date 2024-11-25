#include "mpu6050_tap.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mpu6050_tap {

static const char *TAG = "mpu6050_tap";

// Static pointer to the instance for interrupt handling
static MPU6050TapSensor *instance_ = nullptr;

void IRAM_ATTR static_isr_handler() {
  if (instance_ != nullptr) {
    instance_->on_tap_detected_();
  }
}

void MPU6050TapSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MPU6050 Tap Sensor...");

  instance_ = this;

  this->write_register(0x6B, 0x00);                // Power Management 1: clear sleep mode
  this->write_register(0x1C, 0x00);                // Set accelerometer sensitivity to ±2g
  this->write_register(0x1F, this->sensitivity_);  // Motion threshold
  this->write_register(0x20, this->duration_);     // Motion duration
  this->write_register(0x38, 0x40);                // Enable motion interrupt

  pinMode(this->interrupt_pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->interrupt_pin_), static_isr_handler, FALLING);

  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO %d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->sensitivity_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->duration_);
}

void MPU6050TapSensor::on_tap_detected_() { this->tap_detected_ = true; }

void MPU6050TapSensor::loop() {
  if (this->tap_detected_) {
    this->tap_detected_ = false;
    uint32_t current_time = millis();

    if (current_time - this->last_tap_time_ <= this->double_tap_delay_) {
      ESP_LOGD(TAG, "Double Tap detected!");
      this->publish_state(true);
      this->set_timeout(10, [this]() { this->publish_state(false); });
    } else {
      ESP_LOGD(TAG, "Single Tap detected!");
    }

    this->last_tap_time_ = current_time;
  }
}

void MPU6050TapSensor::write_register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t MPU6050TapSensor::read_register(uint8_t reg) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 1);
  return Wire.available() ? Wire.read() : 0;
}

void MPU6050TapSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "MPU6050 Tap Sensor:");
  LOG_BINARY_SENSOR(TAG, "Tap Detected", this);
}

}  // namespace mpu6050_tap
}  // namespace esphome
