#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <semaphore.h>
#include <time.h>
#include <GL/glut.h>
#include <math.h> 
#include <sys/time.h>


#define COLLECTER 0
#define SPLITTER 1
#define DISTRIBUTER 2
#define SHM_KEY 3000
#define window_width 1500
#define window_height 900
#define MAX_FAMILIES 30
#define MAX_TEAM_MEMBERS 20
#define MAX_PLANES 20  // Assuming a maximum number of planes your simulation can handle
#define MAX_CONTAINERS_TOTAL 50  // Adjust based on expected max containers

// Function prototypes
void sheared_memo_setup();
void setting_file(char *filename);
void fork_workers();
void cleanup_ipc();
void print_workers_shm();
void init();
void initOpenGL();
void display();
void reshape(int width, int height);
void drawCollectors();
void drawSplitters();
void drawDistributers();
void drawFamilies();
void handle_timeout(int sig);
void drawPlane();
void increase_starvation();
void handle_starvation_increase(int sig);
void set_periodic_timer();
void drawContainers();
 
int num_planes, containers_per_plane_min, containers_per_plane_max, drop_frequency_sec, refill_time_min, refill_time_max, num_collectors_teams, collecter_per_team, num_of_splitter, num_of_distributer, distributer_bags, min_distributer, app_period, death_threshold, damage_planes, max_containers, collec_workers_martyred, distrib_workers_martyred, family_death_threshold;
double min_death_rate;
int num_of_collectors;
int num_of_all_worker;
int num_of_all;
int workers_shmID;
sem_t sem;
int planes_in_airspace;

struct Worker {
    int busy;
    pid_t pid;
    int role;
    int teamid;
    int num_of_collecters;
    double team_energy[20]; 
    double energy;
    int is_alive;
    int status[MAX_TEAM_MEMBERS]; // 0: alive, 1: dead, 2: replaced by splitter
    int replacement_index; // Index of collector being replaced    
    pid_t replacement_pid[MAX_TEAM_MEMBERS]; // PID of the splitter that replaces the collector
    int died_team;
};

struct Worker *everyworkers;

struct Family {
    int familyID;
    int starvationRate;
    int bagsReceived;
    int is_alive;
    int counted_as_dead;
};

struct Family *fam; 

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

int simulation_time;
volatile sig_atomic_t keepRunning = 1;  // Flag to control the running of the loop roro
