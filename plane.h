#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

// Constants for simulation
#define MAX_CONTAINERS 5
#define MIN_DROP_INTERVAL 1  // in seconds
#define MAX_DROP_INTERVAL 5  // in seconds
#define REFILL_TIME 10       // in seconds
#define MAX_PLANES_IN_AIRSPACE 3
#define COLLISION_PROBABILITY 10 
#define INITIAL_ALTITUDE 700  // altitude in meters
#define HIT_PROBABILITY_PER_SECOND 0.1  // probability of being hit per second
#define HIGH_ALTITUDE_THRESHOLD 9000
#define MEDIUM_ALTITUDE_THRESHOLD 6000
#define MAX_COLLISIONS_ALLOWED 3 // Define maximum allowed collisions
#define COLLISION_DISTANCE 100  // Distance in simulation units where a collision is detected
#define MAX_PLANES 20  // Assuming a maximum number of planes your simulation can handle
#define MAX_CONTAINERS_TOTAL 50  // Adjust based on expected max containers


typedef struct {
    int in_drop_zone;
    int planes_in_airspace;
    int collision_count;
    int container_destroyed;
} SharedState;

// Structure for a cargo plane
typedef struct {
    int id;
    int container_count;
    int drop_interval;  // Time between drops
    int is_refilling;   // 0 = not refilling, 1 = refilling
    int sem_id;
    SharedState *shared_state;
} CargoPlane;



// Structure for a container
typedef struct {
    long msg_type;  // Required by message queue, can be used to differentiate types of messages
    int index;
    int id;         // Container ID
    int weight;     // Weight of the container in kilograms
    int status;     // 0 = destroyed, 1 = intact, 2 = Incompletely destroyed
    int altitude;   // height
    int location;
} Container;

struct var {
    int num_of_distributer;
    int min_distributer;
    int num_of_splitter;
    int splitter_to_distributer_updates; // To track how many collectors have been replaced
    int splitter_to_collector_updates; // To signal updates to the main.c
    int martyred_collectors; //roro
    int planes_in_airspace;          // Number of planes currently in the airspace
    int plane_ids_in_airspace[MAX_PLANES];
    int plane_ids_exited_collision[MAX_PLANES];
    int plane_ids_exited_refill[MAX_PLANES];
    int count_in_airspace;
    int count_exited_collision;
    int count_exited_refill;
    Container containers[MAX_CONTAINERS_TOTAL]; // Array to hold container data
    int container_count; // Counter for active containers
};


struct var *variables;


// Function prototypes
void initialize_planes(CargoPlane *planes, int num_planes, int sem_id, SharedState *shared_state);
void entere_airspace(CargoPlane *plane);
void refill_plane(CargoPlane *plane);
void simulate_plane_activity(CargoPlane *plane);
int rand_range(int min, int max);
void setup_ipc(int *shm_id, int *sem_id, SharedState **shared_state);
void perform_drop_operations(CargoPlane *plane, int Container_queue_id);
void print_queue_contents(int queue_id);
int check_collision(int planes_in_airspace);
void initialize_container(Container *container, int plane_id, int container_index);
void monitor_status(SharedState *shared_state) ;
int should_simulation_end(CargoPlane *plane,SharedState *shared_state);
void cleanup_and_exit(int sig) ;
void print_debug_info() ;

int num_planes = 5;
int Container_queue_id;
int collision_threshold = 2;
int shot_down_threshold = 10;
SharedState *shared_state;

// Global identifiers
int shm_id, sem_id;
int Container_queue_id;
sem_t sem;

