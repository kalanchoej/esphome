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
  // Wake up the MPU6050 by clearing the SLEEP bit (0x6B register)
  this->write_register(0x6B, 0x00);
  delay(100);  // Wait for MPU6050 to wake up

  // Set accelerometer range to ±2g (0x1C register)
  this->write_register(0x1C, 0x00);
  delay(10);

  // Configure ACCEL_CONFIG2 for low power and DLPF (0x1D register)
  // Example: DLPF_CFG = 2 (92 Hz)
  this->write_register(0x1D, 0x02);
  delay(10);

  // Configure Motion Detection Threshold (0x1F register)
  // Higher value = less sensitive to motion
  this->write_register(0x1F, this->sensitivity_);
  delay(10);

  // Configure Motion Detection Duration (0x20 register)
  // Number of consecutive samples above threshold to trigger
  this->write_register(0x20, this->duration_);
  delay(10);

  // Enable Motion Interrupt (0x38 register)
  this->write_register(0x38, 0x40);  // Set MOT_INT_EN bit
  delay(10);

  // Configure Interrupt Pin (0x37 register)
  this->write_register(0x37, 0x02);  // Set INT_PIN_CFG to active low, open-drain
  delay(10);

  ESP_LOGD(TAG, "MPU6050 configured for motion detection.");
}

void MPU6050TapSensor::on_tap_detected_() {
  // ISR-safe operation: set flag
  this->tap_detected_ = true;
}

void MPU6050TapSensor::loop() {
  if (this->tap_detected_) {
    this->tap_detected_ = false;  // Clear the flag
    ESP_LOGD(TAG, "Tap interrupt detected!");

    // Read INT_STATUS register to confirm interrupt source
    uint8_t int_status = this->read_register(0x3A);
    ESP_LOGD(TAG, "Interrupt Status: 0x%02X", int_status);

    if (int_status & 0x40) {  // Check if MOT_INT bit is set
      // Read acceleration data
      int16_t accel_x, accel_y, accel_z;
      Wire.beginTransmission(0x68);
      Wire.write(0x3B);  // Start with ACCEL_XOUT_H
      Wire.endTransmission(false);
      Wire.requestFrom(0x68, 6);

      if (Wire.available() == 6) {
        accel_x = (Wire.read() << 8) | Wire.read();
        accel_y = (Wire.read() << 8) | Wire.read();
        accel_z = (Wire.read() << 8) | Wire.read();
        ESP_LOGD(TAG, "Acceleration Data - X: %d, Y: %d, Z: %d", accel_x, accel_y, accel_z);

        // Detect tap direction based on acceleration data
        TapDirection dir = detect_tap_direction_(accel_x, accel_y, accel_z);

        if (dir != TAP_UNKNOWN) {
          uint32_t current_time = millis();

          if (awaiting_double_tap_) {
            uint32_t time_since_last_tap = current_time - last_tap_time_;
            if (time_since_last_tap <= 200) {  // Double tap window: 200 ms
              ESP_LOGD(TAG, "Double tap detected: %d", dir);
              execute_callbacks_(true, dir);  // true indicates double tap
              awaiting_double_tap_ = false;
            } else {
              // Previous tap is a single tap
              ESP_LOGD(TAG, "Single tap detected: %d", last_tap_direction_);
              execute_callbacks_(false, last_tap_direction_);
              // Register current tap as the first tap
              last_tap_time_ = current_time;
              last_tap_direction_ = dir;
              awaiting_double_tap_ = true;
            }
          } else {
            // First tap detected, start double tap window
            last_tap_time_ = millis();
            last_tap_direction_ = dir;
            awaiting_double_tap_ = true;

            // Schedule a timeout to treat as single tap if no second tap occurs
            this->set_timeout(200, [this, dir]() {
              if (this->awaiting_double_tap_) {
                ESP_LOGD(TAG, "Single tap detected after timeout: %d", dir);
                this->execute_callbacks_(false, dir);
                this->awaiting_double_tap_ = false;
              }
            });
          }
        } else {
          ESP_LOGD(TAG, "Tap direction unknown.");
        }
      } else {
        ESP_LOGW(TAG, "Failed to read acceleration data.");
      }
    } else {
      ESP_LOGD(TAG, "Interrupt not from motion detection.");
    }
  }
}

TapDirection MPU6050TapSensor::detect_tap_direction_(const int16_t accel_x, const int16_t accel_y,
                                                     const int16_t accel_z) {
  // Simple algorithm to determine tap direction based on dominant acceleration axis
  int16_t abs_x = abs(accel_x);
  int16_t abs_y = abs(accel_y);
  int16_t abs_z = abs(accel_z);

  if (abs_z > abs_x && abs_z > abs_y) {
    return (accel_z > 0) ? TAP_UP : TAP_DOWN;
  } else if (abs_x > abs_y) {
    return (accel_x > 0) ? TAP_RIGHT : TAP_LEFT;
  } else if (abs_y > abs_x) {
    // Depending on device orientation, define Y-axis taps
    // For this example, we'll treat Y-axis as TAP_UNKNOWN
    return TAP_UNKNOWN;
  }

  return TAP_UNKNOWN;
}

void MPU6050TapSensor::execute_callbacks_(bool is_double_tap, TapDirection dir) {
  if (is_double_tap) {
    switch (dir) {
      case TAP_UP:
        if (this->double_tap_up_trigger_ != nullptr) {
          this->double_tap_up_trigger_->trigger();
        }
        break;
      case TAP_DOWN:
        if (this->double_tap_down_trigger_ != nullptr) {
          this->double_tap_down_trigger_->trigger();
        }
        break;
      case TAP_LEFT:
        if (this->double_tap_left_trigger_ != nullptr) {
          this->double_tap_left_trigger_->trigger();
        }
        break;
      case TAP_RIGHT:
        if (this->double_tap_right_trigger_ != nullptr) {
          this->double_tap_right_trigger_->trigger();
        }
        break;
      default:
        break;
    }
  } else {
    switch (dir) {
      case TAP_UP:
        if (this->single_tap_up_trigger_ != nullptr) {
          this->single_tap_up_trigger_->trigger();
        }
        break;
      case TAP_DOWN:
        if (this->single_tap_down_trigger_ != nullptr) {
          this->single_tap_down_trigger_->trigger();
        }
        break;
      case TAP_LEFT:
        if (this->single_tap_left_trigger_ != nullptr) {
          this->single_tap_left_trigger_->trigger();
        }
        break;
      case TAP_RIGHT:
        if (this->single_tap_right_trigger_ != nullptr) {
          this->single_tap_right_trigger_->trigger();
        }
        break;
      default:
        break;
    }
  }
}

void MPU6050TapSensor::write_register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  ESP_LOGD(TAG, "Wrote 0x%02X to register 0x%02X", value, reg);
}

uint8_t MPU6050TapSensor::read_register(uint8_t reg) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 1);
  uint8_t value = Wire.available() ? Wire.read() : 0;
  ESP_LOGD(TAG, "Read 0x%02X from register 0x%02X", value, reg);
  return value;
}

void MPU6050TapSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "MPU6050 Tap Sensor:");
  LOG_BINARY_SENSOR(TAG, "Tap Detected", this);
}

}  // namespace mpu6050_tap
}  // namespace esphome
