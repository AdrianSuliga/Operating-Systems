#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>

int main (int l_param, char * wparam[]){
  int i;
  int tab[20]={1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};  
/*
1) otworz biblioteke
2) przypisz wskaznikom f1 i f2 adresy funkcji z biblioteki sumuj i srednia
3) stworz Makefile kompilujacy biblioteke 'bibl1' ladowana dynamicznie oraz kompilujacy ten program
4) Stosowne pliki powinny znajdowac sie w folderach '.', './bin', './'lib'. Nalezy uzyc: LD_LIBRARY_PATH
*/

  void *handle = dlopen("bibl1.so", RTLD_LAZY);
  if (!handle) {
    printf("Could not open shared library\n");
    return 1;
  }

  double (*f1)(int*, int);
  int (*f2)(int*, int);

  f1 = (double (*)(int*, int))dlsym(handle, "srednia");
  f2 = (int (*)(int*, int))dlsym(handle, "sumuj");

  for (i=0; i<5; i++) {
    printf("Wynik: %lf, %d\n", f1(tab+i, 20-i), f2(tab+i, 20-i));
  }
  
  dlclose(handle);

  return 0;
}
