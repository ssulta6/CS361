#include "hw5.h"
#include <stdio.h>
#include<pthread.h>
#include <sched.h>

static struct elev {
    int current_floor;
    int direction;
    int occupancy;
    pthread_mutex_t lock;
    pthread_mutex_t passenger_lock;
    pthread_cond_t cond;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
} elevators[ELEVATORS];

void scheduler_init() {	
    for (int i = 0; i < ELEVATORS; i++) {
        elevators[i].current_floor=0;		
        elevators[i].direction=-1;
        elevators[i].occupancy=0;
        elevators[i].state=ELEVATOR_CLOSED;
        pthread_mutex_init(&elevators[i].lock,0);
        pthread_cond_init(&elevators[i].cond,0);
    }
}


void passenger_request(int passenger, int elevator, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
        pthread_mutex_lock(&elevators[elevator].lock);

        if(elevators[elevator].current_floor == from_floor && elevators[elevator].state == ELEVATOR_OPEN && elevators[elevator].occupancy==0) {
            enter(passenger, elevator);
            elevators[elevator].occupancy++;
            waiting=0;

            pthread_cond_signal(&elevators[elevator].cond);
            pthread_mutex_unlock(&elevators[elevator].lock);
        } else {
            pthread_mutex_unlock(&elevators[elevator].lock);
            sched_yield();
        }
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
    while(riding) {
        pthread_mutex_lock(&elevators[elevator].lock);

        if(elevators[elevator].current_floor == to_floor && elevators[elevator].state == ELEVATOR_OPEN) {
            exit(passenger, elevator);
            elevators[elevator].occupancy--;
            riding=0;
            pthread_cond_signal(&elevators[elevator].cond);
            pthread_mutex_unlock(&elevators[elevator].lock);
        } else {
            pthread_cond_signal(&elevators[elevator].cond);
            pthread_mutex_unlock(&elevators[elevator].lock);
        }
    }
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int), int(*floor_vacant)(int, int)){

    pthread_mutex_lock(&elevators[elevator].lock);
    if(elevators[elevator].state == ELEVATOR_ARRIVED) {
        door_open(elevator);
        elevators[elevator].state=ELEVATOR_OPEN;
        // only wait for passenger if we have one onboard or we're on a non-vacant floor
        if (elevators[elevator].occupancy == 1 || floor_vacant(at_floor, elevator) == 0) {
            pthread_cond_wait(&elevators[elevator].cond, &elevators[elevator].lock);
            pthread_mutex_unlock(&elevators[elevator].lock);
        } else {
            pthread_mutex_unlock(&elevators[elevator].lock);
        }
    }
    else if(elevators[elevator].state == ELEVATOR_OPEN) {
        door_close(elevator);
        elevators[elevator].state=ELEVATOR_CLOSED;
        pthread_mutex_unlock(&elevators[elevator].lock);
    }
    else {
        if(at_floor==0 || at_floor==FLOORS-1) 
            elevators[elevator].direction*=-1;
        move_direction(elevator,elevators[elevator].direction);
        elevators[elevator].current_floor=at_floor+elevators[elevator].direction;
        elevators[elevator].state=ELEVATOR_ARRIVED;
        pthread_mutex_unlock(&elevators[elevator].lock);
    }
}
