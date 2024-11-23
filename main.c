#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>

#define Q_LIMIT 100
#define BUSY 1
#define IDLE 0

float mean_interarrival, mean_service, uniform_rand, sim_clock, time_last_event, total_time_delayed, area_num_in_q, area_server_status, time_arrival[Q_LIMIT+1], end_time;
int delays_required, num_in_q, customers_delayed, event_type, num_servers;
int *server_status;
float *event_list;

void arrive(int);
void depart(int);
void update_time_avg_stats(void);
void write_report(FILE *);
int timing(void);
int find_idle_server(void);
int still_processing(void);
float gen_rand_uniform(void);
float gen_rand_exponential(float);

/* Initialises the sim */
void initialise_sim(void)
{
  // Seed random number generator with current time
  srand(time(NULL));

  // Initialise Sim Variables
  sim_clock = 0.0;
  num_in_q = 0;
  time_last_event = 0;
  
  // Allocate memory for the server status array
  server_status = (int* )malloc(num_servers * sizeof(int));
  if (server_status == NULL) {
	  printf("Error allocating memory!");
	  exit(1);
  }

  // Initialise all servers to be idle
  for (int i = 0; i < num_servers; i++) {
    server_status[i] = IDLE;
  }

  // Allocate memory for the event list array
  event_list = (float*)malloc((num_servers + 2) * sizeof(int));
  if (event_list == NULL) {
	printf("Error allocating memory!");
  	exit(1);
  }

  // Initialise event list end and arrival
  *event_list = end_time;
  *(event_list + 1) = gen_rand_exponential(mean_interarrival);

  // Initialise all departure times
  for (int i = 2; i < num_servers + 2; i++){
    *(event_list + i) = FLT_MAX;
  }

  // Initialise all servers to be idle
  for (int i = 0; i < num_servers; i++) {
    server_status[i] = IDLE;
  }

  // Initialise Statistical Counters
  customers_delayed = 0;
  total_time_delayed = 0.0;
  area_num_in_q = 0.0;
  area_server_status = 0.0;
  
  // Initialise event list

  
  // Set all values in queue to -1
  for (int i = 0; i < Q_LIMIT; i++)
  {
    time_arrival[i] = -1;
  }
  
  //printf("arrival: %f\ndeparture: %f\n", event_list[0], event_list[1]);
}

/* Update time average stats */
void update_time_avg_stats()
{
  // Calculate time since last event and update time last event to current time
  float time_since_last_event = sim_clock - time_last_event; // Delta x in equations
  time_last_event = sim_clock;
  
  /* Update stats where area_num_in_q is the cumulative area under the graph where the y axis is the number of customers in the queue
  and the x axis is the time. We can later use this to calculate the average number of customers in the queue during the simulation.
  The area_server_status is the cumulative area under the graph with y axis being the server utilisation (0 or 1 for idle / busy) and the
  x axis is time. This will allow us to calculate the proportion of server utilisation during the simulation. */
  
  area_num_in_q += num_in_q * time_since_last_event;
  for (int i = 0; i < num_servers; i++)
  {
	area_server_status += server_status[i] * time_since_last_event;
  }
}

/* Calculates performance metrics and writes report to file */
void write_report(FILE * report)
{
  // Average delay and size in the queue and server utilisation | sim_clock should be total time at the end of the sim
  float avg_q_delay = total_time_delayed / customers_delayed;
  float avg_q_size = area_num_in_q / sim_clock;
  float server_utilisation = area_server_status / sim_clock / num_servers;
  
  fprintf(report, "\nSimulation stats\nAverage delay in the queue: %0.3f minutes\nAverage queue length: %11.3f customers\nAverage server utilisation: %0.3f%%\nDuration of simulation: %11.3f minutes", avg_q_delay, avg_q_size, server_utilisation*100, sim_clock);
}

/* Returns 1 if at least 1 server is busy and 0 if all servers are idle */
int still_processing()
{
  int processing = 0;
  for (int i = 0; i < num_servers; i++)
  {
	if (server_status[i] == BUSY){
	  processing = 1;
	  break;
	}
  }
  return processing;
}

/* Determine next event and advance sim clock */
int timing()
{
  // Determine next event
  int next_event_type = 0;
  int processing = 1;
  float min_time;
  
  // Prevents new arrivals past the simulation closing time
  if (event_list[1] < end_time){
	min_time = event_list[1];
	next_event_type = 1;
  } else{
	min_time = FLT_MAX-1;
	processing = still_processing();
  }

  // Allows the simulation to continue running until all customers have departed
  if (processing == 1){
    // Iterates through servers to find if a departure is the most imminent event
    for (int i = 2; i < (num_servers+2); i++)
    {
      if (event_list[i] < min_time)
	  {
	    min_time = event_list[i];
	    next_event_type = i;
	  }
    }
  } else { // Simulation complete
	// Set time to end duration if less
	if (sim_clock < end_time){
		min_time = end_time;
	} else{
	  min_time = sim_clock;
	}
	next_event_type = 0;
  }
  
  // Advance sim clock
  sim_clock = min_time;
  
  // 0 for end time condition, 1 for arrival, 2... for departure index
  return next_event_type;
}

int find_idle_server()
{
  // Not found return -1
  int idle_server = -1;

  for (int i = 0; i < num_servers; i++){
    if (server_status[i] == IDLE){
      idle_server = i;
      break;
    }
  }
  return idle_server;
}

/* Next departure event */
void depart(int server_index)
{
  // Decrease index by 2 to account for end time and arrival
  server_index -= 2;
  //printf("server_index: %d\n",server_index);
  //printf("queue 0: %f\n", time_arrival[0]);

  // If queue is empty
  if (time_arrival[0] == -1){ 
    // Make server idle
    server_status[server_index] = IDLE;
    
    // Remove departure from consideration
    *(event_list + server_index + 2) = FLT_MAX;
  } else{
    // Reduce the queue by 1
    num_in_q--;
    
    // Compute delay of customer entering service
    total_time_delayed += sim_clock - time_arrival[0];
    customers_delayed++;
    
    // Schedule departure event
    *(event_list + server_index + 2) = sim_clock + gen_rand_exponential(mean_service);
    
    // Move other customers in the queue up one place
    for (int i = 0; i < num_in_q; i++)
    {
      time_arrival[i] = time_arrival[i+1];
    }
    time_arrival[num_in_q] = -1;
  }
}

/* Next arrival event */
void arrive(int server_index)
{ 
  // Decrease index by 2 to account for end time and arrival
  server_index -= 2;

  // Schedule next arrival
  *(event_list + 1) = sim_clock + gen_rand_exponential(mean_interarrival);
  
  // Find an idle server
  int idle_server = find_idle_server();

  if (idle_server != -1){
    // Add 1 to customers delayed but don't add any time delayed as they are served instantly
    customers_delayed++;
  
    // Make server busy
    *(server_status + idle_server) = BUSY;
    
    // Schedule departure event for current customer
    *(event_list + idle_server + 2) = sim_clock + gen_rand_exponential(mean_service);
  } else {
    // Stop simulation if queue is full
    if (num_in_q > Q_LIMIT){
      printf("Error! Stack Overflow in queue");
      exit(1);
    }
    
    // Add new customer to the queue
    time_arrival[num_in_q] = sim_clock;
    
    // Number of customers in queue increases by 1
    num_in_q++;
  }
}

/* Generate random uniformly distribute variate between 0 and 1 */
float gen_rand_uniform()
{  
  // Generates variate between 0 and 1
  float random_variate = (float)rand() / RAND_MAX;
   
  return random_variate;
}

/* Generate random exponentially distributed variate between 0 and 1 with a mean of beta */
float gen_rand_exponential(float beta)
{
  return -1 * beta * log(gen_rand_uniform());
} 

int main(void)
{
  FILE *config, *report;
  // Open files
  config = fopen("config.in","r");
  report = fopen("report.txt","w");
  
  // Check files exist
  if(config == NULL || report == NULL)
  {
    printf("Error! File not read.");
    exit(1);
  }  
  
  // Load input parameters
  fscanf(config, "%f %f %f %d", &mean_interarrival, &mean_service, &end_time, &num_servers);
  fclose(config);
  
  // Write heading of report
  fprintf(report, "Multiple Server Queueing System Simulation Report\n\nInput parameters\nNumber of servers: %10d servers\nMean interarrival time: %12f minutes\nMean service time: %17f minutes\nStop accepting arrivals at: %f minutes\n",num_servers, mean_interarrival, mean_service, end_time);
  
  // Initialise sim
  initialise_sim();

  // Simulation Loop
  do {
	//printf("num in q: %d\n", num_in_q);
	//printf("sim clock: %f\n\n",sim_clock);
    // Timing event to determine next event
    event_type = timing();
    
    // Update Time Average Statistical Counters
    update_time_avg_stats();
    
    //printf("cust_delay: %d\ntotal time delayed: %f\n", customers_delayed, total_time_delayed);
    
    // Call correct event function
    switch (event_type){
      case 0: // end time
    	break;
      case 1: // arrival
        arrive(event_type);
        break;
      default:
        depart(event_type);
        break;
    }
  } while (event_type != 0);
  //printf("final time: %f",sim_clock);
  // Call the report writing function
  write_report(report);
  fclose(report);
  
  return 0;
}
