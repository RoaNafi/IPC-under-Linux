#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <semaphore.h>
#include <time.h>
#include <limits.h>
#include <sys/wait.h>   // for wait()

// Define role constants
#define COLLECTER 0
#define SPLITTER 1
#define DISTRIBUTER 2
#define GENERIC_MSG_TYPE 1
#define MAX_TEAM_MEMBERS 20
#define MAX_FAMILIES 50  
#define MAX_PLANES 20 
#define MAX_CONTAINERS_TOTAL 50


// helloooooo its must be update
#define BAGS_PER_TRIP 5  

typedef struct  {
    	int busy;
        pid_t pid;
        int role;
        int teamid;
        int num_of_collecters;
        double team_energy[MAX_TEAM_MEMBERS]; // enargy of all the collectors in the team	
        double energy;
        int is_alive;
        int status[MAX_TEAM_MEMBERS]; // 0: alive, 1: dead, 2: replaced by splitters
        int replacement_index; // Index of collector being replaced
        pid_t replacement_pid[MAX_TEAM_MEMBERS]; // PID of the splitter that replaces the collector
        int died_team;
    } Worker; 
 
// Shared memory structure
typedef struct {
    Worker workers[100];
} SharedState;


struct Family{
    int familyID;
    int starvationRate;
    int bagsReceived;
    int is_alive;
    int counted_as_dead;
};

typedef struct {
    long msg_type;  // Required by message queue, can be used to differentiate types of messages
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
struct Family *fam;

// Function prototypes
void Collect_containers(int msgid, int teamid);  // Updated to include teamid
void setup_ipc(SharedState **workers_shm, int *msgid, int *store, int *toDistributer);
void send_container_to_store(int store, const Container *container);
void Collector_energyAndDied();
void splitter_to_collector(int died_collector_index); 
void splitting_containers(int store, int toDistributer);
void distribute_bags(int toDistributer);
void cleanup_ipc();
void print_toDistributer_queue(int toDistributer);
void graceful_exit(int signum);
void distributer_EnergyAndDied();
 
int i;   //	i is index in workers shm 
pid_t my_pid;
int const_collector_per_team; 
int num_of_all;
int fam_shmID;
Container container;
Worker my_info;
SharedState *workers_shm;
sem_t sem;
sem_t worker_sem;

