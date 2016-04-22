/**************************************************************************************************
 *                             EE5903 REAL-TIME SYSTEMS (CA1 Exercise)                            *
 *                                                                                                *
 *  1. A software designed for a bad apple identification and discarding system (7x24Hrs).        *
 *  2. BUFFER_SIZE, MAX_PROCCS_TIME and AVE_PROCCS_TIME can be changed according to real need.    *
 *  3. Good apples might also be discarded to reduce total finacial loss (SEE ALGORITM BELOW).    *
 *  4. The discarding principle is as below:                                                      *
 *     -------------------------------------------------------------------------------------      *
 *    | 1/ If the quality of an apple is recognized as "BAD", then add the timestamp that   |     *
 *    |    the apple will reach the actuator to disc_stamps[] for being discarded later.    |     *
 *    | 2/ If the time duration between apple_N passing camera and apple_N's photo finishing|     *
 *    |    processing is longer than (5000-990) ms, then add the apple_N+1’s corresponding  |     *
 *    |    info to disc_stamps[].                                                           |     *
 *     -------------------------------------------------------------------------------------      *
 *                                                                                                *
 *                                                                  Created by CHI JI on 02/2/16  *
 *                                                                            E0001795@U.NUS.EDU  *
 *                                                              National University of Singapore  *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "apples.h"

#define BUFFER_SIZE 500 // Can be changed according to real situation.
#define MAX_PROCCS_TIME 5000  // Can be changed according to real situation.
#define AVE_PROCCS_TIME 990  // Can be changed according to real situation.

int discard = 0;  
int cam_loops = 0;
int photo_count = 0;
PHOTO album[BUFFER_SIZE] = {0};
long long disc_stamps[BUFFER_SIZE] = {0};
long long photo_stamps[BUFFER_SIZE] = {0};
pthread_cond_t  cond;   
pthread_t thread1, thread2, thread3;
static pthread_mutex_t photocount, camloops, discstamps, photostamps, photos;

void camera(void);
void actuator(void);
void processor(void);
long long current_timestamp(void);


int main(){

    /* Make sure the global variables can be read/modified safely*/
    pthread_mutex_init(&photocount, NULL);
    pthread_mutex_init(&camloops, NULL);
    pthread_mutex_init(&discstamps, NULL);
    pthread_mutex_init(&photostamps, NULL);
    pthread_mutex_init(&photos, NULL);

    pthread_cond_init(&cond, NULL);
    
    start_test();

    int ret1, ret2, ret3;
    ret1 = pthread_create(&thread1, NULL, (void *)camera, NULL);
    ret2 = pthread_create(&thread2, NULL, (void *)processor, NULL);
    ret3 = pthread_create(&thread3, NULL, (void *)actuator, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    
    end_test();
    
    pthread_mutex_destroy(&photocount);
    pthread_mutex_destroy(&camloops);
    pthread_mutex_destroy(&discstamps);
    pthread_mutex_destroy(&photostamps);
    pthread_mutex_destroy(&photos);
    pthread_cond_destroy(&cond);
    
    return 0;
}


long long current_timestamp(){
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}


/*  Thread 1  */
void camera(void){

    while (more_apples()){

        wait_until_apple_under_camera();
        pthread_mutex_lock(&photos);
        album[photo_count] = take_photo();
        pthread_mutex_unlock(&photos);
        pthread_mutex_lock(&photostamps);
        photo_stamps[photo_count] = current_timestamp();
        pthread_mutex_unlock(&photostamps);
        pthread_mutex_lock(&photocount);
        photo_count = photo_count + 1;
        if (photo_count == BUFFER_SIZE){
            photo_count = 0;
            pthread_mutex_lock(&camloops);
            cam_loops = cam_loops + 1;
            pthread_mutex_unlock(&camloops);
        }
        pthread_mutex_unlock(&photocount);
        
        usleep(700000);//  In case of retaking photos of the same apple.
    }
}


/*  Thread 2  */
void processor(void){
    
    QUALITY quality[BUFFER_SIZE] = {0};
    int bad_apple_count = -1;
    int proccs_loops = 0;
    int count = 0;

    while(more_apples()){

        if ((count + proccs_loops*BUFFER_SIZE) < (photo_count + cam_loops*BUFFER_SIZE)){

            quality[count] = process_photo(album[count]);
            pthread_mutex_lock(&discstamps);
            if ((current_timestamp() - photo_stamps[count]) > (MAX_PROCCS_TIME - AVE_PROCCS_TIME)){

                if (quality[count] == 1){
                    if (bad_apple_count == BUFFER_SIZE-1){
                        disc_stamps[0] = (MAX_PROCCS_TIME + photo_stamps[count]);
                        bad_apple_count = 1;
                    }
                    else if (bad_apple_count == BUFFER_SIZE - 2){
                        disc_stamps[bad_apple_count + 1] = (MAX_PROCCS_TIME + photo_stamps[count]);
                        bad_apple_count = 0;
                    }
                    else{
                        disc_stamps[bad_apple_count + 1] = (MAX_PROCCS_TIME + photo_stamps[count]);
                        bad_apple_count = bad_apple_count + 2;
                    }
                }
                
                else{
                    if (bad_apple_count == BUFFER_SIZE-1)  bad_apple_count = 0;
                    else  bad_apple_count = bad_apple_count + 1;
                }
                
                pthread_mutex_lock(&photos);
                album[count] = 0;  //  Buffer clean-up
                pthread_mutex_unlock(&photos);

                pthread_mutex_lock(&photostamps);
                photo_stamps[count] = 0;  //  Buffer clean-up
                pthread_mutex_unlock(&photostamps);

                quality[count]= 0;  //  Buffer clean-up
                
                if (count == BUFFER_SIZE - 1){
                    count = 0;
                    proccs_loops = proccs_loops + 1;
                    if ( cam_loops == proccs_loops ){
                        pthread_mutex_lock(&camloops);
                        cam_loops = 0;
                        pthread_mutex_unlock(&camloops);
                        proccs_loops = 0;
                    }
                }
                
                else  count = count + 1;

                disc_stamps[bad_apple_count] = (MAX_PROCCS_TIME + photo_stamps[count]);
                discard = 1;
                pthread_cond_signal(&cond);  // Signal for actuator to be prepared
            }
            
            else{
                if (quality[count] == 1){
                    if (bad_apple_count == BUFFER_SIZE-1)  bad_apple_count = 0;
                    else  bad_apple_count = bad_apple_count + 1;

                    disc_stamps[bad_apple_count] = (MAX_PROCCS_TIME + photo_stamps[count]);
                    discard = 1;
                    pthread_cond_signal(&cond);  // Signal for actuator to be prepared
                }
            }
            
            pthread_mutex_unlock(&discstamps);

            pthread_mutex_lock(&photos);
            album[count] = 0;  //  Buffer clean-up
            pthread_mutex_unlock(&photos);

            pthread_mutex_lock(&photostamps);
            photo_stamps[count] = 0;  //  Buffer clean-up
            pthread_mutex_unlock(&photostamps);

            quality[count]= 0;  //  Buffer clean-up
            
            if (count == BUFFER_SIZE - 1){
                count = 0;
                proccs_loops = proccs_loops + 1;
                if ( cam_loops == proccs_loops ){
                    pthread_mutex_lock(&camloops);
                    cam_loops = 0;
                    pthread_mutex_unlock(&camloops);
                    proccs_loops = 0;
                }
            }
            else  count = count + 1;
        }
    }

    pthread_mutex_lock(&discstamps);
    discard = 1;  // Male sure that thread3 will exit when more_apples()=0
    pthread_cond_signal(&cond); 
    pthread_mutex_unlock(&discstamps);
}


/*  Thread 3  */
void actuator(void)
{
    int disc_count = 0;
    while(more_apples()){

        pthread_mutex_lock(&discstamps);  
        while (!discard){
            pthread_cond_wait(&cond, &discstamps);  // Reduce CPU energy consumption
        }
        pthread_mutex_unlock(&discstamps);
        
        while ((disc_stamps[disc_count] != 0)){
            
            if ((current_timestamp()-disc_stamps[disc_count]<100) && (disc_stamps[disc_count]-current_timestamp()< 100)){
                discard_apple();
                pthread_mutex_lock(&discstamps);
                disc_stamps[disc_count] = 0;  //  Buffer clean-up
                pthread_mutex_unlock(&discstamps);
                disc_count = disc_count +1;
                if (disc_count == BUFFER_SIZE)  disc_count = 0;
            }
            
            else if ((current_timestamp()-disc_stamps[disc_count]) > 100){
                pthread_mutex_lock(&discstamps);
                disc_stamps[disc_count] = 0;  //  Buffer clean-up
                pthread_mutex_unlock(&discstamps);
                disc_count = disc_count + 1;
                if (disc_count == BUFFER_SIZE)  disc_count = 0;
            }
            
            usleep(100000);  // Reduce CPU energy consumption
        }
        discard = 0;
    }
}