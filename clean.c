#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
#include <sys/shm.h>

typedef struct {
    int in_drop_zone;
    int planes_in_airspace;
    int collision_count;
} SharedState;

int main() {
    // Test to collectors queue
    key_t key = ftok("test.c", 'b');
    int msg_queue_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    
    int store = msgget(2002, 0666 | IPC_CREAT);
    if (store == -1) {
        perror("msgget failed");
        exit(1);
    }
    
    // Splitter to Distributer queue
    int toDistributer = msgget(2005, 0666 | IPC_CREAT);
    if (toDistributer == -1) {
        perror("msgget failed");
        exit(1);
    }
    
    // Plane shared memory
    key_t shm_key = ftok("shmfile", 65);
    int shm_id = shmget(shm_key, sizeof(SharedState), 0666|IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach to shared memory
    SharedState* shared_memory = (SharedState*) shmat(shm_id, NULL, 0);
    if (shared_memory == (void*) -1) {
        perror("shmat failed");
        exit(1);
    }

    // Plane to collectors queue
    key_t queue_key = ftok("plane.c", 'b'); 
    int Container_queue_id = msgget(queue_key, 0666 | IPC_CREAT); 
    
    // Cleanup message queue at the end of the operation
    msgctl(msg_queue_id, IPC_RMID, NULL);
    msgctl(store, IPC_RMID, NULL);
    msgctl(toDistributer, IPC_RMID, NULL);
    msgctl(Container_queue_id, IPC_RMID, NULL);
    
    // Detach from shared memory
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed");
    }

    // Remove shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
    }
    
    printf("ALL ARE CLEAN\n");
    return 0;
}

