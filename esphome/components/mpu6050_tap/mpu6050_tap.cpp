#include "mpu6050_tap.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace mpu6050_tap {

static const char *const TAG = "mpu6050_tap";
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

  // Configure accelerometer for ±2g range (most sensitive)
  this->write_register(0x1C, 0x00);

  // Configure tap detection thresholds
  this->write_register(0x1D, this->tap_threshold_);  // X-axis threshold
  this->write_register(0x1E, this->tap_threshold_);  // Y-axis threshold
  this->write_register(0x1F, this->tap_threshold_);  // Z-axis threshold

  // Configure tap timing parameters
  this->write_register(0x21, this->tap_duration_);  // Tap duration
  this->write_register(0x28, 0x15);                 // Set Motion Detection Threshold
  this->write_register(0x37, 0x30);                 // Enable Motion Interrupt
  this->write_register(0x38, 0x40);                 // Enable tap detection interrupts

  // Configure interrupt pin
  pinMode(this->interrupt_pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->interrupt_pin_), static_isr_handler, FALLING);

  ESP_LOGCONFIG(TAG, "MPU6050 Tap Sensor initialized:");
  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO%d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->tap_threshold_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->tap_duration_);
  ESP_LOGCONFIG(TAG, "  Double Tap Window: %dms", this->double_tap_window_ms_);
}

void IRAM_ATTR MPU6050TapSensor::on_tap_detected_() {
  // This runs in interrupt context - keep it minimal!
  uint8_t next_head = (this->tap_queue_head_ + 1) % TAP_QUEUE_SIZE;
  if (next_head != this->tap_queue_tail_) {  // Check if queue is not full
    TapEvent &event = this->tap_queue_[this->tap_queue_head_];
    event.timestamp = millis();
    // Note: Reading I2C in ISR is generally not recommended, but MPU6050 has a FIFO
    // that holds the acceleration data from the moment of the tap
    this->read_accel_data_(&event.accel_x, &event.accel_y, &event.accel_z);
    this->tap_queue_head_ = next_head;
  }
}

void MPU6050TapSensor::loop() {
  while (this->tap_queue_tail_ != this->tap_queue_head_) {
    const TapEvent &event = this->tap_queue_[this->tap_queue_tail_];
    process_tap_event_(event);
    this->tap_queue_tail_ = (this->tap_queue_tail_ + 1) % TAP_QUEUE_SIZE;
  }
}

void MPU6050TapSensor::process_tap_event_(const TapEvent &event) {
  uint32_t now = event.timestamp;

  if (this->waiting_for_double_tap_) {
    uint32_t time_since_last_tap = now - this->last_tap_.timestamp;

    if (time_since_last_tap <= this->double_tap_window_ms_) {
      // This is a double tap
      TapDirection dir = detect_tap_direction_(event);
      execute_callbacks_(true, dir);
      this->waiting_for_double_tap_ = false;
    } else {
      // Too much time has passed, treat the previous tap as a single tap
      TapDirection dir = detect_tap_direction_(this->last_tap_);
      execute_callbacks_(false, dir);
      // And start considering this as a potential first tap of a new sequence
      this->last_tap_ = event;
      this->waiting_for_double_tap_ = true;
    }
  } else {
    // This is the first tap
    this->last_tap_ = event;
    this->waiting_for_double_tap_ = true;

    // Set a timeout to handle this as a single tap if no second tap comes
    this->set_timeout(this->double_tap_window_ms_, [this]() {
      if (this->waiting_for_double_tap_) {
        TapDirection dir = detect_tap_direction_(this->last_tap_);
        execute_callbacks_(false, dir);
        this->waiting_for_double_tap_ = false;
      }
    });
  }
}

TapDirection MPU6050TapSensor::detect_tap_direction_(const TapEvent &event) {
  // Simple direction detection based on highest acceleration component
  int16_t abs_x = abs(event.accel_x);
  int16_t abs_y = abs(event.accel_y);
  int16_t abs_z = abs(event.accel_z);

  if (abs_z > abs_x && abs_z > abs_y) {
    return event.accel_z > 0 ? TAP_UP : TAP_DOWN;
  } else if (abs_x > abs_y) {
    return event.accel_x > 0 ? TAP_RIGHT : TAP_LEFT;
  } else {
    return event.accel_y > 0 ? TAP_UP : TAP_DOWN;  // Using up/down for dominant Y axis
  }
}

void MPU6050TapSensor::execute_callbacks_(bool is_double_tap, TapDirection dir) {
  if (is_double_tap) {
    ESP_LOGD(TAG, "Double tap detected: %s",
             dir == TAP_UP      ? "UP"
             : dir == TAP_DOWN  ? "DOWN"
             : dir == TAP_LEFT  ? "LEFT"
             : dir == TAP_RIGHT ? "RIGHT"
                                : "UNKNOWN");

    switch (dir) {
      case TAP_UP:
        this->double_tap_up_trigger_.trigger();
        break;
      case TAP_DOWN:
        this->double_tap_down_trigger_.trigger();
        break;
      case TAP_LEFT:
        this->double_tap_left_trigger_.trigger();
        break;
      case TAP_RIGHT:
        this->double_tap_right_trigger_.trigger();
        break;
      default:
        break;
    }
  } else {
    ESP_LOGD(TAG, "Single tap detected: %s",
             dir == TAP_UP      ? "UP"
             : dir == TAP_DOWN  ? "DOWN"
             : dir == TAP_LEFT  ? "LEFT"
             : dir == TAP_RIGHT ? "RIGHT"
                                : "UNKNOWN");

    switch (dir) {
      case TAP_UP:
        this->single_tap_up_trigger_.trigger();
        break;
      case TAP_DOWN:
        this->single_tap_down_trigger_.trigger();
        break;
      case TAP_LEFT:
        this->single_tap_left_trigger_.trigger();
        break;
      case TAP_RIGHT:
        this->single_tap_right_trigger_.trigger();
        break;
      default:
        break;
    }
  }
}

void MPU6050TapSensor::read_accel_data_(int16_t *x, int16_t *y, int16_t *z) {
  uint8_t buffer[6];

  // Read accelerometer data registers (0x3B - 0x40)
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  if (Wire.available() == 6) {
    buffer[0] = Wire.read();  // ACCEL_XOUT_H
    buffer[1] = Wire.read();  // ACCEL_XOUT_L
    buffer[2] = Wire.read();  // ACCEL_YOUT_H
    buffer[3] = Wire.read();  // ACCEL_YOUT_L
    buffer[4] = Wire.read();  // ACCEL_ZOUT_H
    buffer[5] = Wire.read();  // ACCEL_ZOUT_L

    *x = (buffer[0] << 8) | buffer[1];
    *y = (buffer[2] << 8) | buffer[3];
    *z = (buffer[4] << 8) | buffer[5];
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
  ESP_LOGCONFIG(TAG, "  Interrupt Pin: GPIO%d", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity: 0x%02X", this->tap_threshold_);
  ESP_LOGCONFIG(TAG, "  Duration: 0x%02X", this->tap_duration_);
  ESP_LOGCONFIG(TAG, "  Double Tap Window: %dms", this->double_tap_window_ms_);
}

}  // namespace mpu6050_tap
}  // namespace esphome