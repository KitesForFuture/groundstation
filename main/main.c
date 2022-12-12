/* WIFI-UART translation
*/

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"

#include "driver/uart.h"

#include "driver/gpio.h"

#include "driver/mcpwm.h"

#include "mymath.c"

#include "uart.c"

#include "motors.c"

#include "my_gpio.c"

#include "RC.c"



#define SERVO_MIN_ANGLE -45
#define SERVO_MAX_ANGLE 54

#define NOT_INITIALIZED -1000000

float currentServoAngle = SERVO_MIN_ANGLE;
float direction = 1;

void controlServoAngle(float reel_in_speed){
	currentServoAngle += direction*0.0045*reel_in_speed;
	if(currentServoAngle > SERVO_MAX_ANGLE) {direction = -1; currentServoAngle = SERVO_MAX_ANGLE;}
	if(currentServoAngle < SERVO_MIN_ANGLE) {direction = 1; currentServoAngle = SERVO_MIN_ANGLE;}
	printf("setting servo angle = %f\n", currentServoAngle);
	setAngle(2, currentServoAngle);
}

void storeServoArmForEnergyGeneration(){
	setAngle(2, -90);
	currentServoAngle = SERVO_MIN_ANGLE;
	direction = 1;
}


void init(){
	initMotors(); // servo (pwm) outputs
	storeServoArmForEnergyGeneration();
	//networking
	//setRole(GROUND_STATION);
	network_setup();
	
	//TODO: set up UART
	initUART();
	
	initGPIO();
	
	init_uptime();
	
}

float line_length = 0;
float line_speed = 0;
float line_length_offset = NOT_INITIALIZED;
float last_line_length = 0;

void app_main(void){
	init();
	//storeServoArmForEnergyGeneration();
	//controlServoAngle(30.0 * reel_in_high_duty_voltage);
	//float landing_request = 0.0;
	while(1){
		
		// pass line length and line tension from UART to WIFI
		float line_length_raw, flight_mode;
		float receive_array[100];
		int receive_array_length;
		while(receiveUARTArray100(&receive_array, &receive_array_length) == 1){
			
			if(receive_array_length == 2){
				line_length_raw = receive_array[0];
				flight_mode = receive_array[1];
			
				line_length = line_length_raw;// - line_length_offset;
				//printf("received %f, %f\n", line_length_raw, flight_mode);
				sendData(LINE_LENGTH_MODE, line_length, flight_mode);
				line_speed = (1-0.125) * line_speed + 0.125 * 50/*frequency*/ * (line_length - last_line_length);
				last_line_length = line_length;
				//printf("line_speed = %f, line_length = %f\n", line_speed, line_length);
				if(-line_speed > 0.5){
					controlServoAngle(30.0 * fabs(line_speed));
				}else{
					storeServoArmForEnergyGeneration();
				}
			}else if(receive_array_length == 37){
				sendDataArrayLarge(CONFIG_MODE, receive_array, 37);
			}
		}
		
		// pass line-tension request from WIFI to UART
		
		if(get_level_GPIO_0() != 0){
			sendUART(3, 0); // request final-landing from VESC
		}
		
	    vTaskDelay(2.0);
    }
    
}
