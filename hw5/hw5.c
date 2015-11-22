#include "hw5.h"
#include <stdio.h>
#include<pthread.h>

pthread_mutex_t lock;


// TODO create a struct for elevator properties
int current_floor;
int direction;
int occupancy;
// TODO create an array of elevators

pthread_barrier_t door_barrier;

enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	

void scheduler_init() {	
		current_floor=0;		
		direction=-1;
		occupancy=0;
		state=ELEVATOR_CLOSED;
		pthread_mutex_init(&lock,0);
                pthread_barrier_init(&door_barrier, NULL, 2);
}


void passenger_request(int passenger, int from_floor, int to_floor, 
											 void (*enter)(int, int), 
											 void(*exit)(int, int))
{	
        // log(0,"DEBUG passenger %d request called!\n", passenger);
	// wait for the elevator to arrive at our origin floor, then get in
	int waiting = 1;
	while(waiting) {
		pthread_mutex_lock(&lock);
		
		if(current_floor == from_floor && state == ELEVATOR_OPEN && occupancy==0) {
			enter(passenger, 0);
			occupancy++;
			waiting=0;
                        
                        log(0,"DEBUG passenger: %d getting in elevator on floor %d\n", passenger, current_floor);
		        pthread_mutex_unlock(&lock);
                        // TODO now release door barrier here
                        pthread_barrier_wait(&door_barrier);
                        log(0,"DEBUG passenger: %d resetting door barrier\n", passenger);
                        // TODO reset the barrier
                        pthread_barrier_destroy(&door_barrier);
                        pthread_barrier_init(&door_barrier, NULL, 2);
		} else {
		    pthread_mutex_unlock(&lock);
		}
	}

	// wait for the elevator at our destination floor, then get out
	int riding=1;
	while(riding) {
		pthread_mutex_lock(&lock);

		if(current_floor == to_floor && state == ELEVATOR_OPEN) {
			exit(passenger, 0);
			occupancy--;
			riding=0;
                        log(0,"DEBUG passenger: %d getting off elevator on floor %d\n", passenger, current_floor);
		        pthread_mutex_unlock(&lock);
                        // TODO now release door barrier here
                        pthread_barrier_wait(&door_barrier);
                        log(0,"DEBUG passenger: %d resetting door barrier\n", passenger);
                        // TODO reset the barrier
                        pthread_barrier_destroy(&door_barrier);
                        pthread_barrier_init(&door_barrier, NULL, 2);
		} else {
		        pthread_mutex_unlock(&lock);
                        // TODO now release door barrier here
                        pthread_barrier_wait(&door_barrier);
                        log(0,"DEBUG passenger: %d resetting door barrier\n", passenger);
                        // TODO reset the barrier
                        pthread_barrier_destroy(&door_barrier);
                        pthread_barrier_init(&door_barrier, NULL, 2);
                }
	}
}

void elevator_ready(int elevator, int at_floor, 
										void(*move_direction)(int, int), 
										void(*door_open)(int), void(*door_close)(int), int(*floor_vacant)(int)){
        // log(0,"DEBUG elevator %d ready called!\n", elevator);
	if(elevator!=0) return;
	
	pthread_mutex_lock(&lock);
	if(state == ELEVATOR_ARRIVED) {
		door_open(elevator);
		state=ELEVATOR_OPEN;
                log(0,"DEBUG elevator arrived on floor %d\n", at_floor);
                // only wait for passenger if we have one onboard or we're on a non-vacant floor
                if (occupancy == 1 || floor_vacant(current_floor) == 0) {
                    log(0,"DEBUG elevator waiting on door barrier at: %d with occupancy: %d\n", at_floor, occupancy);
                    pthread_mutex_unlock(&lock);
                    pthread_barrier_wait(&door_barrier);
                    log(0,"DEBUG elevator DONE waiting on door barrier at: %d\n", at_floor);
                } else {
                    pthread_mutex_unlock(&lock);
                }
	}
	else if(state == ELEVATOR_OPEN) {
                log(0,"DEBUG elevator open at floor: %d, closing!\n", at_floor);
		door_close(elevator);
		state=ELEVATOR_CLOSED;
	        pthread_mutex_unlock(&lock);
	}
	else {
		if(at_floor==0 || at_floor==FLOORS-1) 
			direction*=-1;
                log(0,"DEBUG elevator at floor: %d, moving %d!\n", at_floor, direction);
		move_direction(elevator,direction);
		current_floor=at_floor+direction;
		state=ELEVATOR_ARRIVED;
	        pthread_mutex_unlock(&lock);
	}
}
