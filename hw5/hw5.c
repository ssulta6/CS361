#include "hw5.h"
#include <stdio.h>
#include<pthread.h>
#include <sched.h>

static struct elev {
    int current_floor;
    int direction;
    int occupancy;
    int passenger_id;  // id of passenger on board
    pthread_mutex_t lock;
    pthread_cond_t cond;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
} elevators[ELEVATORS];

static struct pass {
    int id;
    int assigned_elevator;
    int to_floor;
    int from_floor;
    enum {WAITING, ENTERED, EXITED} state;

} passengers[PASSENGERS];

pthread_barrier_t passenger_init_barrier;

int passengers_initialized = 0;

// called directly before spawning passenger and elevator threads
void scheduler_init() {	
    for (int i = 0; i < ELEVATORS; i++) {
        elevators[i].current_floor=0;		
        elevators[i].direction=-1;
        elevators[i].occupancy=0;
        elevators[i].state=ELEVATOR_ARRIVED;
        pthread_mutex_init(&elevators[i].lock,0);
        pthread_cond_init(&elevators[i].cond,0);
    }
    // assign an elevator to each passenger
    for (int i = 0; i < PASSENGERS; i++) {
        passengers[i].assigned_elevator = i % ELEVATORS;
    }

    // all elevator threads should wait at a barrier until all passenger threads have initialized
    pthread_barrier_init(&passenger_init_barrier, NULL, PASSENGERS);
}

int floor_is_empty(int current_floor, int current_elevator) {
    // loop over every remaining passenger (assigned to this elevator) and return 0 if any of the them are on this floor
    for (int i = 0; i < PASSENGERS; i++) {
        if (passengers[i].from_floor == current_floor && passengers[i].state != EXITED && passengers[i].assigned_elevator == current_elevator) {
            return 0;
        }
    }
    // return 1 if this floor has no passengers
    return 1;
}


void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{
    // get the elevator we were assigned to
    int elevator = passengers[passenger].assigned_elevator;
    // init passenger struct
    passengers[passenger].id = passenger;
    passengers[passenger].from_floor = from_floor;
    passengers[passenger].to_floor = to_floor;
    passengers[passenger].state = WAITING;
    // now that this passenger has been initialized, wait at barrier
    int ret = pthread_barrier_wait(&passenger_init_barrier);
    // once all passengers are initialized, one of the threads should change the boolean initialized
    if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
        passengers_initialized = 1;

    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
        pthread_mutex_lock(&elevators[elevator].lock);

        if(elevators[elevator].current_floor == from_floor && elevators[elevator].state == ELEVATOR_OPEN && elevators[elevator].occupancy==0) {
            enter(passenger, elevator);
            // mark that this elevator now has this passenger on board
            elevators[elevator].passenger_id = passenger;
            elevators[elevator].occupancy++;
            waiting=0;
    
            // change the state of passenger to ENTERED 
            passengers[passenger].state = ENTERED;

            pthread_cond_signal(&elevators[elevator].cond);
        }
        
        sched_yield();
        pthread_mutex_unlock(&elevators[elevator].lock);
    }
    

    // wait for the elevator at our destination floor, then get out
    int riding=1;
    while(riding) {
        pthread_mutex_lock(&elevators[elevator].lock);

        if(elevators[elevator].current_floor == to_floor && elevators[elevator].state == ELEVATOR_OPEN) {
            exit(passenger, elevator);

            // mark that no one is in this elevator anymore
            elevators[elevator].passenger_id = -1;

            elevators[elevator].occupancy--;
            riding=0;
            // change the state of passenger to EXITED 
            passengers[passenger].state = EXITED;
            
            pthread_cond_signal(&elevators[elevator].cond);
        }
        
        pthread_mutex_unlock(&elevators[elevator].lock);
    }  // end while riding
}  // end passenger_request

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)){
    
    // if the passengers haven't been initialized, don't proceed any further
    // no mutex is needed here because if we falsely read it as a 0 instead of 1, we'll just
    // loop again and get it right the next time.
    if (!passengers_initialized) {
        return;
    }

    pthread_mutex_lock(&elevators[elevator].lock);

    // if elevator arrived, then open door and wait for passenger
    if(elevators[elevator].state == ELEVATOR_ARRIVED) {
        // only open door if someone could get on or off
        // this only happens when passenger onboard wants to exit or when elevator is vacant and someone on this floor can enter

        int wants_to_exit = 0;
        if (elevators[elevator].occupancy > 0) {
            // get passenger id riding in this elevator
            int passenger = elevators[elevator].passenger_id;
            // find out if passenger wants to exit: if elevator full and is at passenger's to_floor
            if (passenger != -1) {
                wants_to_exit = passengers[passenger].to_floor == elevators[elevator].current_floor;
            }
        }

        // only stop and open if we have one onboard who wants to exit or we're on a non-vacant floor
        if (wants_to_exit || (elevators[elevator].occupancy == 0 && floor_is_empty(at_floor, elevator) == 0)) {
            door_open(elevator);
            elevators[elevator].state=ELEVATOR_OPEN;
            // wait for passenger to leave or enter
            pthread_cond_wait(&elevators[elevator].cond, &elevators[elevator].lock);
        } else {
            // close door and dont wait
            elevators[elevator].state=ELEVATOR_CLOSED;
        }
    } // else if elevator open, just close it
    else if(elevators[elevator].state == ELEVATOR_OPEN) {
        door_close(elevator);
        elevators[elevator].state=ELEVATOR_CLOSED;
    } // else if elevator closed, then move one floor and set to ARRIVED
    else {
        if(at_floor==0 || at_floor==FLOORS-1) 
            elevators[elevator].direction*=-1;
        move_direction(elevator,elevators[elevator].direction);
        elevators[elevator].current_floor=at_floor+elevators[elevator].direction;
        elevators[elevator].state=ELEVATOR_ARRIVED;
    }
    
    pthread_mutex_unlock(&elevators[elevator].lock);
}
