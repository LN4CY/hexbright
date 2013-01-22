#Advanced functional and tactical Flashlight program for Hexbright

7 Modes of operation
	
	OFF_MODE            //main LED off, system off
	MIN_MODE            //system on, main LED brightness at 12.5%
	LOW_MODE            //system on, main LED brightness at 25%
	MID_MODE            //system on, main LED brightness at 50%
	HIGH_MODE           //system on, main LED brightness at 75%
	MAX_MODE            //system on, main LED brightness at 100%
	ACCEL_MODE          //system on, main LED brightness proportional to accelerometer angle
	STROBE_MODE         //system on, strobe mode select proportional to accelerometer angle

	#GLOW_CHARGE is added to the library to allow a soft dim oin and off of the green LED while charging

	transitions
	sbp: short button press
	pnh: press-n-hold
	br: button release  
                  
				     ACCEL_MODE (hold brigness)----sbp-> go to the LOW/MID/HIGH/MAX/OFF mode that is next
                                                ^         |               based on current brightness
                                                |        pnh        
                                                br        |
												|         v
                                       ACCEL_MODE (adjust brightness)
                                                ^
                      __________________________|_____________________________
                     /           |              |           \                \
                   pnh          pnh            pnh          pnh              pnh
                   /             |              |             \                \
	OFF_MODE->sbp->MIN_MODE->sbp->LOW_MODE->sbp->MID_MODE->sbp->HIGH_MODE->sbp->MAX_MODE->sbp->back to OFF_MODE
   	   \
 	   pnh
	     \
 	  -->STROBE_MODE (adjust freq) 
 	  |        \
 	 pnh        br
 	  |          \
 	  <-----STROBE_MODE(hold value)
