#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>

int IsPrime(int number) {
    if (number <= 1) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    for (int i = 3; i * i <= number; i += 2) {
        if (number % i == 0) return 0;
    }
    return 1;
}

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

int main() {
    char pid_str[16];
    int_to_string(getpid(), pid_str);

    char shm_name[64];
    strcpy(shm_name, "/shm_");
    strcat(shm_name, pid_str);

    char sem_name[64];
    strcpy(sem_name, "/sem_");
    strcat(sem_name, pid_str);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        const char error_message[] = "shm_open failed\n";
        write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        const char error_message[] = "ftruncate failed\n";
        write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
        exit(1);
    }

    int* shared_num = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_num == MAP_FAILED) {
        const char error_message[] = "mmap failed\n";
        write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
        exit(1);
    }

    sem_t* sem = sem_open(sem_name, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        const char error_message[] = "sem_open failed\n";
        write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
        exit(1);
    }

    char fileName[256];
    const char prompt[] = "Enter filename: ";
    write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);

    int index = 0;
    char character;
    while (read(STDIN_FILENO, &character, 1) == 1 && character != '\n') {
        fileName[index++] = character;
    }
    fileName[index] = '\0';

    pid_t processId = fork();
    if (processId == 0) {
        execl("./child", "child", fileName, shm_name, sem_name, NULL);
        const char exec_error[] = "exec failed\n";
        write(STDERR_FILENO, exec_error, sizeof(exec_error) - 1);
        exit(1);
    } else if (processId > 0) {
        int number = 0;
        int sign = 1;
        char inputChar;

        while (read(STDIN_FILENO, &inputChar, 1) == 1) {
            if (inputChar == '-') {
                sign = -1;
            } else if (inputChar >= '0' && inputChar <= '9') {
                number = number * 10 + (inputChar - '0');
            } else if (inputChar == '\n') {
                number *= sign;

                *shared_num = number;
                sem_post(sem);

                if (number < 0 || IsPrime(number)) {
                    break;
                }
                number = 0;
                sign = 1;
            }
        }

        wait(NULL);

        munmap(shared_num, sizeof(int));
        sem_close(sem);
        shm_unlink(shm_name);
        sem_unlink(sem_name);
    } else {
        const char fork_error[] = "fork failed\n";
        write(STDERR_FILENO, fork_error, sizeof(fork_error) - 1);
        exit(1);
    }

    return 0;
}