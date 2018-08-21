Engineer: Michael Sikora <m.sikora@uky.edu>

Date: 2018.05.04

Title: audioPlatform development code

Research for: Dr. Kevin D. Donohue at University of Kentucky

# Objective: 
1. To operate 2x pan-tilt servo mounts at the same time as recording and processing audio.
2. Process the received signals to obtain the coherent response power image and adjust the pan-tilt to maximum image SNR. 
			  
		
# Build
Cmake was used to compile. The RtAudio and PCA9685 libraries are needed.

# V.1 Code Design
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
	
# Future
 Idea for future version is to have a menu to select which sequence to run
 so that several tests can be saved
 
 # V.2 Multi-threading
## threads:
 1. AUDIO - Audio thread runs a callback with 3 input channels recording and 1
	output channel playing a wav/raw audio file.

 2. PAN-TILT - Control thread for the PCA9685 PWM driver.
 
 3. ANALYSIS - (extra) run SRP algorithm and determine SNR
 
## function:
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

# Video Test

Verification of dual Servo PWM over 3 meter snake cable

<a href="https://www.youtube.com/watch?v=nqSF_LqeNWM
" target="_blank"><img src="http://img.youtube.com/vi/nqSF_LqeNWM/0.jpg" 
alt="IMAGE ALT TEXT HERE" width="240" height="180" border="10" /></a>

# Version Control

Development is now being done on several computers. The Commits are being made daily and
when needing to switch between computers. Commit comments are in the following format:

git commit -m "TIME LOCATION DATE"

### TIME

EOD - End of Day, MOD - Middle of Day, AH - After Hours, T - Testing

### Location

Desktop, Laptop, RPi

### Date

MM.DD
 
 # Error
Error periodically occurs where the callback will throw an error and fail.
The Error message has been found in the source code at:
https://github.com/thestk/rtaudio/blob/master/RtAudio.cpp#L7915

Message thrown is:
RtApiAlsa::callbackEvent: audio read error, Input/output error.

Error can be recreated by running program with AUDIOINOUT task once. Then,
running the program a second time. The error regularly occurs on the second
run. Running the program again sometimes gives no error.
 
 
# Websocket
A python app was started to interface to the c program over a websocket. At time of writing the Audio threads do not record correctly when run from the python subprocess.
