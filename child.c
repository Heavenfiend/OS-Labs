#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>

void int_to_string(int n, char* buffer) {
    if (n == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int temp = n;
    int len = 0;
    if (temp < 0) {
        buffer[0] = '-';
        temp = -temp;
        len = 1;
    }

    int start = len;
    while (temp > 0) {
        buffer[len++] = (temp % 10) + '0';
        temp /= 10;
    }
    buffer[len] = '\0';

    int end = len - 1;
    while (start < end) {
        char t = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = t;
        start++;
        end--;
    }
}

int IsPrime(int number) {
    if (number <= 1) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    for (int i = 3; i * i <= number; i += 2) {
        if (number % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        const char usage_message[] = "Usage: child <output_file> <shm_name> <sem_name>\n";
        write(STDERR_FILENO, usage_message, sizeof(usage_message) - 1);
        exit(1);
    }

    const char* output_file = argv[1];
    const char* shm_name = argv[2];
    const char* sem_name = argv[3];

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        const char open_error[] = "shm_open failed\n";
        write(STDERR_FILENO, open_error, sizeof(open_error) - 1);
        exit(1);
    }

    int* shared_num = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_num == MAP_FAILED) {
        const char mmap_error[] = "mmap failed\n";
        write(STDERR_FILENO, mmap_error, sizeof(mmap_error) - 1);
        exit(1);
    }

    sem_t* sem = sem_open(sem_name, 0);
    if (sem == SEM_FAILED) {
        const char sem_error[] = "sem_open failed\n";
        write(STDERR_FILENO, sem_error, sizeof(sem_error) - 1);
        exit(1);
    }

    int fileDescriptor = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor == -1) {
        const char open_error[] = "open failed\n";
        write(STDERR_FILENO, open_error, sizeof(open_error) - 1);
        exit(1);
    }

    while (1) {
        sem_wait(sem);

        int number = *shared_num;

        // Проверяем, нужно ли завершить, **до** записи
        if (number < 0 || IsPrime(number)) {
            break;
        }

        // Преобразуем число в строку
        char buffer[32];
        int temp = number;
        int index = 0;

        if (temp == 0) {
            buffer[index++] = '0';
        } else {
            if (temp < 0) {
                buffer[index++] = '-';
                temp = -temp;
            }
            int start = index;
            while (temp > 0) {
                buffer[index++] = (temp % 10) + '0';
                temp /= 10;
            }
            int end = index - 1;
            while (start < end) {
                char t = buffer[start];
                buffer[start] = buffer[end];
                buffer[end] = t;
                start++;
                end--;
            }
        }

        buffer[index++] = '\n';
        write(fileDescriptor, buffer, index);
    }

    close(fileDescriptor);
    munmap(shared_num, sizeof(int));
    sem_close(sem);
    return 0;
}