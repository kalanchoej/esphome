#include "mpu6050_tap.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mpu6050_tap {

static const char *const TAG = "mpu6050_tap";
static MPU6050TapSensor *instance_ = nullptr;

// ISR handler to set the tap_detected_ flag
void IRAM_ATTR static_isr_handler() {
  if (instance_ != nullptr) {
    instance_->on_tap_detected_();
  }
}

void MPU6050TapSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MPU6050 Tap Sensor...");

  // Assign the instance for ISR
  instance_ = this;

  // Initialize I2C
  Wire.begin();

  // Configure MPU6050 settings
  this->configure_mpu6050_();

  // Configure interrupt pin
  pinMode(this->interrupt_pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->interrupt_pin_), static_isr_handler, FALLING);

  ESP_LOGCONFIG(TAG, "MPU6050 Tap Sensor initialized:");
  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO%d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->sensitivity_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->duration_);
}

void MPU6050TapSensor::configure_mpu6050_() {
  this->write_register(0x6B, 0x00);                // Wake up MPU6050
  this->write_register(0x1C, 0x00);                // ±2g range
  this->write_register(0x1D, 0x02);                // DLPF at 92 Hz
  this->write_register(0x1F, this->sensitivity_);  // Motion threshold
  this->write_register(0x20, this->duration_);     // Motion duration
  this->write_register(0x38, 0x40);                // Enable motion interrupt
  this->write_register(0x37, 0x02);                // Interrupt pin configuration

  uint8_t who_am_i = this->read_register(0x75);
  if (who_am_i == 0x68) {
    ESP_LOGI(TAG, "WHO_AM_I check passed: MPU6050 detected.");
  } else {
    ESP_LOGW(TAG, "WHO_AM_I check failed: Expected 0x68, got 0x%02X.", who_am_i);
  }
}

void MPU6050TapSensor::on_tap_detected_() { this->tap_detected_ = true; }

void MPU6050TapSensor::loop() {
  if (!this->tap_detected_)
    return;

  this->tap_detected_ = false;
  uint8_t int_status = this->read_register(0x3A);

  if (!(int_status & 0x40)) {
    ESP_LOGD(TAG, "Interrupt was not motion-related.");
    return;
  }

  int16_t accel_x, accel_y, accel_z;
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);  // Start with ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  if (Wire.available() < 6) {
    ESP_LOGW(TAG, "Failed to read acceleration data.");
    return;
  }

  accel_x = (Wire.read() << 8) | Wire.read();
  accel_y = (Wire.read() << 8) | Wire.read();
  accel_z = (Wire.read() << 8) | Wire.read();

  TapDirection dir = detect_tap_direction_(accel_x, accel_y, accel_z);
  if (dir == TAP_UNKNOWN) {
    ESP_LOGD(TAG, "Tap direction unknown.");
    return;
  }

  uint32_t now = millis();
  if (awaiting_double_tap_) {
    if (now - last_tap_time_ <= 200) {
      ESP_LOGD(TAG, "Double tap detected: %s", tap_direction_to_string(dir));
      execute_callbacks_(true, dir);
      awaiting_double_tap_ = false;
    } else {
      ESP_LOGD(TAG, "Single tap detected: %s", tap_direction_to_string(last_tap_direction_));
      execute_callbacks_(false, last_tap_direction_);
      register_tap_as_first(now, dir);
    }
  } else {
    register_tap_as_first(now, dir);
    this->set_timeout(200, [this, dir]() { handle_single_tap_timeout(dir); });
  }
}

TapDirection MPU6050TapSensor::detect_tap_direction_(const int16_t accel_x, const int16_t accel_y,
                                                     const int16_t accel_z) {
  int16_t abs_x = abs(accel_x);
  int16_t abs_y = abs(accel_y);
  int16_t abs_z = abs(accel_z);

  if (abs_z > abs_x && abs_z > abs_y)
    return (accel_z > 0) ? TAP_UP : TAP_DOWN;
  if (abs_x > abs_y)
    return (accel_x > 0) ? TAP_RIGHT : TAP_LEFT;
  return TAP_UNKNOWN;
}

const char *MPU6050TapSensor::tap_direction_to_string(TapDirection dir) {
  switch (dir) {
    case TAP_UP:
      return "Up";
    case TAP_DOWN:
      return "Down";
    case TAP_LEFT:
      return "Left";
    case TAP_RIGHT:
      return "Right";
    default:
      return "Unknown";
  }
}

void MPU6050TapSensor::register_tap_as_first(uint32_t now, TapDirection dir) {
  last_tap_time_ = now;
  last_tap_direction_ = dir;
  awaiting_double_tap_ = true;
  ESP_LOGD(TAG, "First tap registered: %s", tap_direction_to_string(dir));
}

void MPU6050TapSensor::handle_single_tap_timeout(TapDirection dir) {
  if (awaiting_double_tap_) {
    ESP_LOGD(TAG, "Single tap detected after timeout: %s", tap_direction_to_string(dir));
    execute_callbacks_(false, dir);
    awaiting_double_tap_ = false;
  }
}

DirectionTrigger *MPU6050TapSensor::get_single_tap_trigger(TapDirection dir) {
  if (dir >= TAP_UP && dir <= TAP_RIGHT) {
    return &single_tap_triggers_[dir];
  }
  return nullptr;
}

DirectionTrigger *MPU6050TapSensor::get_double_tap_trigger(TapDirection dir) {
  if (dir >= TAP_UP && dir <= TAP_RIGHT) {
    return &double_tap_triggers_[dir];
  }
  return nullptr;
}

void MPU6050TapSensor::execute_callbacks_(bool is_double_tap, TapDirection dir) {
  DirectionTrigger *trigger = is_double_tap ? &double_tap_triggers_[dir] : &single_tap_triggers_[dir];
  if (trigger != nullptr) {
    trigger->trigger();
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

void MPU6050TapSensor::dump_config() { ESP_LOGCONFIG(TAG, "MPU6050 Tap Sensor:"); }

}  // namespace mpu6050_tap
}  // namespace esphome
