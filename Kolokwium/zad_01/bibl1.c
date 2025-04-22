#include <stdlib.h>
#include <stdio.h>
#include "bibl1.h"

/*napisz biblioteke ladowana dynamicznie przez program zawierajaca funkcje:*/

//1) zliczajaca sume n elementow tablicy tab:
int sumuj(int *tab, int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += tab[i];
    }
    return sum;
}

//2) wyznaczajaca mediane n elementow tablicy tab
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

double mediana(int *tab, int n) {
    int *copy = malloc(n * sizeof(int));
    if (copy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        copy[i] = tab[i];
    }

    qsort(copy, n, sizeof(int), compare);

    double result;
    if (n % 2 == 1) {
        result = (double)copy[n / 2];
    } else {
        result = ((double)copy[n / 2] + (double)copy[n / 2 - 1]) / 2.0;
    }

    free(copy);
    return result;
}