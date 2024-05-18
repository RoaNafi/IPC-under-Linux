#include "main.h"

int main(int argc, char *argv[]) {
    setting_file("file.txt");  // Ensure all config is read before forking
    num_of_collectors = num_collectors_teams * collecter_per_team;  
    num_of_all_worker = num_of_distributer + num_of_collectors + num_of_splitter;
    num_of_all = num_of_distributer + num_collectors_teams + num_of_splitter;

	signal(SIGALRM, handle_timeout);
	alarm(simulation_time);
    
    sheared_memo_setup();
    variables->num_of_distributer = num_of_distributer;
    variables->min_distributer = min_distributer;
    variables->num_of_splitter = num_of_splitter;
    variables->planes_in_airspace = planes_in_airspace;
    variables->martyred_collectors = 0; // roro
    pid_t pid = fork();
    if (pid == 0) {
    
    	
        // Child process for OpenGL rendering
        initOpenGL(&argc, argv);
        glutMainLoop();
        exit(0);
        
    } else if (pid > 0) {
        // Parent process continues with the rest of the application
        fork_workers();
        usleep(20000);
        print_workers_shm();
        
        int total_deaths = 0; // Counter for the number of dead families
        //int last_checked_time = 0; // To control when to check the death rate
	time_t last_update_time = time(NULL);
	set_periodic_timer();
        int flag1;
        while (keepRunning){
                time_t current_time = time(NULL);
		// Check if 5 seconds have passed to update starvation
		
		if (current_time - last_update_time >= 5) {   
		    usleep(200000);
		    increase_starvation();
		    last_update_time = current_time;  // Reset the last update time
		}
          int currently_dead_families = 0;
         for (int i = 0; i < MAX_FAMILIES; i++) {
           if (!fam[i].is_alive && !fam[i].counted_as_dead) {
            fam[i].counted_as_dead = 1;
            total_deaths++;
            }
           if (!fam[i].is_alive) {
            currently_dead_families++;
         }
    }
    
     if (total_deaths > death_threshold) {
        printf("Terminating simulation: too many families have died.\n");
        keepRunning = 0;
    }
    /*
     if (time(NULL) - last_checked_time >= 5) { // Check every 5 seconds
        double death_rate = (double)currently_dead_families / MAX_FAMILIES;
        if (death_rate < min_death_rate) {
            printf("Terminating simulation: death rate below threshold.\n");
            keepRunning = 0;
        }
        last_checked_time = time(NULL);
    }
    */

        	
        	flag1 = (variables->martyred_collectors >= collec_workers_martyred) ? 1 : 0;

        	if(flag1){
        		
        		keepRunning = 0 ;
        	
        	}

        
        }// Wait for all child processes to finish
		
		 // Terminate all workers
		for(int i = 0; i < num_of_all; i++) {
		    kill(everyworkers[i].pid, SIGTERM);
		}
    
       	cleanup_ipc();
        printf("Simulation completed successfully.\n");
        fflush(stdout);
        
    } else {
        perror("Failed to fork");
        exit(1);
    }
    return EXIT_SUCCESS;
}

void handle_timeout(int sig) {
    if (sig == SIGALRM) {
        printf("Simulation time over. Preparing to exit...\n");
        keepRunning = 0;  // Set the flag to false to break the loop
    }
}

void fork_workers() {
    pid_t pid;
  	int worker_index = 0; // Index to keep track of the worker in the shared memory
  	 char role_str[10], teamid_str[10], num_of_all_str[10];

    sprintf(num_of_all_str, "%d", num_of_all); // Convert num_of_all to string
    
    	
    for (int i = 1; i <= num_of_splitter; i++){
    	pid = fork();
    	if (pid == 0) {
    	    sem_wait(&sem); // Lock the semaphore
    	    everyworkers[worker_index].pid = getpid();
            everyworkers[worker_index].role = SPLITTER;
            everyworkers[worker_index].teamid = -1;  
            everyworkers[worker_index].busy = 0;
            everyworkers[worker_index].is_alive = 1;

            
            srand(time(NULL) ^ (getpid() << 16));
            everyworkers[worker_index].energy = 100 + rand() % (150 - 100 + 1);
            
            sem_post(&sem); // Unlock the semaphore
            sprintf(role_str, "%d", SPLITTER);
            execl("./workers", "workers", role_str,num_of_all_str, NULL);
            perror("execl"); // This will only be executed if execl fails
            exit(1);
    	}
    	else if (pid < 0) {
		    perror("Failed to fork splitter");
		    exit(1);
    	}
    	worker_index++;
    }
    
    for (int i = 1; i <= num_of_distributer; i++){
    	pid = fork();
    	if (pid == 0) {
    	    sem_wait(&sem); // Lock the semaphore
  	    	everyworkers[worker_index].pid = getpid();
            everyworkers[worker_index].role = DISTRIBUTER;
            everyworkers[worker_index].teamid = -1; 
            everyworkers[worker_index].busy = 0;
            everyworkers[worker_index].is_alive = 1;
            
            srand(time(NULL) ^ (getpid() << 16));
            everyworkers[worker_index].energy = 100 + rand() % (150 - 100 + 1);
            
            sem_post(&sem); // Unlock the semaphore
            sprintf(role_str, "%d", DISTRIBUTER);
            execl("./workers", "workers", role_str,num_of_all_str, NULL);
            perror("execl"); // This will only be executed if execl fails
            exit(1);
    	}
    	else if (pid < 0) {
		    perror("Failed to fork distributer");
		    fflush(stdout);
		    exit(1);
    	}
    	worker_index++;
    }
    int team_id = 1;
    for (int i = 1; i <= num_collectors_teams ; i++){
		pid = fork();
		if (pid == 0) {
				sem_wait(&sem); // Lock the semaphore
				everyworkers[worker_index].pid = getpid();
                everyworkers[worker_index].role = COLLECTER;
                everyworkers[worker_index].teamid = i;
                everyworkers[worker_index].busy = 0;
                everyworkers[worker_index].num_of_collecters = collecter_per_team;
                everyworkers[worker_index].is_alive = 1;
                
                
				srand(time(NULL) ^ (getpid() << 16));
				for (int j = 0; j < collecter_per_team; j++) {
					everyworkers[worker_index].team_energy[j] = 100 + rand() % (150 - 100 + 1);
					everyworkers[worker_index].status[j] = 0;
				}		  
				  
                sem_post(&sem); // Unlock the semaphore
                sprintf(role_str, "%d", COLLECTER);
                sprintf(teamid_str, "%d", team_id); 
                execl("./workers", "workers", role_str, num_of_all_str,teamid_str, NULL);
                perror("execl"); // This will only be executed if execl fails
                exit(1);
			}
			else if (pid < 0) {
				perror("Failed to fork collector");
				exit(1);
			}
			 worker_index++;
		
		team_id++;
    }
    
}

void sheared_memo_setup(){

	int workers_memo_key = 5200;
	workers_shmID = shmget (workers_memo_key, sizeof(struct Worker)*100 , 0666 | IPC_CREAT);
	
	if (workers_shmID < 0) {
		printf("Failed to create shm worker main \n");
		exit(1);
	}
	
	everyworkers = (struct Worker *)shmat(workers_shmID, NULL, 0);  
	
	if (everyworkers == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }
    // Zero out the memory to ensure all initial values are set to zero
    memset(everyworkers, 0, sizeof(struct Worker) * 100);
	 // Initialize the semaphore for binary access
    if (sem_init(&sem, 1, 1) != 0) { // 1 - shared between processes, initial value 1
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

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
	
    memset(variables, 0, sizeof(struct var)); // Also zero out the variables segment
	//printf("Number of planes set to %d\n", variables->num_of_planes);
    
    int fam_memo_key = 2990;
    int fam_shmID = shmget(fam_memo_key, sizeof(struct Family) * MAX_FAMILIES, 0666);
    if (fam_shmID < 0) {
        perror("Failed to access family shared memory");
        exit(1);
    }

    fam = (struct Family *) shmat(fam_shmID, NULL, 0);
    if (fam == (void *) -1) {
        perror("shmat for families failed");
        exit(1);
    }
    //memset(*workers_shm, 0, sizeof(SharedState));
    memset(fam, 0, sizeof(struct Family) * MAX_FAMILIES);
    
    //roro
    for (int i = 0; i < MAX_FAMILIES; i++) {
        fam[i].familyID = i;
        fam[i].starvationRate = rand() % 50 + 1;
        fam[i].bagsReceived = 0;
        fam[i].is_alive = 1;
    }  
    
}
void setting_file(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "num_planes=%d", &num_planes);
        sscanf(line, "containers_per_plane_min=%d", &containers_per_plane_min);
        sscanf(line, "containers_per_plane_max=%d", &containers_per_plane_max);
        sscanf(line, "drop_frequency_sec=%d", &drop_frequency_sec);
        sscanf(line, "refill_time_min=%d", &refill_time_min);
        sscanf(line, "refill_time_max=%d", &refill_time_max);
        sscanf(line, "num_collectors_teams=%d", &num_collectors_teams);
        sscanf(line, "collecter_per_team=%d", &collecter_per_team);
        sscanf(line, "num_of_splitter=%d", &num_of_splitter);
        sscanf(line, "num_of_distributer=%d", &num_of_distributer);
        sscanf(line, "distributer_bags=%d", &distributer_bags);
        sscanf(line, "min_distributer=%d", &min_distributer);
        sscanf(line, "app_period=%d", &app_period);
        sscanf(line, "death_threshold=%d", &death_threshold);
        sscanf(line, "damage_planes=%d", &damage_planes);
        sscanf(line, "max_containers=%d", &max_containers);
        sscanf(line, "collec_workers_martyred=%d", &collec_workers_martyred);
        sscanf(line, "distrib_workers_martyred=%d", &distrib_workers_martyred);
        sscanf(line, "family_death_threshold=%d", &family_death_threshold);
        sscanf(line, "simulation_time=%d", &simulation_time);
        sscanf(line, "min_death_rate=%lf", &min_death_rate);
    }
    fclose(file);
}

void increase_starvation() {
    for (int i = 0; i < MAX_FAMILIES; i++) {
        sem_wait(&sem); // Ensure exclusive access to the family data
        if (fam[i].is_alive) {
            fam[i].starvationRate += 1 + rand() % 6;  // Increase starvation rate over time
            if (fam[i].starvationRate > 130) { // Threshold for survival, could be calibrated
                fam[i].is_alive = 0;
                printf("family died!!! %d\n",fam[i].familyID);
            }
        }
        sem_post(&sem);
    }
}


void cleanup_ipc() {

	//	############################ SHEADR MOMORY CLEANUP ############################
	
	//	############################ QUEUE CLEANUP ############################
	
	//	############################ SEMAPHORE CLEAN ############################
   
}

void set_periodic_timer() {
    struct itimerval timer;
    timer.it_value.tv_sec = 5;  // Initial delay in seconds for the first call
    timer.it_value.tv_usec = 0;  // Initial microseconds (not used here)
    timer.it_interval.tv_sec = 5;  // Interval for subsequent calls
    timer.it_interval.tv_usec = 0;  // Interval microseconds (not used here)
    
    setitimer(ITIMER_REAL, &timer, NULL);
    signal(SIGALRM, handle_starvation_increase);
}

void handle_starvation_increase(int sig) {
    increase_starvation();

    printf("Starvation levels increased.\n");
}


void print_workers_shm(){

	printf("\nworkers sheard memory\n");
	for (int i = 0; i < num_of_all; i++) {
        
            printf("Worker PID: %d, Role: ", everyworkers[i].pid);
           
            fflush(stdout);
            
            switch (everyworkers[i].role) {
                case COLLECTER:
                    printf("Collector ,Team ID: %d ,busy: %d ,alive: %d ,collecters = %d ,", everyworkers[i].teamid,everyworkers[i].busy,everyworkers[i].is_alive , everyworkers[i].num_of_collecters);
                    fflush(stdout);
                    printf("Enargy: [");
                     for (int j = 0; j < collecter_per_team; j++) {
           				printf("%.1f, ",everyworkers[i].team_energy[j]);
       				 }
                    printf("]\n");
                    
                    break;
                case SPLITTER:
                    printf("Splitter, busy: %d ,alive: %d , Enargy: %.1f\n",everyworkers[i].busy,everyworkers[i].is_alive , everyworkers[i].energy);
                    fflush(stdout);
                    break;
                case DISTRIBUTER:
                    printf("Distributer , busy: %d ,alive: %d , Enargy: %.1f\n",everyworkers[i].busy,everyworkers[i].is_alive ,everyworkers[i].energy);
					fflush(stdout);                    
					break;
                default:
                    printf("Unknown\n");
                    break;
            }
    }
    printf("\n_________________________________________________________________\n\n");




}


void initOpenGL(int *argc, char **argv) {
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Worker Visualization");

    // OpenGL initialization function
    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
}
void renderBitmapString(float x, float y, void *font, const char *string) {
    const char *c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}


void drawCloud(float x, float y, float width, float height) {
    int num_segments = 20; // Increase the segments for a smoother circle
    float angle_step = 2.0f * M_PI / num_segments; // Calculating the angle step based on the number of segments

    // Primary cloud body with three main circles
    glColor3f(1.0f, 1.0f, 1.0f); // Set color to white
    float radius_x = width * 0.3f; // Radius for the width of the circles
    float radius_y = height * 0.3f; // Radius for the height of the circles

    // Central circles for bulk of the cloud
    for (float nx = x - width * 0.3f; nx <= x + width * 0.3f; nx += width * 0.3f) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(nx, y); // Center of circle
        for (int i = 0; i <= num_segments; i++) {
            float angle = i * angle_step;
            glVertex2f(nx + cos(angle) * radius_x, y + sin(angle) * radius_y);
        }
        glEnd();
    }

    // Smaller circles on the sides for a fluffier look
    float side_offset_x = width * 0.5f; // Horizontal offset for side circles
    float side_offset_y = height * 0.1f; // Vertical offset for side circles

    // Left side circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x - side_offset_x, y - side_offset_y); // Center of left side circle
    for (int i = 0; i <= num_segments; i++) {
        float angle = i * angle_step;
        glVertex2f(x - side_offset_x + cos(angle) * (radius_x * 0.7f), y - side_offset_y + sin(angle) * (radius_y * 0.7f));
    }
    glEnd();

    // Right side circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x + side_offset_x, y - side_offset_y); // Center of right side circle
    for (int i = 0; i <= num_segments; i++) {
        float angle = i * angle_step;
        glVertex2f(x + side_offset_x + cos(angle) * (radius_x * 0.7f), y - side_offset_y + sin(angle) * (radius_y * 0.7f));
    }
    glEnd();
}

void drawClouds() {

    drawCloud(window_width * 0.1f, window_height * 0.8f, 80, 40);  
    drawCloud(window_width * 0.2f, window_height * 0.9f, 100, 50);  
    drawCloud(window_width * 0.4f, window_height * 0.85f, 90, 45); 
    drawCloud(window_width * 0.7f, window_height * 0.95f, 120, 60);
    drawCloud(window_width * 0.8f, window_height * 0.75f, 80, 40);  
    drawCloud(window_width * 0.9f, window_height * 0.85f, 120, 60); 
    drawCloud(window_width * 0.3f, window_height * 0.78f, 100, 50); 
    drawCloud(window_width * 0.6f, window_height * 0.90f, 90, 45); 
}


void init() {
    // Beige background
    glClearColor(0.96f, 0.96f, 0.86f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void drawSkyArea() {
    glColor3f(0.765f, 0.871f, 0.969f);  // Light blue for the sky
    glBegin(GL_QUADS);
    glVertex2f(0, window_height * 0.3f); // Start from 30% height from bottom
    glVertex2f(window_width, window_height * 0.3f);
    glVertex2f(window_width, window_height);
    glVertex2f(0, window_height);
    glEnd();
}
void drawGroundArea() {
    glColor3f(0.0f, 0.5f, 0.0f); // Dark green for the ground
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(window_width, 0);
    glVertex2f(window_width, window_height * 0.3f); // Ground covers 30% height from bottom
    glVertex2f(0, window_height * 0.3f);
    glEnd();
}


void drawCollecterArea() {
    glColor3f(0.961f, 0.949f, 0.875f);  // Soft off-white
    glBegin(GL_QUADS);
    glVertex2f(window_width * 0.0f, 0);
    glVertex2f(window_width * 0.25f, 0);
    glVertex2f(window_width * 0.25f, window_height * 0.3f);
    glVertex2f(window_width * 0.0f, window_height * 0.3f);
    glEnd();
}

void drawDistributersArea() {
    glColor3f(0.863f, 0.918f, 0.827f);  // Soft green
    glBegin(GL_QUADS);
    glVertex2f(window_width * 0.25f, 0);
    glVertex2f(window_width * 0.5f, 0);
    glVertex2f(window_width * 0.5f, window_height * 0.3f);
    glVertex2f(window_width * 0.25f, window_height * 0.3f);
    glEnd();
}

void drawSplitterArea() {
    glColor3f(0.871f, 0.827f, 0.918f);  // Soft lavender
    glBegin(GL_QUADS);
    glVertex2f(window_width * 0.5f, 0);
    glVertex2f(window_width * 0.75f, 0);
    glVertex2f(window_width * 0.75f, window_height * 0.3f);
    glVertex2f(window_width * 0.5f, window_height * 0.3f);
    glEnd();
}

void drawFamiliesArea() {
    glColor3f(0.937f, 0.914f, 0.933f);
    glBegin(GL_QUADS);
    glVertex2f(window_width * 0.75f, 0);
    glVertex2f(window_width, 0);
    glVertex2f(window_width, window_height * 0.3f);
    glVertex2f(window_width * 0.75f, window_height * 0.3f);
    glEnd();
}


void display() {
    num_planes = variables->planes_in_airspace;
    if (variables->splitter_to_collector_updates || variables->splitter_to_distributer_updates) {
        printf("Updating display for splitter-to-collector/distributer changes.\n");

        // Reset the flag
        variables->splitter_to_collector_updates = 0;
        variables->splitter_to_distributer_updates = 0;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Apply the clear color and clear depth
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, 0.0, window_height, -1.0, 1.0);

    
    // Reset the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    

    drawSkyArea();
    drawClouds();
    //drawPlane();
    drawGroundArea();
    drawCollecterArea();
    drawDistributersArea();
    drawSplitterArea();
    drawFamiliesArea();
    
    
    drawCollectors();
    drawSplitters();
    drawDistributers(); 
    drawFamilies();
    
    glDisable(GL_DEPTH_TEST);
    drawContainers();
    glEnable(GL_DEPTH_TEST);

    
    
    
        // Draw plane status labels
    char labelBuffer[1024]; // Buffer to hold the string
    sprintf(labelBuffer, "Planes in airspace: ");
    for (int i = 0; i < variables->count_in_airspace; i++) {
        char numBuf[10];
        sprintf(numBuf, "%d ", variables->plane_ids_in_airspace[i]);
        strcat(labelBuffer, numBuf);
    } 
    renderBitmapString(10, window_height - 30, GLUT_BITMAP_HELVETICA_18, labelBuffer);

    sprintf(labelBuffer, "Planes exit due to collision: ");
    for (int i = 0; i < variables->count_exited_collision; i++) {
        char numBuf[10];
        sprintf(numBuf, "%d ", variables->plane_ids_exited_collision[i]);
        strcat(labelBuffer, numBuf);
    }
    renderBitmapString(10, window_height - 50, GLUT_BITMAP_HELVETICA_18, labelBuffer);

    sprintf(labelBuffer, "Planes exit for refill: ");
    for (int i = 0; i < variables->count_exited_refill; i++) {
        char numBuf[10];
        sprintf(numBuf, "%d ", variables->plane_ids_exited_refill[i]);
        strcat(labelBuffer, numBuf);
    }
    renderBitmapString(10, window_height - 70, GLUT_BITMAP_HELVETICA_18, labelBuffer);


    
    // Set the color for the text
    glColor3f(0, 0, 0); // Black color text
    // Drawing titles at the top of each area
    renderBitmapString(window_width * 0.01f, window_height * 0.28f, GLUT_BITMAP_HELVETICA_18, "Collectors");
    renderBitmapString(window_width * 0.26f, window_height * 0.28f, GLUT_BITMAP_HELVETICA_18, "Splitters");
    renderBitmapString(window_width * 0.51f, window_height * 0.28f, GLUT_BITMAP_HELVETICA_18, "Distributers");
    renderBitmapString(window_width * 0.76f, window_height * 0.28f, GLUT_BITMAP_HELVETICA_18, "Families");
    
    glutSwapBuffers(); // Important for double buffering
    glutPostRedisplay();
}

void reshape(int width, int height) {
    if (height == 0) height = 1;  // Prevent division by zero
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);  // Set the viewport to cover the new window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);
}

void drawCollectors() {
    float areaWidth = window_width * 0.25f; // Width of the collector area
    float collectorHeight = 30; // Fixed height for each collector
    float spacingX = 10; // Horizontal spacing between each team's area
    float spacingY = 10; // Vertical spacing between rows of collectors within a team

    float collectorWidth = (areaWidth / num_collectors_teams) - spacingX; // Width per collector, accounting for spacing
    float startX = spacingX; // Start from the left side of the collector area plus initial spacing

    int collectorStartIndex = num_of_splitter + num_of_distributer; // Adjust start index for collectors

    for (int team = 0; team < num_collectors_teams; team++) {
        float x = startX + team * (collectorWidth + spacingX); // X position for each team
        float labelY = window_height * 0.3f; // Y position for labels

        int index = collectorStartIndex + team; // Index for the team in the shared memory
		// Drawing the label with team PID
        glColor3f(0, 0, 0); // Black for text
        char label[30]; // Buffer for the label
        sprintf(label, "TeamID: %d", everyworkers[index].teamid); // Prepare the label
        renderBitmapString(x + 5, labelY, GLUT_BITMAP_HELVETICA_12, label);
        
        for (int collectorIndex = 0; collectorIndex < collecter_per_team; collectorIndex++) {
            float y = window_height * 0.3f - 15 - (collectorIndex + 1) * (collectorHeight + spacingY); // Y position for each collector
           
            char info[256];
            // Set colors based on worker status within the team
            switch (everyworkers[index].status[collectorIndex]) {
                case 0:
                    glColor3f(0.7294, 0.5961, 0.4510); // Default color for alive collectors
				    sprintf(info, "C-%d", collectorIndex+1 );
				    
                    break;
                case 1:
                    glColor3f(1.0, 0.0, 0.0); // Red color for dead collectors
                   
                    sprintf(info, "C-%d", collectorIndex+1 );
				    
                    break;
                case 2:
                	glColor3f(1.0, 0.0, 0.0); // Red color for dead collectors
                	usleep(200000);
                    glColor3f(0.4588, 0.5765, 0.4706); // Color for splitters (assumed splitter color)	
                    sprintf(info, "C-%d R.BY %d", collectorIndex+1,  everyworkers[index].replacement_pid[collectorIndex] );
                    
                    break;
                default:
                    glColor3f(0.5, 0.5, 0.5); // Grey for unknown status
                    break;
            }

            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + collectorWidth, y);
            glVertex2f(x + collectorWidth, y + collectorHeight);
            glVertex2f(x, y + collectorHeight);
            glEnd();

			glColor3f(0, 0, 0); // Black color for text
			renderBitmapString(x + 5, y + 15, GLUT_BITMAP_HELVETICA_10, info);
            
        }
    }
}



void drawSplitters() {
    float areaWidth = window_width * 0.25f; // Width of the splitter area
    int SplitterPerRow = (int)sqrt(num_of_splitter); 
    float splitterWidth = ( areaWidth / SplitterPerRow )- 10; // Width per splitter
    float splitterHeight = 30; // Height of splitter representation
    float startX = window_width * 0.25f + 5; // Start X position
    float startY = window_height * 0.3f - splitterHeight - 10;  // Start Y position

	
    
    int splitterCount = 0;
    char pid_str[20]; // String to hold the PID for display
    char line1[15];


    for (int i = 0; i < num_of_all; i++) {
        if (everyworkers[i].role == SPLITTER && everyworkers[i].is_alive) {
            int col = i % SplitterPerRow;
        int row = i / SplitterPerRow;
        float x = startX + col * (splitterWidth + 10);  // Calculate X position for each family
        float y = startY - row * (splitterHeight + 10);  // Calculate Y position for each family, adding vertical spacing
        

        glColor3f(0.4588, 0.5765, 0.4706); // Color for splitters
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + splitterWidth, y);
        glVertex2f(x + splitterWidth, y + splitterHeight);
        glVertex2f(x, y + splitterHeight);
        glEnd();
            

            // Prepare the PID string
            sprintf(pid_str, "PID: %d", everyworkers[i].pid);
            snprintf(line1, sizeof(line1), "%d", everyworkers[i].pid);
            // Set the text color
            glColor3f(0, 0, 0);
            // Render the second line (actual PID)
            renderBitmapString(x + 5, y + splitterHeight / 2, GLUT_BITMAP_HELVETICA_10, line1);
            

            splitterCount++;
        }
    }
}



void drawDistributers() {
    float areaWidth = window_width * 0.25f; // Width of the distributer area
    int DistributerPerRow = (int)sqrt(num_of_distributer); 
    float distributerWidth = (areaWidth / DistributerPerRow) - 10; // Width per distributer
    float distributerHeight = 30; // Fixed height for the distributer representation
    float startX = window_width * 0.5f + 5; // Start X position
    float startY = window_height * 0.3f - distributerHeight - 10;  // Start Y position


    int distributerCount = 0;
    char pid_str[20]; // String to hold the PID for display
    char line1[15]; // To hold the formatted PID

	int Endindex = num_of_splitter + num_of_distributer;
        int Startindex = num_of_splitter;
            
    for (int i = Startindex; i < Endindex; i++) {
        if (everyworkers[i].role == DISTRIBUTER && everyworkers[i].is_alive) {
            int adjustedIndex = i - Startindex; // Adjust index for proper placement in rows and columns

            int col = adjustedIndex  % DistributerPerRow;
            int row = adjustedIndex  / DistributerPerRow;
            float x = startX + col * (distributerWidth + 10);  // Calculate X position for each distributer
            float y = startY - row * (distributerHeight + 10);  // Calculate Y position for each distributer, adding vertical spacing

            // Prepare the PID string
            sprintf(pid_str, "PID: %d", everyworkers[i].pid);
            snprintf(line1, sizeof(line1), "%d", everyworkers[i].pid);
            
           
             //char info[256];
             // Set colors based on worker status within the team
            switch (everyworkers[i].status[0]) {
                case 0:
                     glColor3f(0.5529, 0.4196, 0.6431);
		      //sprintf(info, "C-%d", distIndex+1);
                    break;
                case 1:
                   // glColor3f(1.0, 0.0, 0.0); // Red color for dead collectors
                   
                    //sprintf(info, "C-%d", distIndex+1 );
				    
                    break;
                case 3:
                    glColor3f(0.4588, 0.5765, 0.4706); // Color for splitters (assumed splitter color)	
                   // sprintf(info, "C-%d R.BY %d", collectorIndex+1,  everyworkers[distIndex].replacement_pid[collectorIndex] );
                    
                    break;
                default:
                    glColor3f(0.5, 0.5, 0.5); // Grey for unknown status
                    break;
            }

            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + distributerWidth, y);
            glVertex2f(x + distributerWidth, y + distributerHeight);
            glVertex2f(x, y + distributerHeight);
            glEnd();

            
            // Set the text color
            glColor3f(0, 0, 0); // Black for the text
            
            renderBitmapString(x + 5, y + distributerHeight / 2, GLUT_BITMAP_HELVETICA_10, line1);


            distributerCount++;
        }
    }
        for (int i = 0; i < num_of_splitter; i++) {
        if (everyworkers[i].role == DISTRIBUTER && everyworkers[i].is_alive) {
          
            int col = i  % DistributerPerRow;
            int row = i  / DistributerPerRow;
            float x = startX + col * (distributerWidth + 10);  // Calculate X position for each distributer
            float y = startY - row * (distributerHeight + 10);  // Calculate Y position for each distributer, adding vertical spacing

            
            // Prepare the PID string
            sprintf(pid_str, "PID: %d", everyworkers[i].pid);
            snprintf(line1, sizeof(line1), "%d", everyworkers[i].pid);
            
           
             //char info[256];
             // Set colors based on worker status within the team
            switch (everyworkers[i].status[0]) {
                case 0:
                     glColor3f(0.5529, 0.4196, 0.6431);
		      //sprintf(info, "C-%d", distIndex+1);
                    break;
                case 1:
                   // glColor3f(1.0, 0.0, 0.0); // Red color for dead collectors
                   
                    //sprintf(info, "C-%d", distIndex+1 );
				    
                    break;
                case 3:
                    glColor3f(0.4588, 0.5765, 0.4706); // Color for splitters (assumed splitter color)	
                   // sprintf(info, "C-%d R.BY %d", collectorIndex+1,  everyworkers[distIndex].replacement_pid[collectorIndex] );
                    
                    break;
                default:
                    glColor3f(0.5, 0.5, 0.5); // Grey for unknown status
                    break;
            }

            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + distributerWidth, y);
            glVertex2f(x + distributerWidth, y + distributerHeight);
            glVertex2f(x, y + distributerHeight);
            glEnd();
            
            // Set the text color
            glColor3f(0, 0, 0); // Black for the text
            
            renderBitmapString(x + 5, y + distributerHeight / 2, GLUT_BITMAP_HELVETICA_10, line1);

            distributerCount++;
        }
    }
}


void drawFamilies() {
    float areaWidth = window_width * 0.25f;  // The width of the families area is 25% of the window width
    float areaHeight = window_height * 0.3f;  // The height of the families area is 30% of the window height
    int familiesPerRow = (int)sqrt(MAX_FAMILIES);  // Calculate a balanced number of families per row
    float familyWidth = (areaWidth / familiesPerRow) - 10;  // Allocate width to each family, subtracting some spacing
    float familyHeight = (areaHeight / (MAX_FAMILIES / familiesPerRow + (MAX_FAMILIES % familiesPerRow ? 1 : 0))) - 10;  // Adjust height accordingly
    float startX = window_width * 0.75f;  // Start drawing from 75% width of the window
    float startY = window_height * 0.3f - familyHeight - 10;  // Start drawing near the bottom part of the area

    char familyID_str[20]; // Buffer to hold family ID for display
    char line1[15]; // To hold the formatted family ID
    
    // Find the maximum starvation rate to scale colors
    int maxStarvation = 0;
    for (int i = 0; i < MAX_FAMILIES; i++) {
        if (fam[i].starvationRate > maxStarvation) {
            maxStarvation = fam[i].starvationRate;
        }
    }
    
    int thresholdHigh = maxStarvation * 2 / 3; // Top 33%
    int thresholdMedium = maxStarvation / 3; // Middle 33%
    
    for (int i = 0; i < MAX_FAMILIES; i++) {
        int col = i % familiesPerRow;
        int row = i / familiesPerRow;
        float x = startX + col * (familyWidth + 10);  // Calculate X position for each family
        float y = startY - row * (familyHeight + 10);  // Calculate Y position for each family, adding vertical spacing
        
        
        
          // Color coding based on starvation rate
        if (fam[i].starvationRate > thresholdHigh) {
            glColor3f(0.3529, 0.3490, 0.3529); // Dark grey for most starved
            if(fam[i].is_alive == 0){
            glColor3f(1, 0, 0); //died
            }
        } else if (fam[i].starvationRate > thresholdMedium) {
            glColor3f(0.7608, 0.7373, 0.7569); // Lighter grey for medium starved
        } else {
            glColor3f(1.0, 1.0, 1.0); // White for least starved
        }
        
        
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + familyWidth, y);
        glVertex2f(x + familyWidth, y + familyHeight);
        glVertex2f(x, y + familyHeight);
        glEnd();
        
        fam[i].familyID = i;
        // Prepare the family ID string
        sprintf(familyID_str, "ID: %d", fam[i].familyID);
        snprintf(line1, sizeof(line1), "%d", fam[i].familyID);
        
        // Set the text color
        glColor3f(0, 0, 0); // Black for the text to ensure visibility

        // Render the family ID multiple times to simulate bold text
        float textX = x + 5; // Position text inside the rectangle
        float textY = y + familyHeight / 2 - 5; // Center text vertically within the rectangle
        renderBitmapString(textX, textY, GLUT_BITMAP_HELVETICA_10, line1);
    }
}
/*
void drawPlane() {
    int num_segments = 20; // Number of segments to create a circle
    float main_radius_x = 25.0f; // Width for the main circle (front of the plane)
    float main_radius_y = 20.0f; // Height for the main circle
    float tail_radius = 8.0f; // Radius for the tail circles
    float angle_step = 2.0f * M_PI / num_segments; // Step between each point in circle

    int num_planes = variables->planes_in_airspace; // Get the number of planes from shared memory
    float spacing = window_width / (num_planes + 1); // Calculate spacing based on window width

    glColor3f(0.0f, 0.0f, 0.0f); // Set color to black for all planes

    for (int p = 0; p < num_planes; p++) {
        float x = (p + 1) * spacing; // Calculate x position based on spacing
        float y = window_height * 0.75; // Keep y position constant for all planes, similar to the first code

        // Draw main circle (ellipse for more width)
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y); // Center of circle
        for (int i = 0; i <= num_segments; i++) {
            float angle = i * angle_step;
            glVertex2f(x + cos(angle) * main_radius_x, y + sin(angle) * main_radius_y);
        }
        glEnd();
        

        // Draw both tail circles positioned to the left of the main circle
        float tail_center_x = x - main_radius_x - tail_radius;

        // Draw first tail circle
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(tail_center_x, y); // Center of first tail circle
        for (int i = 0; i <= num_segments; i++) {
            float angle = i * angle_step;
            glVertex2f(tail_center_x + cos(angle) * tail_radius, y + sin(angle) * tail_radius);
        }
        glEnd();

        // Draw second tail circle, slightly above the first
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(tail_center_x, y + 2 * tail_radius); // Center of second tail circle
        for (int i = 0; i <= num_segments; i++) {
            float angle = i * angle_step;
            glVertex2f(tail_center_x + cos(angle) * tail_radius, y + 2 * tail_radius + sin(angle) * tail_radius);
        }
        glEnd();
    }
}
*/
void drawContainers() {
    float containerWidth = 20.0f;  // Width of each container representation
    float containerHeight = 10.0f;  // Height of each container representation
    float yOffset = 100.0f;  // Increase Y-offset to draw containers higher above the area

    // Constants defined for each area's position and width
    float collectorBaseX = 0.0f;  // Starting X for collector area
    float splitterBaseX = window_width * 0.25f;  // Starting X for splitter area
    float distributorBaseX = window_width * 0.5f;  // Starting X for distributor area
    float areaWidth = window_width * 0.25f;  // Width for each area

    int containersPerRow = floor(areaWidth / (containerWidth + 5.0f));  // Fit containers within area width

    // Iterate over all containers
    for (int i = 0; i < variables->container_count; i++) {
        Container container = variables->containers[i];

        // Choose base X and calculate positions based on container's location
        float baseX = 0.0f;
        switch (container.location) {
            case COLLECTER:  // Collector area
                baseX = collectorBaseX;
                break;
            case SPLITTER:  // Splitter area
                baseX = splitterBaseX;
                break;
            case DISTRIBUTER:  // Distributor area
                baseX = distributorBaseX;
                break;
        }

        float x = baseX + (container.index % containersPerRow) * (containerWidth + 10.0f); // Calculate x position
        float y = window_height * 0.4f - (container.index / containersPerRow) * (containerHeight + 10.0f) - yOffset; // Calculate y higher

        // Set color based on the status of the container
        if (container.status == 0) {
            glColor3f(1.0f, 0.0f, 0.0f);  // Red for destroyed
        } else if (container.status == 1) {
            glColor3f(0.0f, 1.0f, 0.0f);  // Green for intact
        } else {
            glColor3f(0.5f, 0.5f, 0.5f);  // Grey for incompletely destroyed
        }

        // Draw the container
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + containerWidth, y);
        glVertex2f(x + containerWidth, y + containerHeight);
        glVertex2f(x, y + containerHeight);
        glEnd();

        // Optionally, draw container ID
        glColor3f(0.0f, 0.0f, 0.0f);  // Black for text
        //char idBuf[10];
        //sprintf(idBuf, "%d", container.id);
        //renderBitmapString(x + 5, y + 5, GLUT_BITMAP_HELVETICA_12, idBuf);
    }
}

