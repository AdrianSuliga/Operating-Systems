#include <stdio.h>
#include "collatz.h"

int collatz_conjecture(int input)
{
    if (input % 2 == 0) {
        return input / 2;
    } else {
        return 3 * input + 1;
    }
}

int test_collatz_convergence(int input, int max_iter, int *steps)
{
    int iter = 0;
    steps[iter] = input;
    iter++;

    while (input != 1 && iter < max_iter) {
        input = collatz_conjecture(input);
        steps[iter] = input;
        iter++;
    }

    if (input != 1) {
        steps = NULL;
        return 0;
    }

    return iter - 1;
}