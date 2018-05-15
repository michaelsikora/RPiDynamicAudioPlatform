/*
 * servoDemo.c
 * 
 * Programmer: Michael Sikora <m.sikora@uky.edu>
 * Date: 2018.05.07
 * Title: servoDemo
 * Research for: Dr. Kevin D. Donohue at University of Kentucky
 * 
 * 
 */

#include "pca9685.h"
#include "RtAudio.h"

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>

#include <inttypes.h>
#include <math.h>
#include <sys/time.h>


#define PIN_BASE 300
#define MAX_PWM 4096
#define HERTZ 60

uint64_t current_timestamp() {
    struct timespec te; 
    clock_gettime(CLOCK_REALTIME, &te);
    printf("nanoseconds: %lld\n", te.tv_nsec/1000000);
    return te.tv_nsec;
}

// Calculates the length of the pulse in samples
int calcTicks(float impulseMs, int hertz)
{
	float cycleMs = 1000.0f / hertz;
	return (int)(MAX_PWM * impulseMs / cycleMs + 0.5f);
}

int main(int argc, char **argv)
{
	// PROGRAM OUTLINE
	// 1.Setup PCA9685
	int fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		return fd;
	}

	pca9685PWMReset(fd);
	printf("Frequency is set to %d hertz\n", HERTZ);
	printf("Returned fd value from setup is %d\n", fd);

	int pin; // selects which servo to send PWM to
	
	// Servo pulse widths for major orientations. 
	// Program Calibrate was used to find these values
	float servo0[3] = {0.65, 1.5, 2.5}; 
	float servo1[2] = {1.35, 2.35};
	float servo2[3] = {0.55, 1.4, 2.4};
	float servo3[2] = {1.75, 2.9};
	
	// 2.Wait for input to start
	printf("Type any number and enter to continue: ");
	scanf("%d",&pin);
	uint64_t optime;
	uint64_t tic;
	uint64_t toc;
	int delayms = 200;
	
	// SEQUENCE 1
	// 3.Run through orientations
			tic = current_timestamp();
		// DEMONSTRATION SEQUENCE
			pin = 0; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[1], HERTZ)); 
			pin = 1; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[0], HERTZ)); 
			delay(delayms); 
			printf("Orientation 1.1\n"); 
			pin = 2; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[1], HERTZ));  
			pin = 3; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[0], HERTZ));  
			delay(delayms);
			printf("Orientation 1.2\n"); 
			//~ delay(2000);			
			pca9685PWMReset(fd);
			

			toc = current_timestamp();
			optime = (toc-tic)/1000000;
			printf(" Delay: %lld ms \n", optime);
			delay(2000);
			tic = current_timestamp();
			
			pin = 0; // LEFT 90 DEGREES
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[2], HERTZ)); 
			pin = 1; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[0], HERTZ));  
			delay(delayms);
			printf("Orientation 2.1\n");
			pin = 2;
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[2], HERTZ));  
			pin = 3; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[0], HERTZ));  
			delay(delayms);
			printf("Orientation 2.2\n"); 
			pca9685PWMReset(fd);
			
			
			toc = current_timestamp();
			optime = (toc-tic)/1000000;
			printf(" Delay: %lld ms \n", optime);
			delay(2000);
			tic = current_timestamp();
			
			fprintf(stdout, "%u\n", (unsigned)time(NULL)); 
			pin = 0; // RIGHT 90 DEGREES
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[0], HERTZ)); 
			pin = 1;
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[1], HERTZ)); 
			delay(delayms); 
			printf("Orientation 3.1\n");
			pin = 2;
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[0], HERTZ));
			pin = 3;
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[1], HERTZ));  
			delay(delayms);
			printf("Orientation 3.2\n"); 
			//~ delay(2000);
			pca9685PWMReset(fd);
			
			
			toc = current_timestamp();
			optime = (toc-tic)/1000000;
			printf(" Delay: %lld ms \n", optime);
			delay(2000);
			tic = current_timestamp();
			
			fprintf(stdout, "%u\n", (unsigned)time(NULL)); 
			pin = 0; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo0[1], HERTZ));  
			pin = 1; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo1[0], HERTZ));  
			delay(delayms);
			printf("Orientation 4.1\n"); 
			pin = 2; // CENTERED
			pwmWrite(PIN_BASE + pin, calcTicks(servo2[1], HERTZ));  
			pin = 3; // FLAT
			pwmWrite(PIN_BASE + pin, calcTicks(servo3[0], HERTZ));  
			delay(delayms);
			printf("Orientation 4.2\n"); 
			//~ delay(2000);			
			pca9685PWMReset(fd);
			
			toc = current_timestamp();
			optime = (toc-tic)/1000000;
			printf(" Delay: %lld ms \n", optime);
				
	fprintf(stdout, "%u\n", (unsigned)time(NULL)); 
	// 4.Wait for input to continue ( User moves arrays to new location )
	printf("Type any number and enter to continue: ");
	scanf("%d", &pin);
	
	return 0;
}

