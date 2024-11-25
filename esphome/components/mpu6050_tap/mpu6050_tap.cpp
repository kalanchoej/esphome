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

  // Ensure only one instance is set for the static interrupt handler
  instance_ = this;

  // Wake up the MPU6050
  this->write_register(0x6B, 0x00);  // Power Management 1: clear sleep mode

  // Set accelerometer sensitivity to ±2g
  this->write_register(0x1C, 0x00);

  // Configure motion threshold (sensitivity)
  this->write_register(0x1F, this->sensitivity_);

  // Configure motion duration
  this->write_register(0x20, this->duration_);

  // Enable motion interrupt
  this->write_register(0x38, 0x40);

  // Configure interrupt pin
  pinMode(this->interrupt_pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->interrupt_pin_), static_isr_handler, FALLING);

  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO %d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->sensitivity_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->duration_);
}

void MPU6050TapSensor::on_tap_detected_() {
  // ISR-safe operation: set flag
  this->tap_detected_ = true;
}

void MPU6050TapSensor::loop() {
  if (this->tap_detected_) {
    this->tap_detected_ = false;  // Clear the flag
    ESP_LOGD(TAG, "Tap detected!");
    this->publish_state(true);
    this->set_timeout(100, [this]() { this->publish_state(false); });  // Simple debounce
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
