RealtimeSystem
Holding my CA scripts

                            EE5903 REAL-TIME SYSTEMS (CA1 Exercise)                            
                                                                                               
  1. A software designed for a bad apple identification and discarding system (7x24Hrs).       
  2. BUFFER_SIZE, MAX_PROCCS_TIME and AVE_PROCCS_TIME can be changed according to real need.   
  3. Good apples might also be discarded to reduce total finacial loss (SEE ALGORITM BELOW).   
  4. The discarding principle is as below:                                                     
-------------------------------------------------------------------------------------     
  1/ If the quality of an apple is recognized as "BAD", then add the timestamp that       
     the apple will reach the actuator to disc_stamps[] for being discarded later.        
  2/ If the time duration between apple_N passing camera and apple_N's photo finishing    
     processing is longer than (5000-990) ms, then add the apple_N+1’s corresponding      
     info to disc_stamps[].                                                               
-------------------------------------------------------------------------------------     
                                                                                               
                                                        Created by CHI JI on 02/2/16 
                                                                  E0001795@U.NUS.EDU 
                                                    National University of Singapore 
 
