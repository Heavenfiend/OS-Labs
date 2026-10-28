#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int IsPrime(int number) {
    if (number <= 1) {
        return 0;
    }
    if (number == 2) {
        return 1;
    }
    if (number % 2 == 0) {
        return 0;
    }
    for (int index = 3; index * index <= number; index += 2) {
        if (number % index == 0) {
            return 0;
        }
    }
    return 1;
}

int main() {
    int pipe1[2];
    if (pipe(pipe1) == -1) {
        const char error_message[] = "pipe1 failed\n";
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
        // дочерний процесс
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        execl("./child", "child", fileName, NULL);
        const char exec_error[] = "exec failed\n";
        write(STDERR_FILENO, exec_error, sizeof(exec_error) - 1);
        exit(1);
    } else if (processId > 0) {
        // родительский процесс
        close(pipe1[0]);

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

                char debug_msg[32];
                int len = 0;
                int temp = number;
                if (temp == 0) debug_msg[len++] = '0';
                else {
                    if (temp < 0) { debug_msg[len++] = '-'; temp = -temp; }
                    int start = len;
                    while (temp > 0) { debug_msg[len++] = (temp % 10) + '0'; temp /= 10; }
                    int end = len - 1;
                    while (start < end) { char t = debug_msg[start]; debug_msg[start] = debug_msg[end]; debug_msg[end] = t; start++; end--; }
                }
                debug_msg[len++] = '\n';
                write(STDERR_FILENO, "Sending: ", 9);
                write(STDERR_FILENO, debug_msg, len);

                write(pipe1[1], &number, sizeof(number));
                if (number < 0 || IsPrime(number)) {
                    break;
                }
                number = 0;
                sign = 1;
            }
        }

        close(pipe1[1]);
        wait(NULL);
    } else {
        const char fork_error[] = "fork failed\n";
        write(STDERR_FILENO, fork_error, sizeof(fork_error) - 1);
        exit(1);
    }

    return 0;
}
