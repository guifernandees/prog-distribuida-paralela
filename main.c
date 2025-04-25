#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <omp.h>

#define MAX_TASKS    100
#define MAX_MACHINES 21

typedef struct {
    int i[MAX_MACHINES];    // processing times per machine
    int exec[MAX_MACHINES]; // execution progress per machine
    int maq;                // current machine index
} task;

// global tasks array; only indices 0..n-1 used
task tasks[MAX_TASKS + 1];

int n, m;

// swap two ints
static inline void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// compute n!, but as size_t so it can hold up to 20! safely
size_t factorial(int k) {
    size_t result = 1;
    for (int i = 2; i <= k; ++i) {
        result *= (size_t)i;
    }
    return result;
}

// recursively build all permutations of arr[0..n-1]
void generate_permutations(int *arr, int start, int n,
                           int **perms, size_t *perm_count) {
    if (start == n - 1) {
        // allocate one more copy of the current perm
        int *copy = malloc((size_t)n * sizeof *copy);
        if (!copy) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memcpy(copy, arr, (size_t)n * sizeof *copy);
        perms[*perm_count] = copy;
        (*perm_count)++;
        return;
    }
    for (int i = start; i < n; i++) {
        swap(&arr[start], &arr[i]);
        generate_permutations(arr, start + 1, n, perms, perm_count);
        swap(&arr[start], &arr[i]);  // backtrack
    }
}

int simulate_permutation(int *perm, int n, int m, task tasks_global[]) {
    int local_machines[MAX_MACHINES];
    task local_tasks[MAX_TASKS];

    memcpy(local_tasks, tasks_global, (size_t)n * sizeof *local_tasks);

    // mark all machines free
    #pragma omp simd
    for (int i = 0; i < m; i++)
        local_machines[i] = -1;

    // reset each task
    for (int i = 0; i < n; i++) {
        memset(local_tasks[i].exec, 0, (size_t)m * sizeof *local_tasks[i].exec);
        local_tasks[i].maq = 0;
    }

    int finished = 0;
    int makespan = 0;
    while (finished < n) {
        // try to assign every unfinished task
        for (int idx = finished; idx < n; idx++) {
            int t = perm[idx];
            int cur = local_tasks[t].maq;
            if (cur < m && local_machines[cur] < 0)
                local_machines[cur] = t;
        }
        // advance one time‐unit
        for (int i = 0; i < m; i++) {
            int t = local_machines[i];
            if (t >= 0) {
                if (++local_tasks[t].exec[i] >= local_tasks[t].i[i]) {
                    local_tasks[t].maq++;
                    if (local_tasks[t].maq >= m)
                        finished++;
                    local_machines[i] = -1;
                }
            }
        }
        makespan++;
    }
    return makespan;
}

int main(void) {
    FILE *in  = fopen("inputs/pfs.in", "r");
    FILE *out = fopen("inputs/pfs.out", "w");
    if (!in || !out) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    while (1) {
        if (fscanf(in, "%d %d", &n, &m) != 2)
            break;
        if (n == 0 && m == 0)
            break;

        // validate bounds so that n * sizeof(int) can never overflow
        if (n < 1 || n > MAX_TASKS ||
            m < 1 || m > MAX_MACHINES)
        {
            fprintf(stderr,
                    "Invalid problem size: n must be 1..%d, m must be 1..%d\n",
                    MAX_TASKS, MAX_MACHINES);
            return EXIT_FAILURE;
        }

        // read processing times
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                fscanf(in, "%d", &tasks[i].i[j]);
            }
        }

        // initial [0,1,2,...,n-1]
        int *init_perm = malloc((size_t)n * sizeof *init_perm);
        if (!init_perm) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        for (int i = 0; i < n; i++)
            init_perm[i] = i;

        // how many perms?
        size_t num_perms = factorial(n);
        int **perms = malloc(num_perms * sizeof *perms);
        if (!perms) {
            perror("malloc");
            return EXIT_FAILURE;
        }

        size_t perm_count = 0;
        generate_permutations(init_perm, 0, n, perms, &perm_count);
        free(init_perm);

        int best = INT_MAX;
        #pragma omp parallel for reduction(min: best) schedule(dynamic)
        for (size_t p = 0; p < perm_count; p++) {
            int mspan = simulate_permutation(perms[p], n, m, tasks);
            if (mspan < best)
                best = mspan;
        }

        // cleanup
        for (size_t p = 0; p < perm_count; p++)
            free(perms[p]);
        free(perms);

        fprintf(out, "%d\n", best);
        fflush(out);
    }

    fclose(in);
    fclose(out);
    return EXIT_SUCCESS;
}
