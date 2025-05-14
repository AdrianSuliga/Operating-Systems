#include <stdio.h>
#include <stdlib.h>

#ifdef DYNAMIC
#include <dlfcn.h>
#else
#include "lib/collatz.h"
#endif

#define ARR_MAX_LEN 100

int main(int argc, char **argv)
{
    int (*collatz_function)(int, int, int*);
    #ifdef DYNAMIC
    void *handle = dlopen("libcollatz.so", RTLD_LAZY);
    if (!handle) {
        printf("Could not open shared library\n");
        return 0;
    }
    collatz_function = (int (*)(int, int, int*))dlsym(handle, "test_collatz_convergence");
    #else
    collatz_function = &test_collatz_convergence;
    #endif

    int *numbers_to_test;
    int steps[ARR_MAX_LEN];

    if (argc == 1) {
        numbers_to_test = malloc(sizeof(int));
        numbers_to_test[0] = 31;
    } else {
        numbers_to_test = malloc((argc - 1) * sizeof(int));
        for (int i = 1; i < argc; i++) {
            numbers_to_test[i - 1] = atoi(argv[i]);
        }
    }

    if (argc > 1) {
        argc--;
    }

    for (int i = 0; i < argc; i++) {
        int number = numbers_to_test[i];
        printf("Trying to converge %d\n", number);

        number = collatz_function(number, ARR_MAX_LEN, steps);

        if (number != 0) {
            printf("Number converged in %d steps\n", number);
            for (int i = 0; i < number + 1; i++) {
                printf("%d ", steps[i]);
            }
            printf("\n");
        } else {
            printf("Could not converge number in given amount of steps\n");
        }
    }

    #ifdef DYNAMIC
    dlclose(handle);
    #endif

    free(numbers_to_test);

    return 0;
}