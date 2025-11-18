#include "sensors.hpp"


/*

for timimg for the ultrasonic: the input triggers one interrupt on a rising edge. 
the ISR records the current time /starts a timer and either puts it into a global variable
or puts it into dma memory or something
the input triggers another interrupt on a falling edge
this interrupt handler records the current time, then subtracts the last current time from this the current time 
and we have the time that the pin was high. 
maybe use pulseIn? from wiring_pulse.c

*/
// this should be triggered by a timer that goes off every 5 seconds
struct timeval tv_rising_edge;
struct timeval tv_falling_edge;
int64_t time_rising_edge;
int64_t time_falling_edge;
int64_t time_echo;
uint32_t offset = 0;
int isr_index = 0;
int time_arr[] = {};


hw_timer_t* ultrasonic_timer = nullptr;           // 50 Hz tick ??
hw_timer_t* ultrasonic_backup_timer = nullptr;    





      


// could possibly set the SAME timer, but then that timer gets reduced to a shorter period of time
// void ultrasonic_backup_timer_init(){
//     ultrasonic_backup_timer = timerBegin(0, 80, true); // guessing this brings it down to 1 us?
//     timerAttachInterrupt(ultrasonic_backup_timer, &ultrasonic_trig_isr, true);
//     timerAlarmWrite(ultrasonic_backup_timer, 1000*1000, true);
//     timerAlarmEnable(ultrasonic_backup_timer); 
// }



void ultrasonic_timer_init(){
    // add an interrupt hadnler
    // make the ultrasonic_trig_isr the interrupt handler
    // set the alarm for 1 or 5 seconds

    ultrasonic_timer = timerBegin(0, 80, true); // guessing this brings it down to 1 us?
    timerAttachInterrupt(ultrasonic_timer, &ultrasonic_trig_isr, true);
    timerAlarmWrite(ultrasonic_timer, 1000*1000, true);
    timerAlarmEnable(ultrasonic_timer);
    // arm the alarm
    // timerStart();
    // timerStop();
    // timerRead();

    // gptimer_handle_t gptimer = NULL;
    // gptimer_config_t timer_config = {
    //     .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
    //     .direction = GPTIMER_COUNT_UP,      // Counting direction is up
    //     .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    // };
    // // Create a timer instance
    // ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    // // Enable the timer
    // ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // // Start the timer
    // ESP_ERROR_CHECK(gptimer_start(gptimer));
  // Timer (unchanged) ...
}

void ultrasonic_trig_isr() {
    // send a 10us pulse
    isr_index = isr_index % 4 + 1; // increment index here, NOT the echo isr, in case the echo isr doesnt get called

    gpio_num_t TRIG_PIN;
    // decides which ultrasonic sensor to trigger 
    if (isr_index == 1) TRIG_PIN = TRIG_PIN_L1;
    if (isr_index == 2) TRIG_PIN = TRIG_PIN_L2;
    if (isr_index == 3) TRIG_PIN = TRIG_PIN_R1;
    if (isr_index == 4) TRIG_PIN = TRIG_PIN_R2;
    
    gpio_set_level(TRIG_PIN, 1);
    delayMicroseconds(10); // maybe should change this to another interrupt??
    gpio_set_level(TRIG_PIN_L1, 0);
    timerAlarmWrite(ultrasonic_timer, 1000*1000* 30, true); // arm the timer for 30 seconds to try again on a different ultrasonic 

}


//if the echo never receives a falling edge pulse, the ultrasonic never arms the alarm and sends another trig pulse
// we need a timer to tell us if the ultrasonic 


void ultrasonic_echo_isr(void* arg){
    
    // from https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/system_time.html   
    uint32_t gpio_num = (uint32_t) arg; // check and see which pin triggered this interrupt
    int level = gpio_get_level((gpio_num_t)gpio_num); // check and see what the level is 

    if (level == 1) {
        // start the timer, essentially
        gettimeofday(&tv_rising_edge, NULL);
        time_rising_edge = (int64_t)tv_rising_edge.tv_sec * 1000000L + (int64_t)tv_rising_edge.tv_usec;

    }
    if(level == 0){
        // stop the timer, essentially
        gettimeofday(&tv_falling_edge, NULL);
        time_falling_edge = (int64_t)tv_falling_edge.tv_sec * 1000000L + (int64_t)tv_falling_edge.tv_usec;
        time_echo = time_rising_edge = time_falling_edge; // get high time
        // put the value into the right value in the array
        time_arr[isr_index] = time_echo;
        // incrememt the index for next time: index is from 1-4
       
        int target; // ms to wait before calling the next function 
        int big_wait = 5; // number of seconds to wait between each set of measurements 
        if (isr_index == 4) {target = 1000 * big_wait;}
        else { target = 10;} // wait 10ms before the trigger interrupt is called
        timerAlarmWrite(ultrasonic_timer, 1000* target, true);  // arm the alarm
    }
}


// initialises the Ultrasonics to be GPIO

void ultrasonic_init(){
    gpio_config_t trig_conf = {};
    //disable interruspt
    trig_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    trig_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    trig_conf.pin_bit_mask = ULTRASONIC_OUTPUT_MASK;
    //disable pull-down mode
    trig_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // maybe enable the pulldowns??
    //disable pull-up mode
    trig_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&trig_conf);

    gpio_config_t echo_conf = {};
    echo_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    echo_conf.pin_bit_mask = ULTRASONIC_INPUT_MASK;
    //set as input mode
    echo_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    echo_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    echo_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&echo_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT); // allows per-pin GPIO interrupts
    gpio_isr_handler_add(ECHO_PIN_L1, ultrasonic_echo_isr, (void*) ECHO_PIN_L1);
    // just get one working for now, then add the others
    
    // gpio_isr_handler_add(TRIG_PIN_L2, gpio_isr_handler, (void*) TRIG_PIN_L2);
    // gpio_isr_handler_add(TRIG_PIN_R1, gpio_isr_handler, (void*) TRIG_PIN_R1);
    // gpio_isr_handler_add(TRIG_PIN_R2, gpio_isr_handler, (void*) TRIG_PIN_R2);
    
    gpio_intr_enable(ECHO_PIN_L1);// not sure we need this


    // set all the TRIG pins low, initially
    gpio_set_level(TRIG_PIN_L1, 0);
    gpio_set_level(TRIG_PIN_R1, 0);
    gpio_set_level(TRIG_PIN_L2, 0);
    gpio_set_level(TRIG_PIN_R2, 0);

    // xTimerStartFromISR();

}


// initialises the LDRs to be initialised for ADC
void ldr_init(){


}


// triggered on a timer, every 5 seconds
// updates the LED output
void ldr_isr(){


}

// initialises the tilt sensors ti be GPIO inputs
// triggers an ISR on a rising edge 
// 
void tilt_init(){

}



void tilt_isr(){


}










/*
 * SPDX-FileCopyrightText: 2020-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */



/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO_OUTPUT_IO_0: output
 * GPIO_OUTPUT_IO_1: output
 * GPIO_INPUT_IO_0:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO_INPUT_IO_1:  input, pulled up, interrupt from rising edge.
 *
 * Note. You can check the default GPIO pins to be used in menuconfig, and the IOs can be changed.
 *
 * Test:
 * Connect GPIO_OUTPUT_IO_0 with GPIO_INPUT_IO_0
 * Connect GPIO_OUTPUT_IO_1 with GPIO_INPUT_IO_1
 * Generate pulses on GPIO_OUTPUT_IO_0/1, that triggers interrupt on GPIO_INPUT_IO_0/1
 *
 */

// #define GPIO_OUTPUT_IO_0    CONFIG_GPIO_OUTPUT_0
// #define GPIO_OUTPUT_IO_1    CONFIG_GPIO_OUTPUT_1
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
// /*
//  * Let's say, GPIO_OUTPUT_IO_0=18, GPIO_OUTPUT_IO_1=19
//  * In binary representation,
//  * 1ULL<<GPIO_OUTPUT_IO_0 is equal to 0000000000000000000001000000000000000000 and
//  * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
//  * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
//  * */
// #define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
// #define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
// #define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
// /*
//  * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
//  * In binary representation,
//  * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
//  * 1ULL<<GPIO_INPUT_IO_1 is equal to 0000000000000000000000000000000000100000
//  * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
//  * */
// #define ESP_INTR_FLAG_DEFAULT 0

// static QueueHandle_t gpio_evt_queue = NULL;

// static void IRAM_ATTR gpio_isr_handler(void* arg)
// {
//     uint32_t gpio_num = (uint32_t) arg;
//     xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
// }

// static void gpio_task_example(void* arg)
// {
//     uint32_t io_num;
//     for (;;) {
//         if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
//             printf( "GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level((gpio_num_t)io_num));
//         }
//     }
// }

// void app_main(void)
// {
//     //zero-initialize the config structure.
//     gpio_config_t io_conf = {};
//     //disable interrupt
//     io_conf.intr_type = GPIO_INTR_DISABLE;
//     //set as output mode
//     io_conf.mode = GPIO_MODE_OUTPUT;
//     //bit mask of the pins that you want to set,e.g.GPIO18/19
//     io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
//     //disable pull-down mode
//     io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
//     //disable pull-up mode
//     io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
//     //configure GPIO with the given settings
//     gpio_config(&io_conf);

//     //interrupt of rising edge
//     io_conf.intr_type = GPIO_INTR_POSEDGE;
//     //bit mask of the pins, use GPIO4/5 here
//     io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
//     //set as input mode
//     io_conf.mode = GPIO_MODE_INPUT;
//     //enable pull-up mode
//     io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
//     gpio_config(&io_conf);

//     //change gpio interrupt type for one pin
//     gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

//     //create a queue to handle gpio event from isr
//     gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
//     //start gpio task
//     xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

//     //install gpio isr service
//     gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
//     //hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
//     //hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

//     //remove isr handler for gpio number.
//     gpio_isr_handler_remove(GPIO_INPUT_IO_0);
//     //hook isr handler for specific gpio pin again
//     gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


//     printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

//     int cnt = 0;
//     while (1) {
//         printf("cnt: %d\n", cnt++);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//         gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
//         gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
//     }
// }