#include "plane.h"

int main() {

    signal(SIGTERM, cleanup_and_exit); // Setup signal handling
    printf("Starting the cargo plane simulation...\n"); 
    fflush(stdout);

    // Initialize shared memory and semaphores
    srand(time(NULL));
    //int shm_id, sem_id;
    SharedState *shared_state;

    setup_ipc(&shm_id, &sem_id, &shared_state); 
    CargoPlane planes[num_planes];

    initialize_planes(planes, num_planes, sem_id, shared_state);
    printf("Launching plane processes...\n");
    fflush(stdout);
    for (int i = 0; i < num_planes; i++) {
        if (fork() == 0) {
            simulate_plane_activity(&planes[i]);
            exit(0);
        }
    }


    while (wait(NULL) > 0);  // Wait for all child processes to finish
    printf("All planes have completed their missions.\n");
    fflush(stdout);
    // Cleanup and final output
    print_queue_contents(Container_queue_id);
    cleanup_and_exit(0); // Normal cleanup if no signal
}

void initialize_planes(CargoPlane *planes, int num_planes, int sem_id, SharedState *shared_state) {
    printf("Initializing cargo planes...\n");
    fflush(stdout);
    for (int i = 0; i < num_planes; i++) {
        planes[i].id = i;
        planes[i].container_count = rand_range(1, MAX_CONTAINERS);
        planes[i].drop_interval = rand_range(MIN_DROP_INTERVAL, MAX_DROP_INTERVAL);
        planes[i].is_refilling = 0;
        planes[i].sem_id = sem_id;
        planes[i].shared_state = shared_state;
        printf("Plane %d initialized with %d containers and a drop interval of %d seconds.\n", i, planes[i].container_count, planes[i].drop_interval);
        fflush(stdout);
    }
}

void initialize_container(Container *container, int plane_id, int container_index) {
    container->msg_type = 1;
    container->id = ((plane_id+num_planes+1) * 100) + container_index;  // Generate ID from plane ID and container index
    container->weight = 10;  // Assuming each container has 100kg of flour
    container->status = 1;    // Assume intact unless hit
    container->altitude = INITIAL_ALTITUDE;
    container->index = container_index;
}

void entere_airspace(CargoPlane *plane) {
    printf("Plane %d is ready to enter the airspace...\n", plane->id);
    fflush(stdout);
    struct sembuf op = {0, -1, 0}; // P-operation (wait and lock)
    struct sembuf v_op = {0, 1, 0}; // V-operation (signal and unlock)

    while (1) {
        semop(plane->sem_id, &op, 1); // Attempt to decrement semaphore (enter airspace)
        if (plane->shared_state->planes_in_airspace < MAX_PLANES_IN_AIRSPACE && variables->planes_in_airspace < MAX_PLANES_IN_AIRSPACE ) {
            plane->shared_state->planes_in_airspace++; // Mark entry into airspace
            variables->planes_in_airspace++; 
            variables->plane_ids_in_airspace[variables->count_in_airspace] = plane->id;
variables->count_in_airspace++;
    printf("---------Debug: Plane %d entered airspace. Count in airspace: %d\n", plane->id, variables->count_in_airspace);

            
    print_debug_info();
            printf("Plane %d has entered the airspace.\n", plane->id);
            fflush(stdout);
            semop(plane->sem_id, &v_op, 1); // Increment semaphore as more can still enter
            break;
        }
        semop(plane->sem_id, &v_op, 1); // Increment semaphore if not able to enter
        sleep(1); // Wait before retrying to reduce CPU load
    }

    perform_drop_operations(plane,Container_queue_id);  // Correctly placed to perform dropping operations after entering airspace
    
    // After dropping, reduce plane count in airspace
    semop(plane->sem_id, &op, 1); // Lock semaphore for airspace exit management
    plane->shared_state->planes_in_airspace--;
    variables->planes_in_airspace--;
    ///////////////
    printf("Plane %d has exited the airspace.\n", plane->id);
    fflush(stdout);
    semop(plane->sem_id, &v_op, 1); // Unlock semaphore

    refill_plane(plane); // Now that the plane is out of the airspace, start refilling
}

int check_collision(int planes_in_airspace) { 
   // good logic according to num of planes in airspace not randomly
    double collision_chance = COLLISION_PROBABILITY / 100.0 * (planes_in_airspace * (planes_in_airspace - 1) / 2);
    double rand_val = (double)rand() / RAND_MAX;
    return rand_val < collision_chance;
}

int simulate_missile_hit(int altitude) {
    double random_chance = (double)rand() / RAND_MAX; // Random number between 0 and 1
    double hit_probability = 0.5 + (10000 - altitude) * 0.0002;  // Increased base probability and scaling factor
    sleep(1);
 
    if (random_chance < hit_probability) {
        if (altitude > HIGH_ALTITUDE_THRESHOLD) {
            return 0; // Container destroyed
        } else if (altitude > MEDIUM_ALTITUDE_THRESHOLD) {
            return 2; // Container partially intact
        } else {       
            return 1; // Container mostly intact
        }
    }
    return 1; // No hit, container intact
}

void perform_drop_operations(CargoPlane *plane, int Container_queue_id) {
    struct sembuf op = {0, -1, 0};
    struct sembuf v_op = {0, 1, 0};
    Container container;
    int current_altitude = 10000; // Initial altitude for dropping
    srand(time(NULL)); // Seed random number generator

    printf("Plane %d begins dropping operations...\n", plane->id);
    fflush(stdout);
    
    int container_index = 1;  // Start at 1 for the first container to be dropped


    while (plane->container_count > 0 && !plane->is_refilling && current_altitude > 0) {
    
        
        semop(plane->sem_id, &op, 1);  // Lock the drop zone
        if (plane->shared_state->in_drop_zone == 0) {
            plane->shared_state->in_drop_zone = 1;
            
            initialize_container(&container, plane->id, variables->container_count);

            container.altitude = current_altitude;
            
           
            
            printf("Plane %d is dropping a container. Containers left: %d\n", plane->id, plane->container_count - 1);
            fflush(stdout);


	    sem_wait(&sem);
             // Here, we add the container to shared state
            int idx = variables->container_count++;
            variables->containers[idx] = container;
      	    //variables->containers[idx].index= variables->container_count;
            
            int hit_result = simulate_missile_hit(current_altitude);
            
            if (hit_result == 0) {
            container.status = 0;

            plane->shared_state->container_destroyed++;

            printf("-- Container Destroyed = %d --\n",plane->shared_state->container_destroyed);
            printf("Container ID %d destroyed at high altitude %d.\n", container.id, current_altitude);
            } else if (hit_result == 2) {
                container.weight /= 2;
                container.status = 2;
                printf("Container ID %d hit but partially intact at medium altitude %d, weight now %dkg.\n", container.id, current_altitude, container.weight);
                if (msgsnd(Container_queue_id, &container, sizeof(container) - sizeof(long), 0)==-1){
                perror("msgsnd failed");
                }
            } else {
                container.status = 1;
                printf("Container ID %d dropped safely from low altitude %d.\n", container.id, current_altitude);
                if (msgsnd(Container_queue_id, &container, sizeof(container) - sizeof(long), 0)==-1){
                perror("msgsnd failed");
                }
            }
            
            variables->containers[idx] = container;  // Update the container's status in shared memory

            sem_post(&sem);

            plane->container_count--;
            current_altitude -= 1000; // Decrement altitude after each drop
            container_index++;  // Increment the index for the next container
            plane->shared_state->in_drop_zone = 0;
            printf("Plane %d has finished dropping and reset the drop zone.\n", plane->id);
            fflush(stdout);
        }
        semop(plane->sem_id, &v_op, 1);  // Unlock the drop zone
        sleep(plane->drop_interval);
    }

    plane->is_refilling = 1;
    
        variables->plane_ids_exited_refill[variables->count_exited_refill++] = plane->id;
    printf("Plane %d is exiting for refilling. Refill count: %d\n", plane->id, variables->count_exited_refill);
    semop(plane->sem_id, &v_op, 1); // Unlock semaphore after decrementing count
 

}

void refill_plane(CargoPlane *plane) {
         printf("Plane %d is refilling...\n", plane->id);
        fflush(stdout);
        sleep(REFILL_TIME);
        plane->is_refilling = 0;
        plane->container_count = rand_range(1, MAX_CONTAINERS);
        if (should_simulation_end(plane, shared_state)) {
        printf("Simulation ending: conditions met after refilling.\n");
        kill(0, SIGTERM); // Send SIGTERM to all processes in the group
        exit(0); // End simulation based on other conditions
    }       
        printf("Plane %d has refilled and now has %d containers.\n", plane->id, plane->container_count);
        fflush(stdout);
        entere_airspace(plane);  // Re-enter the airspace after refilling
         
}


int find_colliding_plane_id(int current_plane_id) {
    if (variables->count_in_airspace <= 1) {
        // If the current plane is the only one in the airspace, return an invalid ID
        return -1;
    }

    int index = rand() % variables->count_in_airspace;
    // Ensure we don't select the current plane
    while (variables->plane_ids_in_airspace[index] == current_plane_id) {
        index = rand() % variables->count_in_airspace;
    }

    return variables->plane_ids_in_airspace[index];
}


void simulate_plane_activity(CargoPlane *plane) {
    int collision_detected = 0;
    while (!collision_detected && !should_simulation_end(plane, shared_state)) {
        signal(SIGTERM, cleanup_and_exit);

        entere_airspace(plane);

        if (check_collision(plane->shared_state->planes_in_airspace)) {
            plane->shared_state->collision_count++;
            collision_detected = 1;  // Set collision flag

            int colliding_plane_id = find_colliding_plane_id(plane->id);
            printf("-- COLLISION --\n");
            printf("Collision detected! Plane %d and %d exiting airspace and stopping operations.\n", plane->id, colliding_plane_id);

            variables->plane_ids_exited_collision[variables->count_exited_collision++] = plane->id;
            variables->plane_ids_exited_collision[variables->count_exited_collision++] = colliding_plane_id;

            printf("--------Debug: Collision! Plane %d and %d exited. Collision count: %d\n", plane->id, colliding_plane_id, variables->count_exited_collision);

        }

        if (!collision_detected && plane->container_count == 0) {
            refill_plane(plane);
        }

        if (collision_detected) {
            printf("Plane %d is exiting due to collision. Total collisions: %d\n", plane->id, plane->shared_state->collision_count);
            break;  // Exit loop and thus end this plane's activity
        }
    }

    if (should_simulation_end(plane, shared_state)) {
        printf("Simulation ending due to conditions met.\n");
        kill(0, SIGTERM);
        exit(0);
    }
}

int should_simulation_end(CargoPlane *plane, SharedState *shared_state) {
    if (plane->shared_state->collision_count > MAX_COLLISIONS_ALLOWED || plane->shared_state->container_destroyed > shot_down_threshold) {
        printf("Ending simulation due to: %s\n",
        plane->shared_state->collision_count > MAX_COLLISIONS_ALLOWED ? "collision limit exceeded" : "container destruction limit exceeded");
        return 1;
    }
    return 0;
}

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void setup_ipc(int *shm_id, int *sem_id, SharedState **shared_state) {
   key_t shm_key = ftok("shmfile", 65);
   key_t queue_key = ftok("plane.c", 'b'); 
    
    *shm_id = shmget(shm_key, sizeof(SharedState), 0666|IPC_CREAT);
    *shared_state = (SharedState *) shmat(*shm_id, (void*)0, 0);

    // Initialize shared state
    (*shared_state)->in_drop_zone = 0;
    (*shared_state)->planes_in_airspace= 0; // Make sure airspace is initially free
    (*shared_state)->collision_count = 0;
    (*shared_state)->container_destroyed = 0 ;
   
    

    *sem_id = semget(shm_key, 1, 0666|IPC_CREAT);
     semctl(*sem_id, 0, SETVAL, MAX_PLANES_IN_AIRSPACE);
     
     Container_queue_id = msgget(queue_key, 0666 | IPC_CREAT); 
    if (Container_queue_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    int var_memo_id = shmget(4343, sizeof(struct var), IPC_CREAT | 0666);
    if (var_memo_id < 0) {
        perror("shmgethihi");
        exit(1);
    }

    variables = (struct var *)shmat(var_memo_id, NULL, 0);
    if (variables == (void *) -1) {
        perror("shmat");
        exit(1);
    }
    
        // Initialize the variable counts
    variables->count_in_airspace = 0; // Ensure it's set to 0 at the start
    variables->count_exited_collision = 0;
    variables->count_exited_refill = 0;
    variables->container_count = 0 ;
    variables->planes_in_airspace = 0;

    // Initialize arrays to -1 or a similar invalid ID if necessary
    memset(variables->plane_ids_in_airspace, -1, sizeof(variables->plane_ids_in_airspace));
    memset(variables->plane_ids_exited_collision, -1, sizeof(variables->plane_ids_exited_collision));
    memset(variables->plane_ids_exited_refill, -1, sizeof(variables->plane_ids_exited_refill));

    if (sem_init(&sem, 1, 1) != 0) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }   

     printf("IPC setup complete. Airspace allows up to %d planes.\n", MAX_PLANES_IN_AIRSPACE);
    fflush(stdout);
}

void print_debug_info() {
    printf("Debug Info:\n");
    printf("Planes in Airspace: %d\n", variables->count_in_airspace);
    for (int i = 0; i < variables->count_in_airspace; i++) {
        printf(" - Plane ID in Airspace: %d\n", variables->plane_ids_in_airspace[i]);
    }
    printf("Planes Exited Due to Collision: %d\n", variables->count_exited_collision);
    for (int i = 0; i < variables->count_exited_collision; i++) {
        printf(" - Plane ID Exited Collision: %d\n", variables->plane_ids_exited_collision[i]);
    }
    printf("Planes Exited for Refill: %d\n", variables->count_exited_refill);
    for (int i = 0; i < variables->count_exited_refill; i++) {
        printf(" - Plane ID Exited for Refill: %d\n", variables->plane_ids_exited_refill[i]);
    }
}


void print_queue_contents(int queue_id) {
    Container msg;
    int result;
    printf("*********************************\n");
    printf("Queue Contents After Dropping Operations:\n");

    while ((result = msgrcv(queue_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT)) != -1) {
        const char *status_description = (msg.status == 1) ? "Intact" : "Partially Destroyed";
        printf("Container ID: %d, Weight: %d kg, Status: %s, Altitude at Drop: %d meters\n",
               msg.id, msg.weight, status_description, msg.altitude);
    }

    if (errno == ENOMSG) {
        printf("The queue is empty or all containers have been processed.\n");
    } else {
        perror("Failed to receive message");
    }
    printf("*********************************\n");
}

void cleanup_and_exit(int sig) {
    printf("Cleanup and exiting due to signal %d.\n", sig);
    if (shm_id > 0) { // Detach and remove shared memory
        shmdt(shared_state);
        shmctl(shm_id, IPC_RMID, NULL);
    }

    if (sem_id > 0) { // Remove semaphore
        semctl(sem_id, 0, IPC_RMID, NULL);
    }

    exit(0);
}


