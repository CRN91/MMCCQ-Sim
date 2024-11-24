#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>

#define Q_LIMIT 100
#define BUSY 1
#define IDLE 0

float mean_interarrival, mean_service, sim_clock, time_last_event, area_server_status, end_time, cumulative_arrival_rate;
int customers_delayed, event_type, num_servers, customers_lost;
int *server_status;
float *event_list;

void arrive(int);
void depart(int);
void update_time_avg_stats(void);
void gen_sim_csv(void);
void run_sim(void);
void write_report(FILE *);
void write_csv(FILE *);
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
  area_server_status = 0.0;
  customers_lost = 0;
  cumulative_arrival_rate = 0;
}

/* Update time average stats */
void update_time_avg_stats()
{
  // Calculate time since last event and update time last event to current time
  float time_since_last_event = sim_clock - time_last_event; // Delta x in equations
  time_last_event = sim_clock;
  
  // If server status is idle it multiplies by 0 so adds nothing, if its busy it multiplies by 1 creating a cumulative total
  for (int i = 0; i < num_servers; i++)
  {
	area_server_status += server_status[i] * time_since_last_event;
  }
}

/* Calculates performance metrics and writes report to file */
void write_report(FILE * report)
{
  // Average delay and size in the queue and server utilisation | sim_clock should be total time at the end of the sim
  float server_utilisation = area_server_status / sim_clock / num_servers;
  float total_customers = customers_lost + customers_delayed;
  float blocking_probability = (float)customers_lost / total_customers;
  fprintf(report, "\nSimulation stats\nNumber of customers lost: %5d customers\nTotal Customers: %22f\nBlocking probability: %14f%%\nAverage server utilisation: %0f%%\nActual arrival rate: %16f seconds\nDuration of simulation: %16f seconds", customers_lost, total_customers, blocking_probability*100, server_utilisation*100, cumulative_arrival_rate/total_customers, sim_clock);
}

void write_csv(FILE * csv)
{
  float server_utilisation = area_server_status / sim_clock / num_servers;
  float total_customers = customers_lost + customers_delayed;
  float blocking_probability = (float)customers_lost / total_customers;
  fprintf(csv,"%f,%f,%f\n",cumulative_arrival_rate/total_customers,blocking_probability,server_utilisation);
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

  // Finds next idle server checking in order
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

  // Make server idle
  server_status[server_index] = IDLE;
    
  // Remove departure from consideration
  *(event_list + server_index + 2) = FLT_MAX;
}

/* Next arrival event */
void arrive(int server_index)
{ 
  // Decrease index by 2 to account for end time and arrival
  server_index -= 2;

  // Schedule next arrival
  float gen_arrival = gen_rand_exponential(mean_interarrival);
  *(event_list + 1) = (sim_clock + gen_arrival);
  cumulative_arrival_rate += gen_arrival;
  
  // Find an idle server
  int idle_server = find_idle_server();

  // If a server is available assign the customer to that server
  if (idle_server != -1){
    // Add 1 to customers delayed
    customers_delayed++;
  
    // Make server busy
    *(server_status + idle_server) = BUSY;
    
    // Schedule departure event for current customer
    *(event_list + idle_server + 2) = sim_clock + gen_rand_exponential(mean_service);
  } else {
    // Customer is lost
	customers_lost++;
  }
}

/* Generate random uniformly distribute variate between 0 and 1 */
float gen_rand_uniform()
{  
  // Generates variate between 0 (inclusive) and 1 (exclusive)
  return (float)rand() / (float)((unsigned)RAND_MAX + 1);
}

/* Generate random exponentially distributed variate between 0 and 1 with a mean of beta */
float gen_rand_exponential(float beta)
{
  return -1 * beta * log(1 - gen_rand_uniform());
}

void gen_sim_csv(void)
{
  FILE *csv;

  // Open csv
  csv = fopen("sim_data.csv","w");
  // Write csv heading
  fprintf(csv,"Interarrival rate,Blocking probability,Server utilisation\n");

  // Run simulation 100 times
  for (int i = 0; i < 90; i++)
  {
	run_sim();
	mean_interarrival++;
	write_csv(csv);
  }
  fclose(csv);
}

void run_sim(void)
{
  // Initialise sim
  initialise_sim();

  // Simulation Loop
  do {
    // Timing event to determine next event
	event_type = timing();
	// Update Time Average Statistical Counters
	update_time_avg_stats();

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
  
  // Default behaviour is to run the sim once, if set to 0 will run 100 times which generates a csv I use for my coursework
  int single_run = 1;
  if (single_run == 1)
  {
    // Write heading of report
    fprintf(report, "Multiple Server Queueing System with Loss Simulation Report\n\nInput parameters\nNumber of servers: %11d servers\nMean interarrival time: %13f seconds\nMean service time: %19f seconds\nStop accepting arrivals at: %f seconds\n",num_servers, mean_interarrival, mean_service, end_time);
  
    //Run the simulation
    run_sim();

    // Call the report writing function
    write_report(report);
    fclose(report);
  } else{
	gen_sim_csv();
  }
  
  return 0;
}
