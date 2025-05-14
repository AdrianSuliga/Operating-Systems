#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int sum_file(char *input)
{
    int file = open(input, O_RDONLY);
    char buffor;
    int sum = 0;

    while (read(file, &buffor, sizeof(buffor))) {
        sum += (int)buffor;
    }

    close(file);
    return sum;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Expected 2 arguments, but got %d", argc - 1);
        return 0;
    }

    char *input_path = argv[1];
    char *output_path = argv[2];

    int start_offset = -1;
    int control_sum = sum_file(input_path);

    int input_file = open(input_path, O_RDONLY);
    int output_file = open(output_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    while (lseek(input_file, start_offset, SEEK_END) >= 0) {
        char buffer;
        if (read(input_file, &buffer, sizeof(buffer)) <= 0) {
            printf("Could not read from file\n");
        } 
        write(output_file, &buffer, sizeof(buffer));
        start_offset--;
    }

    close(input_file);
    close(output_file);

    printf("Input check sum: %d\nOutput check sum: %d\n", control_sum, sum_file(output_path));

    return 0;
}