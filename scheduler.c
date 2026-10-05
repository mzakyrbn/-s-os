#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//struct bikin sendiri
typedef struct {
  int *array;
  size_t used;
  size_t size;
} Array;

typedef struct {
	char pid[31];
	int at; //arrival time
	int bt; //burst time
	int wt; //waiting time
	int *movTimes; //array to store movement time from a queue to another 
	int movTimesCnt; //counter for movTimes
	bool onQue; //sign if its on a queue
	bool migrate; //sign if its gonna migrate
	bool terminated; //sign if its terminated
	int terminatedTime; //times it terminated
} Process; 

int getMax(int a, int b);
int getMin(int a, int b);
int assignQue(Process *processes, Process **MainQueue, int processTotal, int QueueCnt, int time);
void rearrange(Process **queue, int *queueCnt);
void initArray(Array *a, size_t initialSize);
void insertArray(Array *a, int element);
void checkContextSwitch(Process *currentProcess, Process **lastProcess, int *contextSwitchCount); 
double countCPUUtilization(int time, int idle);
double countThroughput(int time, int processCount);

int main() {
	const int totalQueue = 3;
	int idle = 0;
	int inputReport;
	int processTotal;
	int tq1; //time quantum Q0
	int tq2; //time quantum Q1

	do {
		printf("Jumlah proses: ");
		inputReport = scanf("%d", &processTotal);

		if (inputReport == 0) printf("Input salah. Harap masukkan input dengan benar.\n");
	} while (inputReport == 0);

	do {
		printf("Quantum Q0 (RR > 0): ");
		inputReport = scanf("%d", &tq1);

		if (inputReport == 0) printf("Input salah. Harap masukkan input dengan benar.\n");
		else if (tq1 <= 0) printf("time quantum harus lebih besar dari 0\n");
		
	} while (inputReport == 0 || tq1 <= 0);

	do {
		printf("Quantum Q0 (RR > 0): ");
		inputReport = scanf("%d", &tq2);

		if (inputReport == 0) printf("Input salah. Harap masukkan input dengan benar.\n");
		else if (tq1 <= 0) printf("time quantum harus lebih besar dari 0\n");
		
	} while (inputReport == 0 || tq2 <= 0);

	Process *processes = malloc(processTotal * sizeof(Process));

	for (int i = 0; i < processTotal; i++) {
		int at;
		int bt;
		printf("P%d - masukkan AT BT queue: ", i);
		do {
			inputReport = scanf("%d %d", &at, &bt);

			if (inputReport == 0) printf("Input salah. Harap masukkan input dengan benar.\n");
			else if (at < 0 || bt <= 0) printf("at tidak boleh negatif dan bt harus lebih besar dari 0\n");
		} while (at < 0 || bt <= 0);

		sprintf(processes[i].pid, "P%d", i + 1);
		processes[i].at = at;
		processes[i].bt = bt;
		processes[i].wt = 0;
		processes[i].movTimes = malloc(totalQueue * sizeof(int));
		processes[i].movTimes[0] = at; //assigning first movement to Q0
		processes[i].movTimesCnt = 1;
		processes[i].onQue = false;
		processes[i].migrate = false;
		processes[i].terminated = false;
		processes[i].terminatedTime = 0;
	}

	//process input and queue configuration
	printf("=======================================================================\n"
		   "PROCESS INPUT AND QUEUE CONFIGURATION\n"
		   "=======================================================================\n"
		   "Q0: RR (quantum:%d) | Q1: RR (quantum:%d) | Q2 FCFS\n"
		   "PID%*sAT%*sBT Initial Queue\n"
		   "-----------------------------------------------------------------------\n", tq1, tq2, 13, "", 9, "");
	for (int i = 0; i < processTotal; i++) {
		printf("%s:%*s%d%*s%d%*sQ0\n", processes[i].pid, 15, "", processes[i].at, 10, "", processes[i].bt, 12, "");
	}
	printf("=======================================================================\n\n");

	//CPU Execution Timeline (Gantt Chart)
	printf("=======================================================================\n"
		   "CPU EXECUTION TIMELINE (GANTT CHART)\n"
		   "=======================================================================\n");
	
	int first = 0;
	int time = 0;
	bool done = false;
	Process **q0 = malloc(processTotal * sizeof(Process *));
	Process **q1 = malloc(processTotal * sizeof(Process *));
	Process **q2 = malloc(processTotal * sizeof(Process *));
	int q0cnt = 0;
	int q1cnt = 0;
	int q2cnt = 0;
	int finished = 0;
	
	Array checkPoint;
	initArray(&checkPoint, 15);
	insertArray(&checkPoint, time);
	printf("|");
	
	//to help count context switch
	Process *lastProcess = NULL;
	int contextSwitchCount = 0;

	//the hell's begin
	while (!done) {
		q0cnt = assignQue(processes, q0, processTotal, q0cnt, time);

		int timeConsumed;

		while (q0cnt > 0) {
			Process *current = q0[first];
			timeConsumed = getMin(tq1, current->bt);
			current->bt = getMax(current->bt - tq1, 0);
			time += timeConsumed;

			checkContextSwitch(current, &lastProcess, &contextSwitchCount);
			
			if (current->bt > 0) {
				current->migrate = true;
				current->movTimes[current->movTimesCnt] = time;
				current->movTimesCnt++;
				q1[q1cnt] = q0[first];
				q1cnt++;
			}
			else {
				current->terminated = true;
				current->onQue = false;
				current->terminatedTime = time;
				finished++;
			}
			printf("%*s%s(Q0)%*s|", 3, "", current->pid, 3, "");
			insertArray(&checkPoint, time);
			rearrange(q0, &q0cnt);

			q0cnt = assignQue(processes, q0, processTotal, q0cnt, time);
		}

		while (q1cnt > 0) {
			if (q0cnt > 0) break;
			Process *current = q1[first];
			current->migrate = false;
			timeConsumed = getMin(tq2, current->bt);
			current->bt = getMax(current->bt - tq2, 0);
			time += timeConsumed;
			
			checkContextSwitch(current, &lastProcess, &contextSwitchCount);

			if (current->bt > 0) {
				current->migrate = true;
				current->movTimes[current->movTimesCnt] = time;
				current->movTimesCnt++;
				q2[q2cnt] = q1[first];
				q2cnt++;
			}
			else {
				current->terminated = true;
				current->onQue = false;
				current->terminatedTime = time;
				finished++;
			}
			printf("%*s%s(Q1)%*s|", 3, "", current->pid, 3, "");
			rearrange(q1, &q1cnt);
			insertArray(&checkPoint, time);

			q0cnt = assignQue(processes, q0, processTotal, q0cnt, time);
			if (q0cnt > 0) break;
		}

		while (q2cnt > 0) {
			if (q0cnt > 0 || q1cnt > 0) break;
			Process *current = q2[first];
			current->migrate = false;
			timeConsumed = current->bt;
			time += timeConsumed;
			current->terminated = true;
			current->onQue	= false;
			current->terminatedTime = time;
			finished++;

			checkContextSwitch(current, &lastProcess, &contextSwitchCount);

			rearrange(q2, &q2cnt);
			insertArray(&checkPoint, time);
			printf("%*s%s(Q2)%*s|", 3, "", current->pid, 3, "");
		}

		if (finished == processTotal) {
			done = true;
			continue;
		}

		if (q0cnt == 0 && q1cnt == 0 && q2cnt == 0) {
			while (q0cnt == 0) {
				time++;
				idle++;
				q0cnt = assignQue(processes, q0, processTotal, q0cnt, time);
			}
			insertArray(&checkPoint, time);
			printf("%*sidle%*s|", 3, "", 3, ""); 

			lastProcess = NULL; 
		}
	}

	printf("\n");
	for (int i = 0; i < checkPoint.used; i++) {
		printf("%d%*s", checkPoint.array[i], 12, "");
	}
	printf("\n");

	//Process / Queue movements

	for (int i = 0; i < processTotal; i++) {
    Process *current = &processes[i];

    printf("%s : Q0 (t=%d)", current->pid, current->movTimes[0]);

    if (current->movTimesCnt >= 2) {
        printf(" -> Q1 (t=%d)", current->movTimes[1]);
    }

    if (current->movTimesCnt >= 3) {
        printf(" -> Q2 (t=%d)", current->movTimes[2]);
    }

    printf(" -> TERMINATED (t=%d)\n", current->terminatedTime);
	}
	printf("\n");

	//Queue Migration

	// cpu utilization and throughput
	double cpuUtilization = countCPUUtilization(time, idle);
	double throughput = countThroughput(time, processTotal);
	printf("=======================================================================\n"
		   "CPU UTILIZATION AND THROUGHPUT\n"
		   "=======================================================================\n");
	printf("CPU Utilization	: %.2f%%\n", cpuUtilization);
	printf("Throughput	: %.2f process/time unit\n", throughput);

	// context switch
	printf("\n=======================================================================\n"
		   "CONTEXT SWITCH INFORMATION\n"
		   "=======================================================================\n");
	printf("Total Context Switch	: %d\n", contextSwitchCount);
}

//helper function (just make your own dont even bother to read mine)
//tis mine (make your own area to make it easy for yourself)
int getMax(int a, int b) {
	if (a >= b) return a;
	return b;
}

int getMin(int a, int b) {
	if (a <= b) return a;
	return b;
}

int assignQue(Process *processes, Process **MainQueue, int processTotal, int QueueCnt, int time) {
    for (int i = 0; i < processTotal; i++) {
        if (!processes[i].onQue && !processes[i].terminated && time >= processes[i].at) {
            MainQueue[QueueCnt] = &processes[i];
            processes[i].onQue = true;
            QueueCnt++;
        }
    }

    return QueueCnt;
}

void rearrange(Process **queue, int *queueCnt) {
    for (int i = 0; i < *queueCnt - 1; i++) {
        queue[i] = queue[i + 1];
    }
    (*queueCnt)--;
}

void initArray(Array *a, size_t initialSize) {
  a->array = malloc(initialSize * sizeof(int));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, int element) {
  if (a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}
// endarea

// count context switch
void checkContextSwitch(Process *currentProcess, Process **lastProcess, int *contextSwitchCount) {
	// check if current Process with past Process is difference or no 
	if (*lastProcess != NULL && *lastProcess != currentProcess) {
		(*contextSwitchCount)++;
	}
	// update last process running
	*lastProcess = currentProcess;
}

// count cpu utilization ((total time - idle time) / total time) * 100% 
double countCPUUtilization(int time, int idle) {
	if (time == 0) return 0.0;
	double cpuUtilization = ((double) (time-idle) / time) * 100.0;
	return cpuUtilization;
}

// count throughput (jumlah proses / total time)
double countThroughput(int time, int processCount) {
	if (time == 0) return 0.0;
	double throughput = ((double) processCount / time);
	return throughput;
}



