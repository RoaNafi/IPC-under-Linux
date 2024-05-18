#include "workers.h"

void handle_sigterm(int sig) {
   
    printf("Worker %d terminating.\n", getpid());
    fflush(stdout);
    exit(0);  // Exit the process
}

int main(int argc, char *argv[]) {

	sleep(1 + rand() % 4); //  wait for either 1, 2, or 3 seconds
	
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments\n");
        return EXIT_FAILURE;
    }
    
    
    
   
    int role = atoi(argv[1]);
    int msgid, store, toDistributer;
	int found = 0;
	int teamid;
	num_of_all = atoi(argv[2]);
	my_pid = getpid();
	
    // Only Collectors will have a second argument for team ID
    if (role == COLLECTER && argc >= 3) {
        teamid = atoi(argv[3]);
    }
 	
 	
 	
 	setup_ipc(&workers_shm,&msgid, &store, &toDistributer); 
   
   	
	
    //printf("Number of distributers: %d\n", variables->num_of_distributer);
    //printf("Minimum number of distributers: %d\n", variables->min_distributer);
    //printf("Number of splitters: %d\n", variables->num_of_splitter);

    
    // Scan workers shared memory to read my info
    //	i is index in w shm dont use i to something else 
    for ( i = 0; i < num_of_all; i++) {
		if (workers_shm->workers[i].pid == my_pid) {
		      my_info = workers_shm->workers[i];
		      found = 1;
		      break;
		   }
    }

    if (!found) {
      	 printf("Worker PID %d not found in shared memory. Checking again...\n", my_pid);
         fflush(stdout);
         sleep(1);  // Wait a bit before checking again
         
        }
    
    const_collector_per_team = my_info.num_of_collecters;
   
    //	the worker keeps working
	while(1){
	
			my_info = workers_shm->workers[i];	//to keep read the role
			
			switch (my_info.role) {
				case COLLECTER:
				
					signal(SIGTERM, handle_sigterm); //roro with cleanup
					
					if (my_info.num_of_collecters > 0){
						/*
						//	print info
			   			printf("Team %d ,busy: %d ,collecters = %d ,Enargy: ", my_info.teamid , my_info.busy , my_info.num_of_collecters);
				        fflush(stdout);
				        for (int j = 0; j < const_collector_per_team; j++) {
				   			printf("%.2f, ",my_info.team_energy[j]);
				   			fflush(stdout);
			   			}     
			   			printf("\n");
				   		fflush(stdout);
						*/
						
						//	Collector process 	
						Collect_containers(msgid,teamid);  
						send_container_to_store(store,&container);
						Collector_energyAndDied();
					
					}else{
					
						printf("All collectors in team %d are did\n",my_info.teamid);
						fflush(stdout);
						return EXIT_SUCCESS;
					}
					break;
				case SPLITTER: 
						
					 if(workers_shm->workers[i].is_alive){
					 	 
						 signal(SIGTERM, graceful_exit); //roro  without cleanup
						 splitting_containers(store, toDistributer);
						 sleep(1 + rand() % 4); //  wait for either 1, 2, or 3 seconds
					 
					 }	
					 
					  
				    break;
				case DISTRIBUTER:
					
					signal(SIGTERM, handle_sigterm);
					
					if(workers_shm->workers[i].is_alive){
						
						
						distribute_bags(toDistributer);
						distributer_EnergyAndDied();
						
    					
					
					}else{
						//printf("distributer %d is did\n",my_info.pid);
						return EXIT_SUCCESS;
					}
					
				    break;
				default:
				    sleep(1);  // Delay before rechecking role
				    break;
			}
		}
		
    return EXIT_SUCCESS;
}

void Collect_containers(int msgid, int teamid) {
    while (1) {
        if (msgrcv(msgid, &container, sizeof(Container) - sizeof(long), GENERIC_MSG_TYPE, 0) == -1) {
            if (errno == ENOMSG) {
                printf("No more messages for Team %d Collector %d.\n", teamid, my_pid);
                fflush(stdout); 
                break;
            }
            perror("msgrcv failed");
            break;  // Exit on unrecoverable error
        } else {
            
            printf("Team %d collected Container ID %d ,with Energy: ", teamid, container.id);
            fflush(stdout);
            
             sem_wait(&sem);
             
              for (int i = 0; i < variables->container_count; i++) {
        if (variables->containers[i].id == container.id) {
            // Container found, update location
            variables->containers[i].location = 1;  // 1 indicates the container is with the collector
            break;  // Break the loop once the container is found and updated
        }
    }

        // Unlock the semaphore after making changes
        sem_post(&sem);
            
            for (int j = 0; j < const_collector_per_team; j++) {
				printf("%.2f, ",my_info.team_energy[j]);
				fflush(stdout);
			}     
	    printf("\n");
	    fflush(stdout);
            workers_shm->workers[i].busy = 1;
            sleep(1 + rand() % 4); //   1, 2, or 3 seconds needs to collect container
            
            break; 
        }
    }
}

void send_container_to_store(int store, const Container *container) {
    Container new_container = *container;
    new_container.msg_type = GENERIC_MSG_TYPE;  
    if (msgsnd(store, &new_container, sizeof(Container) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        
    }
    
    printf("Team %d sent Container %d to STORE\n",my_info.teamid ,container->id);
    fflush(stdout);
    sleep(1 + rand() % 4); // 1, 2, or 3 seconds needs to send containers to store 
     
    workers_shm->workers[i].busy = 0;
    
}


void Collector_energyAndDied() {
    srand(time(NULL));  // Seed the random generator (consider doing this only once outside of this function if called frequently)
	
    for (int j = 0; j < const_collector_per_team ; j++) {
    
        int threshold = 2 + rand() % 9;  // Random threshold between 2 and 10 
        int energy_loss = 4 + rand() % 15;  // Random energy loss between 1 and 5 
		
		//int energy_loss = 10; 
		my_info.team_energy[j] -= energy_loss;
        if (my_info.team_energy[j] < 0) my_info.team_energy[j] = 0;  // Ensure energy does not go negative
        
         
        // Write updated energy levels back to shared memory
		workers_shm->workers[i].team_energy[j] = my_info.team_energy[j];
			
			
        if (my_info.team_energy[j] < threshold ) {
        
            printf("Collector %d in Team %d has died.\n", j, my_info.teamid);
            fflush(stdout);
            workers_shm->workers[i].status[j] = 1; //died
            
            
            variables->martyred_collectors++;//roro
            //printf("+++++++++++++++ variables->martyred_collectors = %d\n",variables->martyred_collectors );
             fflush(stdout);
            sem_wait(&worker_sem);  
			workers_shm->workers[i].num_of_collecters = workers_shm->workers[i].num_of_collecters - 1;	
			sem_post(&worker_sem); // Unlock the semaphore  
				
            //to make sure that splitter not = 0
			int flag = variables->num_of_splitter - 1; 
			
			if(flag <= 4){
					printf("There is four splitter left and cannot be taken out. col\n");
					
            }else{
		       	usleep(2000);
		       	sem_wait(&worker_sem); 
		       	variables->num_of_splitter--;
		        sem_post(&worker_sem); // Unlock the semaphore 
		        splitter_to_collector(j);
		        
            }
            //printf("+++++++++ workers_shm->workers[i].status = %d \n",workers_shm->workers[i].status);
        } 
    }
}

// when collector did
void splitter_to_collector(int died_collector_index) {
	
		
    for (int k = 0; k < num_of_all; k++) {
        if ((workers_shm->workers[k].role == SPLITTER) & (workers_shm->workers[k].is_alive == 1)) {
        
        	
        	sem_wait(&worker_sem);
        	
        	
             workers_shm->workers[k].is_alive = 0;
             workers_shm->workers[i].team_energy[died_collector_index] = workers_shm->workers[k].energy;
             workers_shm->workers[i].status[died_collector_index] = 2; // Mark as replaced by splitter
            
            workers_shm->workers[i].replacement_index = died_collector_index;
            workers_shm->workers[i].replacement_pid[died_collector_index] = workers_shm->workers[k].pid;
         
            kill(workers_shm->workers[k].pid, SIGTERM);
            usleep(20000);
            
          	workers_shm->workers[i].num_of_collecters = workers_shm->workers[i].num_of_collecters + 1;	
          	
            printf("Splitter %d with pid %d come insted of collictor in team %d\n", k,workers_shm->workers[k].pid , workers_shm->workers[i].teamid);
            fflush(stdout);
            
            //variables->collector_replacements++; // Increment the replacement counter
            variables->splitter_to_collector_updates = 1; // Signal an update
             
			sem_post(&worker_sem); // Unlock the semaphore   	           
           	
           	usleep(20000);
           	
            break;  // Stop after modifying the first splitter found
        }
        	
    }
    
   
    
}


void distributer_EnergyAndDied(){
		
	//	decrease Energy
	int threshold = 2 + rand() % 9;  // Random threshold between 2 and 10 
        int energy_loss = 3 + rand() % 20;  // Random energy loss between 1 and 5 
	//int energy_loss = 10;
		
		
		my_info.energy -= energy_loss;
        if (my_info.energy  < 0) my_info.energy  = 0;  // Ensure energy does not go negative
        
         
        // Write updated energy levels back to shared memory
		workers_shm->workers[i].energy = my_info.energy;
		
		if(my_info.energy < threshold){
			// so distributer did 
			workers_shm->workers[i].is_alive = 0; 
			printf("Distributor %d has died ,Energy = %.2f\n", my_info.pid,my_info.energy);
        	        fflush(stdout);
        	        workers_shm->workers[i].status[0] = 1;

            
			sem_wait(&worker_sem);
			variables->num_of_distributer--;
	
			// to Check if we need to employ some splitter as distributer 
			if(variables->num_of_distributer <= variables->min_distributer){
				
				int splitter_needed = variables->num_of_splitter / 5;
				if (splitter_needed  < 1) splitter_needed  = 1;  // Ensure splitter_needed not less than 1 
				
				//to make sure that splitter not = 0
				int flag = variables->num_of_splitter -  splitter_needed;
				if(flag <= 4){
					printf("There is four splitter left and cannot be taken out\n");
					
				} else {
					printf("splitter to distributer\n");
					variables->num_of_splitter = variables->num_of_splitter -  splitter_needed;
				
					//printf("splitter_needed = %d\n",splitter_needed);
					
					for(int m = 0; m < splitter_needed; m++){
						variables->splitter_to_distributer_updates =1;
						//search for some alive splitter 
						for (int k = 0; k < num_of_all; k++) {
							if ((workers_shm->workers[k].role == SPLITTER) & (workers_shm->workers[k].is_alive == 1)){
									
									
									workers_shm->workers[k].role = DISTRIBUTER;
									workers_shm->workers[k].status[0] = 3;
									printf("Splitter %d, %d become distributer\n" ,k,workers_shm->workers[k].pid);
									fflush(stdout);
									variables->num_of_distributer++;
									break;
							} 
									
						}
					}

					
				}
			}
			sem_post(&worker_sem);
		}else{
			//if not died
			printf("Distributer %d back from trip with Energy %.2f\n", my_info.pid, my_info.energy);
			fflush(stdout);
		
		}
}		

void splitting_containers(int store, int toDistributer) {
    Container container;
    
    while (1) {
        if (workers_shm->workers[i].busy) {
            sleep(1);  // Simple back-off strategy
            continue;
        }

        workers_shm->workers[i].busy = 1;

        if (msgrcv(store, &container, sizeof(Container) - sizeof(long), GENERIC_MSG_TYPE, IPC_NOWAIT) != -1) {
            printf("Splitter %d processing container %d with weight %d kg\n", my_pid, container.id, container.weight);
            fflush(stdout);
            
                        sem_wait(&sem);
             
              for (int i = 0; i < variables->container_count; i++) {
        if (variables->containers[i].id == container.id) {
            // Container found, update location
            variables->containers[i].location = 2;  // 1 indicates the container is with the collector
            
            break;  // Break the loop once the container is found and updated
        }
    }

        // Unlock the semaphore after making changes
        sem_post(&sem);

            Container bag;
            for (int j = 0; j < container.weight; j++) {
                bag.msg_type = GENERIC_MSG_TYPE;
                bag.id = container.id;  // Ensure each bag has the correct container ID
                bag.weight = 1;
                bag.status = 1;
                bag.altitude = 0;
				 sleep(1 + rand() % 4); // 1, 2, or 3 seconds needs to send 1 bag to distributer 
                if (msgsnd(toDistributer, &bag, sizeof(Container) - sizeof(long), IPC_NOWAIT) == -1) {
                    perror("Failed to send 1-kg bag to distributer");
                    continue;
                }

                //printf("Splitter %d sent 1-kg bag from container %d to distribution\n", my_pid, container.id);
                //fflush(stdout);
            }
            
            workers_shm->workers[i].busy = 0;
            //sleep(1 + rand() % 4); // wait for 1, 2, or 3 seconds bafor split new container
            break;
            
        } else {
            if (errno != ENOMSG && errno != EAGAIN) {
                perror("Failed to receive container");
            }
            workers_shm->workers[i].busy = 0;
        }
        
    }
    printf("Splitter %d finish processing container %d with weight %d kg\n", my_pid, container.id, container.weight);
    fflush(stdout);
}


void distribute_bags(int toDistributer) {
    Container bag;
    int count = 0;
    Container bags[BAGS_PER_TRIP]; // To store bags temporarily
    int attempts = 0;
    usleep(2000);

    // Attempt to receive BAGS_PER_TRIP bags, with a maximum of 1500 attempts (approx 15 seconds with a sleep of 10ms between attempts)
    while (count < BAGS_PER_TRIP && attempts < 2000) {
        if (msgrcv(toDistributer, &bag, sizeof(Container) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            bags[count++] = bag; // Store the bag received
        } else {
            if (errno == ENOMSG) {
                // No message available right now, wait a bit before retrying
                usleep(10000); // Wait 10 milliseconds
            } else {
                perror("Failed to receive bag for distribution");
                break; 
            }
        }
        attempts++;
    }

    // Distribute the bags collected so far
    printf("Distributer %d start trip with Energy %.2f\n", my_info.pid, my_info.energy);
    
            sem_wait(&sem);
             
              for (int i = 0; i < variables->container_count; i++) {
        if (variables->containers[i].id == container.id) {
            // Container found, update location
            variables->containers[i].location = 3;  // 1 indicates the container is with the collector
            
            break;  // Break the loop once the container is found and updated
        }
    }

        // Unlock the semaphore after making changes
        sem_post(&sem);
    for (int i = 0; i < count; i++) {
        sem_wait(&sem); // Lock the semaphore to protect shared memory access
        int maxNeedIndex = -1;
        int maxNeed = INT_MIN;

        // Find the family with the highest need
        for (int j = 0; j < MAX_FAMILIES; j++) {
            if (fam[j].starvationRate > maxNeed && fam[j].is_alive) {
                maxNeed = fam[j].starvationRate;
                maxNeedIndex = j;
            }
        }

        if (maxNeedIndex != -1) {
           if(fam[maxNeedIndex].is_alive){
            // Distribute the bag to the family
            fam[maxNeedIndex].bagsReceived++;
            fam[maxNeedIndex].starvationRate -= ((rand() % 7) + 1); // Assuming each bag reduces the starvation rate by 1 to 7
	    if (fam[maxNeedIndex].starvationRate < 0){
	    fam[maxNeedIndex].starvationRate = 0;
	    }
            printf("Distributer %d delivered a bag from container %d to family %d. Total bags received: %d, Starvation rate now: %d\n",
                   my_pid, bags[i].id, fam[maxNeedIndex].familyID, fam[maxNeedIndex].bagsReceived, fam[maxNeedIndex].starvationRate);
            fflush(stdout);
            
        }
        }
        sem_post(&sem); 
    }
    
    sleep(1 + rand() % 4); // trip time is 1, 2, or 3 seconds 
   
    
}


void setup_ipc(SharedState **workers_shm, int *msgid, int *store, int *toDistributer) { 
   
    //	############################ SHEADR MOMORY SETUP ############################
    int workers_memo_key = 5200;
    int shmid = shmget(workers_memo_key, sizeof(Worker)*100 , 0666 | IPC_CREAT);
    
    if (shmid == -1) {
        perror("workers_memo shmget failed");
        exit(1);
    }

   *workers_shm = (SharedState *)shmat(shmid, NULL, 0);
    if (*workers_shm == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    
    int fam_memo_key = 2990;
	fam_shmID = shmget (fam_memo_key, sizeof(struct Family)*100 , 0666 | IPC_CREAT);
	
	if (fam_shmID < 0) {
		printf("Failed to create shm worker fam\n");
		exit(1);
	}
	
	fam = (struct Family *)shmat(fam_shmID, NULL, 0); 
	
	//key_t var_key = ftok("shmfile", 80);
    int var_memo_id = shmget(4343, sizeof(struct var), IPC_CREAT | 0666);
    if (var_memo_id < 0) {
        perror("worker var_memo shmget failed");
        exit(1);
    }

    variables = (struct var *)shmat(var_memo_id, NULL, 0);
    if (variables == (void *) -1) {
        perror("worker variables shmat");
        exit(1);
    }
	
	//	################################	QUEUE SETUP 	############################
	
    key_t key = ftok("plane.c", 'b');
    *msgid = msgget(key, 0666 | IPC_CREAT);
    if (*msgid == -1) {
        perror("msgget failed");
        exit(1);
    }
    
    *store = msgget(2002, 0666 | IPC_CREAT);
    if (*store == -1) {
        perror("msgget failed");
        exit(1);
    }

    *toDistributer = msgget(2005, 0666 | IPC_CREAT);  // Create a new message queue for distribution
    if (*toDistributer == -1) {
        perror("msgget failed");
        exit(1);
    }
    //	############################ SEMAPHORE SETUP ############################
     
    if (sem_init(&worker_sem, 1, 1) != 0) { 
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
  
    if (sem_init(&sem, 1, 1) != 0) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }   
    //	############################ FAMILIES ############################
    
}

void cleanup_ipc() {

}

void graceful_exit(int signum) {
   	// signal to kill splitter 
    printf("Handled SIGTERM, exiting gracefully.\n");
    exit(0);
}

//	JUST print queue
void print_toDistributer_queue(int toDistributer) {
    Container received_container;
    printf("Contents of toDistributer queue:\n");
    
    // Attempt to receive all messages in the queue
    while (msgrcv(toDistributer, &received_container, sizeof(Container) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        printf("Container ID: %d, Weight: %d kg, Status: %d, Altitude: %d\n",
            received_container.id, received_container.weight, received_container.status, received_container.altitude);
    }

    if (errno != ENOMSG) {
        perror("Failed to receive from toDistributer queue");
    }
}



