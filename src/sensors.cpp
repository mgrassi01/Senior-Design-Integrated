#include "sensors.hpp"


/*
ultrasonic, tilt, and light sensors. 
*/

// ----------------------------------- GLOBAL VARIABLES  --------------------------------//
int ldr_state = 2; // start out assuming dim, the in-between value
int ultrasonic_state = 0; 
int tilt_state = 0;
hw_timer_t* timer0 = nullptr;           // 50 Hz tick ??





// this is edge triggered and will not use many arduino functions
void tilt_isr(void* arg){
  uint32_t gpio_num = (uint32_t) arg;
  gpio_num_t gpio_numT = (gpio_num_t)gpio_num;

  // int level = digitalRead((int)gpio_num); // another option: use the arduino logic
  int level = gpio_get_level(gpio_numT); // check and see what the level is 

  // 4 bit val goes like FRONT | BACK | LEFT | RIGHT
  if(level == HIGH){
    if(gpio_numT == TILT_PIN_R) {
      tilt_state |= 1;
    } else if(gpio_numT == TILT_PIN_L) {
      tilt_state |= (1 << 1);
    } else if(gpio_numT == TILT_PIN_B) {
      tilt_state |= (1 << 2);
    } else if(gpio_numT == TILT_PIN_F) {
      tilt_state |= (1 << 3);
    }
  }

  else if(level == LOW){
    if(gpio_numT == TILT_PIN_R) {
      tilt_state &= ~(1 << 0);
    } else if(gpio_numT == TILT_PIN_L) {
      tilt_state &= ~(1 << 1);
    } else if(gpio_numT == TILT_PIN_F) {
      tilt_state &= ~(1 << 2);
    } else if(gpio_numT == TILT_PIN_B) {
      tilt_state &= ~(1 << 3);
    }
  }

}



// this is timer triggered and will run after the ultrasonics, in a similar way
int ldr(int pin_num)
{
  int ldr_value = 0; // value read from the ldr
  float ldr_voltage = 0.00;
  int ldr_state_next = 0;

  ldr_value = analogRead(pin_num);
  ldr_voltage = float(ldr_value) * 5.00 / 1023.00;
  Serial.print("ldr voltage = ");
  Serial.println(ldr_voltage);


  if(ldr_state == 1){ // very dim
    if(ldr_voltage < (BRIGHT - LDR_BUFFER)) {
      ldr_state_next = 4; // BRIGHT
    } else if(ldr_voltage < (VERY_DIM - LDR_BUFFER)){
      ldr_state_next = 2; // dim
    } else{
      ldr_state_next = 1; // very dim
    }
    
  } else if (ldr_state == 2) { // dim
    if(ldr_voltage < (BRIGHT - LDR_BUFFER)) {
      ldr_state_next = 4;
    } else if (ldr_voltage > (VERY_DIM + LDR_BUFFER)){
      ldr_state_next = 1;
    } else {
      ldr_state_next = 2;
    }
    
  } else if(ldr_state == 4){ // BRIGHT
    if(ldr_voltage > (VERY_DIM + LDR_BUFFER)){
      ldr_state_next = 1;
    } else if(ldr_voltage > (BRIGHT + LDR_BUFFER)) {
      ldr_state_next = 2;
    } else{
      ldr_state_next = 4;
    }
  }
  

  return ldr_state_next;
}





int ultrasonic(int TRIG_PIN, int ECHO_PIN){

  float timing = 0.0;
  float distance = 0.0; 
  int buffer = 2;

  int next_ultrasonic_state = 0;

  // trigger the ultrasonic in order to get a measurement
  digitalWrite(TRIG_PIN, LOW);
  delay(2);
  digitalWrite(TRIG_PIN, HIGH);
  delay(10);
  digitalWrite(TRIG_PIN, LOW);

  // get the input from the echo pin
  timing = pulseIn(ECHO_PIN, HIGH);
  distance = (timing * 0.034) /(2*2.54);
  distance *= 10/8.85; // adjust for accuracy

  // beginning state
  if(ultrasonic_state == 0){
    if(distance <= 12){
      next_ultrasonic_state = 1; // far
    } else if((distance > 12) && (distance < 24)){
      next_ultrasonic_state = 2; // close
    } else if(distance > 24){
      next_ultrasonic_state = 4; // far
    } 
     
  } 
  // very close 
  else if(ultrasonic_state == 1){
    if(distance > (24 + 2)) {
     next_ultrasonic_state = 4; // far
    } else if(distance > (12 + buffer)) {
      next_ultrasonic_state = 2; // close
    }else {
      next_ultrasonic_state = 1; // very close 
    }
  }
  // clBUFFER2ose 
  else if(ultrasonic_state == 2) {
    if(distance > (24 + buffer)) {
      next_ultrasonic_state = 4;
    } else if(distance < (12 - buffer)){
      next_ultrasonic_state = 1;
    } else {
      next_ultrasonic_state = 2;
    }
  }
  // far
  else if(ultrasonic_state == 4) {
    if (distance < (12 - buffer)){
      next_ultrasonic_state = 1; // transition to very close
    } else if (distance < (24 - buffer)){
      next_ultrasonic_state = 2; // transition to close
    } else {
      next_ultrasonic_state = 4; // stay the same
    }
    
  }

  ultrasonic_state = next_ultrasonic_state;

  // Serial.print("Distance : ");
  // Serial.print(distance);
  // Serial.print(" in.  | ultrasonic state: ");
  // Serial.println(ultrasonic_state);

  // if(distance < 1) {
  //   Serial.println("Ouch! Stop hitting me!");
  // }  

  return ultrasonic_state;
}
 



void ultrasonic_ldr_isr(){
  // call the ultrasonic function and get the value
  // call it for L1 pins
  int L1_ultrasonic_state = 0;
  int L2_ultrasonic_state = 0;
  int R1_ultrasonic_state = 0;
  int R2_ultrasonic_state = 0;

  L1_ultrasonic_state = ultrasonic(TRIG_PIN_L1, ECHO_PIN_L1);
  R1_ultrasonic_state = ultrasonic(TRIG_PIN_R1, ECHO_PIN_R1);

  // the order is L2 | R2 | L1 | R1, MSB is L2
  ultrasonic_state = (L1_ultrasonic_state << 3) | (R1_ultrasonic_state ) | (L2_ultrasonic_state << 9) | (R2_ultrasonic_state << 6); // update the state
  
  int ldr1_state = ldr(LDR_PIN1);
  int ldr2_state = ldr(LDR_PIN2);
  ldr_state = (ldr1_state << 3) | ldr2_state;



  
  // arm the alarm for 1,5, or 10 seconds 
  int target = 1; // in seconds
  timerAlarmWrite(timer0, 1000*1000 * target, true); // in us
  timerAlarmEnable(timer0);

}

void timer0_init(){
    // add an interrupt handler
    // make the ultrasonic_ldr_isr the interrupt handler
    // set the alarm for 1 or 5 seconds

    timer0 = timerBegin(0, 80, true); // guessing this brings it down to 1 us?
    timerAttachInterrupt(timer0, &ultrasonic_ldr_isr, true);
    timerAlarmWrite(timer0, 1000*1000, true);
    timerAlarmEnable(timer0);

}


void tilt_init(){
  pinMode(TILT_PIN_B, INPUT);
  pinMode(TILT_PIN_F, INPUT);
  pinMode(TILT_PIN_L, INPUT);
  pinMode(TILT_PIN_R, INPUT);

  // attachInterrupt(digitalPinToInterrupt(TILT_PIN_B), tilt_isr(TILT_PIN_B), (RISING | FALLING) ); // not sure this will work
  // attachInterrupt(digitalPinToInterrupt(TILT_PIN_F), tilt_isr, (RISING | FALLING) ); // not sure this will work
  // attachInterrupt(digitalPinToInterrupt(TILT_PIN_R), tilt_isr, (RISING | FALLING) ); // not sure this will work
  // attachInterrupt(digitalPinToInterrupt(TILT_PIN_L), tilt_isr, (RISING | FALLING) ); // not sure this will work
  
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(gpio_num_t(TILT_PIN_F), tilt_isr, (void*) TILT_PIN_F);
  gpio_isr_handler_add(gpio_num_t(TILT_PIN_B), tilt_isr, (void*) TILT_PIN_B);
  gpio_isr_handler_add(gpio_num_t(TILT_PIN_R), tilt_isr, (void*) TILT_PIN_R);
  gpio_isr_handler_add(gpio_num_t(TILT_PIN_L), tilt_isr, (void*) TILT_PIN_L);

}

void ultrasonic_init() {
  pinMode(ECHO_PIN_L1, INPUT);
  pinMode(ECHO_PIN_R1, INPUT);
  pinMode(ECHO_PIN_L2, INPUT);
  pinMode(ECHO_PIN_R2, INPUT);

  pinMode(TRIG_PIN_L1, OUTPUT);
  pinMode(TRIG_PIN_R1, OUTPUT);
  pinMode(TRIG_PIN_L2, OUTPUT);
  pinMode(TRIG_PIN_R2, OUTPUT);

  digitalWrite(TRIG_PIN_L1, LOW);
  digitalWrite(TRIG_PIN_R1, LOW);
  digitalWrite(TRIG_PIN_L2, LOW);
  digitalWrite(TRIG_PIN_R2, LOW);


}

void ldr_init() {
  pinMode(LDR_PIN1, INPUT);
  pinMode(LDR_PIN2, INPUT);
}


// when the gold lead is tilted DOWN, the circuit closes. 
void sensors_loop() {

  ldr_init();
  ultrasonic_init();
  tilt_init();
  timer0_init();

}
