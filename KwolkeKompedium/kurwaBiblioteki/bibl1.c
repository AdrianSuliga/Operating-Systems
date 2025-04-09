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

//2) liczaca srednia n elementow tablicy tab
double srednia(int *tab, int n)
{
    return sumuj(tab, n) / n;
}


