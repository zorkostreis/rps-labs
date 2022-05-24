#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

typedef struct {
	pthread_t tid;
	int first;
	int last;
} ThreadRecord;

int M, N; // Размерность сетки
double* cur, * prev; // Массивы значений напряжений в текущий и предыдущий момент времени
double U = 20.0;
double C = 1.0;
double R = 5.0;
double h = 1.0;
double I = 10.0;
int done = 0;
ThreadRecord* threads; // Массив индексов границ лент матрицы
pthread_barrier_t barr1, barr2;

void* my_solver(void* arg_p) {
	ThreadRecord* thr;
	int i, j;
	double tmp = 0.0;
	thr = (ThreadRecord*)arg_p;
	while (!done) {
		pthread_barrier_wait(&barr1);
		for (i = thr->first; i <= thr->last; i++) {
			for (j = 0; j < N; j++) {
				// Условия вычислений для четырех концов сетки RC-элементов
				if (j == 0 && i == 0) {
					cur[i * N + j] = U;
					continue;
				}
				else if (j == N - 1 && i == 0) {
					cur[i * N + j] = U;
					continue;
				}
				else if (j == 0 && i == M - 1) {
					cur[i * N + j] = U;
					continue;
				}
				else if (j == N - 1 && i == M - 1) {
					cur[i * N + j] = U;
					continue;
				} // Условия для вычисления остальных границ сетки RC-элементов
				else if (j == 0)
					tmp = prev[(i - 1) * N + j] + prev[i * N + j + 1] +
					prev[(i + 1) * N + j];
				else if (j == N - 1)
					tmp = prev[i * N + j - 1] + prev[(i - 1) * N + j] +
					prev[(i + 1) * N + j];
				else if (i == 0)
					tmp = prev[i * N + j - 1] + prev[i * N + j + 1] +
					prev[(i + 1) * N + j];
				else if (i == M - 1)
					tmp = prev[i * N + j - 1] + prev[(i - 1) * N + j] +
					prev[i * N + j + 1];
				// Условие для части расчета внутренних узлов сетки RC-элементов
				else
					tmp = prev[i * N + j - 1] + prev[(i - 1) * N + j] +
					prev[i * N + j + 1] + prev[(i + 1) * N + j];
				// Окончательный подсчет значения напряжения в узле
				cur[i * N + j] = h * tmp / C / R + prev[i * N + j] * (1 - 4 *
					h / C / R);
			}
		}
		pthread_barrier_wait(&barr2);
	}
}

int main(int argc, char* argv[]) {
	int nthread; // Кол-во расчётных потоков
	int ntime;   // Кол-во циклов
	int i, j, k;

	pthread_attr_t pattr;

	if (argc != 5)
		exit(1);

	M = atoi(argv[1]);
	N = atoi(argv[2]);
	nthread = atoi(argv[3]);
	ntime = atoi(argv[4]);

	if (M % nthread)
		exit(2);

	// Выделение памяти
	cur = (double*)malloc(sizeof(double) * M * N);
	prev = (double*)malloc(sizeof(double) * M * N);

	memset(cur, 0, sizeof(double) * M * N);
	memset(prev, 0, sizeof(double) * M * N);

	pthread_attr_init(&pattr);
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

	threads = (ThreadRecord*)calloc(nthread, sizeof(ThreadRecord));

	pthread_barrier_init(&barr1, NULL, nthread + 1);
	pthread_barrier_init(&barr2, NULL, nthread + 1);

	j = M / nthread;
	for (i = 0; i < nthread; i++) {
		threads[i].first = j * i; // Индекс начальной строки для потока
		threads[i].last = j * (i + 1) - 1; // Индекс конечной строки для потока
		if (pthread_create(&(threads[i].tid), &pattr, my_solver, (void*)
			&(threads[i]))) // Создание потоков управления
			perror("pthread_create");
	}
	struct timeval tv1, tv2, dtv;
	struct timezone tz;
	gettimeofday(&tv1, &tz);

	pthread_barrier_wait(&barr1); // Старт расчётов

	FILE* fd = fopen("output.txt", "w");
	FILE* fp = fopen("vg.dat", "w");
	fprintf(fp, "set dgrid3d\n");
	fprintf(fp, "set hidden3d\n");
	fprintf(fp, "set xrange [0:%d]\n", M);
	fprintf(fp, "set yrange [0:%d]\n", N);
	fprintf(fp, "set zrange [0:%d]\n", (int)U);
	fprintf(fp, "do for [i=0:%d]{\n", ntime - 1);
	fprintf(fp, "splot 'output.txt' index i using 1:2:3 with lines\npause(0.1)}\n");

	for (k = 1; k < ntime; k++) {
		pthread_barrier_wait(&barr2);
		memcpy(prev, cur, sizeof(double) * M * N);
		memset(cur, 0, M * N);
		pthread_barrier_wait(&barr1);
		for (i = 0; i < M; i++)
			for (j = 0; j < N; j++) {
				fprintf(fd, "%d\t%d\t%f\n", i, j, cur[i * N + j]);
			}
		fprintf(fd, "\n\n");
	}
	fclose(fd);
	pclose(fp);

	done = 1;
	gettimeofday(&tv2, &tz);
	dtv.tv_sec = tv2.tv_sec - tv1.tv_sec;
	dtv.tv_usec = tv2.tv_usec - tv1.tv_usec;
	printf("%d s %ld ms\n", (int)dtv.tv_sec, dtv.tv_usec);
	return 0;
}