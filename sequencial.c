#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int i[21];    // processing times per machine
  int exec[21]; // execution progress per machine
  int maq;      // current machine index (machine assigned to task so far)
} task;

int n, m;
task tasks[101]; // global tasks array (only first n used)

// Helper function: swap two integers.
void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

// Compute factorial of n (assumes n is small).
int factorial(int n) {
  int result = 1;
  for (int i = 2; i <= n; i++)
    result *= i;
  return result;
}

// Recursive function to generate all permutations of arr[start..n-1].
// "perms" is an array (of size at least factorial(n)) to hold each complete
// permutation, and perm_count tracks how many permutations have been stored.
void generate_permutations(int *arr, int start, int n, int **perms,
                           int *perm_count) {
  if (start == n - 1) {
    int *perm = (int *)malloc(n * sizeof(int));
    memcpy(perm, arr, n * sizeof(int));
    perms[*perm_count] = perm;
    (*perm_count)++;
    return;
  }
  for (int i = start; i < n; i++) {
    swap(&arr[start], &arr[i]);
    generate_permutations(arr, start + 1, n, perms, perm_count);
    swap(&arr[start], &arr[i]); // backtrack
  }
}

// Function to simulate the schedule for a given permutation.
// Returns the makespan computed for the permutation.
int simulate_permutation(int *perm, int n, int m, task tasks_global[]) {
  int local_machines[21];
  task local_tasks[101];

  // Copy only the first n tasks from the global array.
  memcpy(local_tasks, tasks_global, n * sizeof(task));

  // Initialize machines array.
  for (int i = 0; i < m; i++) {
    local_machines[i] = -1;
  }

  // Reset each task's execution counters and machine index.
  for (int i = 0; i < n; i++) {
    memset(local_tasks[i].exec, 0, m * sizeof(int));
    local_tasks[i].maq = 0;
  }

  int cont_n = 0;
  int makespan = 0;

  // Simulation loop: continue until all n tasks have finished processing.
  while (cont_n < n) {
    // Schedule tasks on available machines.
    for (int i = cont_n; i < n; i++) {
      int task_index = perm[i];
      int current_machine = local_tasks[task_index].maq;
      // If the machine is free, assign task to it.
      if (current_machine < m && local_machines[current_machine] < 0) {
        local_machines[current_machine] = task_index;
      }
    }

    // Run one time unit of processing on each machine.
    for (int i = 0; i < m; i++) {
      if (local_machines[i] >= 0) {
        int t_index = local_machines[i];
        local_tasks[t_index].exec[i]++; // simulate processing time on machine i
        if (local_tasks[t_index].exec[i] >= local_tasks[t_index].i[i]) {
          // Task finishes on this machine; move it to the next.
          local_tasks[t_index].maq++;
          if (local_tasks[t_index].maq >= m)
            cont_n++;             // task finished all machines
          local_machines[i] = -1; // free up the machine
        }
      }
    }
    makespan++;
  }
  return makespan;
}

int main(int argc, char *argv[]) {
  FILE *in = fopen("inputs/pfs.in", "r");
  FILE *out = fopen("inputs/pfs.out", "w");
  if (!in || !out) {
    perror("Failed to open input or output file");
    return EXIT_FAILURE;
  }

  while (1) {
    // Reset tasks structure.
    memset(tasks, 0, sizeof(tasks));

    fscanf(in, "%d%d", &n, &m);
    if (n == 0 || m == 0)
      break;

    // Read processing times for each task and each machine.
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < m; j++) {
        fscanf(in, "%d", &tasks[i].i[j]);
      }
    }

    // Create the initial permutation [0, 1, ..., n-1].
    int *init_perm = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
      init_perm[i] = i;
    }

    // Compute total number of permutations (n!).
    int num_perms = factorial(n);
    int **perms = (int **)malloc(num_perms * sizeof(int *));
    int perm_count = 0;
    generate_permutations(init_perm, 0, n, perms, &perm_count);
    free(init_perm);

    int global_min_makespan = INT_MAX;

    for (int p = 0; p < perm_count; p++) {
      int curr_makespan = simulate_permutation(perms[p], n, m, tasks);
      // The reduction clause automatically takes the minimum.
      if (curr_makespan < global_min_makespan)
        global_min_makespan = curr_makespan;
    }

    // Free allocated memory for permutations.
    for (int p = 0; p < perm_count; p++) {
      free(perms[p]);
    }
    free(perms);

    fprintf(out, "%d\n", global_min_makespan);
    fflush(out);
  }

  fclose(in);
  fclose(out);
  return EXIT_SUCCESS;
}
