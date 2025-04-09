#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * Funkcja 'read_end' powinna:
 *  - otworzyc plik o nazwie przekazanej w argumencie
 *    'file_name' w trybie tylko do odczytu,
 *  - przeczytac ostatnie 8 bajtow tego pliku i zapisac
 *    wynik w argumencie 'result'.
 */
void read_end(char *file_name, char *result) {
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    // Przesuwamy się na 8 bajtów przed koniec pliku
    if (lseek(fd, -8, SEEK_END) == -1) {
        perror("lseek");
        close(fd);
        return;
    }

    // Czytamy 8 bajtów do bufora result
    ssize_t bytes_read = read(fd, result, 8);
    if (bytes_read != 8) {
        perror("read");
    }

    close(fd);
}



int main(int argc, char *argv[]) {
    char result[8];

    if (argc < 2) return -1;
    read_end(argv[1], (char *) result);
    printf("%s\n", result);
    return 0;
}
