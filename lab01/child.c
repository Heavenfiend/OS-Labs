#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

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

int main(int argumentCount, char* argumentValues[]) {
    if (argumentCount != 2) {
        const char usage_message[] = "Usage: child <output_file>\n";
        write(STDERR_FILENO, usage_message, sizeof(usage_message) - 1);
        exit(1);
    }

    int fileDescriptor = open(argumentValues[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor == -1) {
        const char open_error[] = "open failed\n";
        write(STDERR_FILENO, open_error, sizeof(open_error) - 1);
        exit(1);
    }

    int number;
    while (read(STDIN_FILENO, &number, sizeof(number)) == sizeof(number)) {
        if (number < 0 || IsPrime(number)) {
            close(fileDescriptor);
            exit(0);
        }

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
            // Reverse digits
            int end = index - 1;
            while (start < end) {
                char temp_char = buffer[start];
                buffer[start] = buffer[end];
                buffer[end] = temp_char;
                start++;
                end--;
            }
        }

        buffer[index++] = '\n';
        write(fileDescriptor, buffer, index);
    }

    close(fileDescriptor);
    return 0;
}
