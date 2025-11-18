#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_sntp.h"
#include "rom/gpio.h"
#include "esp32-hal-timer.h"
#include "esp_timer.h"
#include "esp32-hal-gpio.h"

#define LDR_PIN1 GPIO_NUM_13 // for ADC1_CH0
#define LDR_PIN2 GPIO_NUM_14// for ADC1_CH2 // i think we can do this

// GPIO output/input 
#define TILT_PIN_L GPIO_NUM_1
#define TILT_PIN_R GPIO_NUM_2
#define TILT_PIN_F GPIO_NUM_3
#define TILT_PIN_B GPIO_NUM_4
#define TRIG_PIN_L1 GPIO_NUM_5
#define TRIG_PIN_R1 GPIO_NUM_6
#define ECHO_PIN_L1 GPIO_NUM_7
#define ECHO_PIN_R1 GPIO_NUM_8
#define TRIG_PIN_L2 GPIO_NUM_9
#define TRIG_PIN_R2 GPIO_NUM_10
#define ECHO_PIN_L2 GPIO_NUM_11
#define ECHO_PIN_R2 GPIO_NUM_12

#define BUFFER1 2;
#define BUFFER2 2;
#define BUFFER3 2;

// constant values, can be changed 
#define VERY_DIM 1.75
#define BRIGHT 1.2
#define LDR_BUFFER 0.05 


#define ESP_INTR_FLAG_DEFAULT 0

#define ULTRASONIC_OUTPUT_MASK ((1 << TRIG_PIN_L1) | (1 << TRIG_PIN_L2) | (1 << TRIG_PIN_R1) | (1 << TRIG_PIN_R2))
#define ULTRASONIC_INPUT_MASK ((1 << ECHO_PIN_L1) | (1 << ECHO_PIN_L2) | (1 << ECHO_PIN_R1) | (1 << ECHO_PIN_R2))



