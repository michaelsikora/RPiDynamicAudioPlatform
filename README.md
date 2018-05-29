Engineer: Michael Sikora <m.sikora@uky.edu>

Date: 2018.05.04

Title: audioPlatform development code

Research for: Dr. Kevin D. Donohue at University of Kentucky

Objective: 
1. To operate 2x pan-tilt servo mounts at the same time as recording and processing audio.
2. Process the received signals to obtain the coherent response power image and adjust the pan-tilt to maximum image SNR. 
			  
			  
Cmake was used to compile. The RtAudio and PCA9685 libraries are needed.


	// PROGRAM OUTLINE
	// 1.Setup Audio for recording only ( Or callback dump )
	
	// 2.Setup PCA9685
	
	// 3.Wait for input to start
	
	// SEQUENCE 1
	// 4.Set both platforms to planar endfire
	
	// 5.Record Audio for 3 seconds
	
	// 6.Set both platforms to planar broadside
	
	// 7.Record Audio for 3 seconds
	
	// 8.Wait for input to continue ( User moves arrays to new location )
	
	// 9. Repeat steps 4 through 8 for 0.5 to 3 meters
	
	
 Idea for future version is to have a menu to select which sequence to run
 so that several tests can be saved

 
 Version 2:
 
 threads:
 1. AUDIO - Audio thread runs a callback with 3 input channels recording and 1
	output channel playing a wav/raw audio file.

 2. PAN-TILT - Control thread for the PCA9685 PWM driver.
 
 3. ANALYSIS - (extra) run SRP algorithm and determine SNR
 
 function:
 1. (extra)  share struct between threads to tell:
	-PAN-TILT Current Orientation
	-PAN-TILT Moving flag
	-PAN-TILT Orientation Queue (next orientation)
	-PAN-TILT Platform setup id for SNR algorithm
	-AUDIO Recording in progress flag
	-AUDIO counter for completed recording processes (or amount of data captured)
	
	requires semaphore/mutexes
	
 2. Run a predefined sequence of orientations and record/play processes
 
	faster to implement
