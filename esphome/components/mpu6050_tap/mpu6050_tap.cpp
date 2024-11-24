#include "mpu6050_tap.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mpu6050_tap {

static const char *TAG = "mpu6050_tap";

static MPU6050TapSensor *instance_ = nullptr;

void IRAM_ATTR static_isr_handler() {
  if (instance_ != nullptr) {
    instance_->on_tap_detected_();
  }
}

void MPU6050TapSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MPU6050 Tap Sensor...");
  instance_ = this;

  // Wake up the MPU6050
  this->write_register(0x6B, 0x00);  // Power Management 1: clear sleep mode

  // Configure accelerometer sensitivity (±2g)
  this->write_register(0x1C, 0x00);

  // Configure tap thresholds
  this->write_register(0x1D, this->x_threshold_);
  this->write_register(0x1E, this->y_threshold_);
  this->write_register(0x1F, this->z_threshold_);

  // Configure tap timing parameters
  this->write_register(0x21, this->duration_);  // Tap duration
  this->write_register(0x22, this->latency_);   // Tap latency (between taps)
  this->write_register(0x23, this->window_);    // Tap window (for double taps)

  // Enable both single and double tap detection
  this->write_register(0x38, 0x60);  // Enable single/double tap interrupts

  // Configure interrupt pin
  pinMode(this->interrupt_pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->interrupt_pin_), static_isr_handler, FALLING);

  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO%d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->x_threshold_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->duration_);
  ESP_LOGCONFIG(TAG, "  Latency: 0x%02X", this->latency_);
  ESP_LOGCONFIG(TAG, "  Window: 0x%02X", this->window_);
}

void MPU6050TapSensor::on_tap_detected_() { this->tap_detected_ = true; }

void MPU6050TapSensor::loop() {
  if (this->tap_detected_) {
    this->tap_detected_ = false;

    uint8_t int_status = this->read_register(0x3A);  // Read interrupt status

    if (int_status & 0x10) {
      ESP_LOGD(TAG, "Single Tap Detected!");
      this->publish_state(true);
      this->set_timeout(10, [this]() { this->publish_state(false); });
    }

    if (int_status & 0x20) {
      ESP_LOGD(TAG, "Double Tap Detected!");
      if (this->double_tap_binary_sensor_ != nullptr) {
        this->double_tap_binary_sensor_->publish_state(true);
        this->set_timeout(10, [this]() { this->double_tap_binary_sensor_->publish_state(false); });
      }
    }
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
  LOG_BINARY_SENSOR(TAG, "  Single Tap", this);
  LOG_BINARY_SENSOR(TAG, "  Double Tap", this->double_tap_binary_sensor_);
}

}  // namespace mpu6050_tap
}  // namespace esphome