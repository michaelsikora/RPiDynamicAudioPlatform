Engineer: Michael Sikora <m.sikora@uky.edu>
Date: 2018.05.04
Title: audioPlatform development code
Research for: Dr. Kevin D. Donohue at University of Kentucky
Objective: 1. To operate 2x pan-tilt servo mounts at the same time as 
		      recording and processing audio.
		   2. Process the received signals to obtain the coherent 
			  response power image and adjust the pan-tilt to maximum 
			  image SNR. 
			  
			  
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

