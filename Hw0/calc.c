#include <stdio.h>
#include <string.h>

int main() {
    char buffer[11]; // Maximum input length of 10 characters + 1 for null terminator

    while (1) {
        // Read input from the user
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            // End of input, break the loop
            break;
        }

        // Remove potential newline at the end of the buffer
        buffer[strcspn(buffer, "\n")] = 0;

        // Check if the input is "exit"
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        int FirstNum, SecondNum, result;
        char op;

        // Check if the input matches the expected format
        if (sscanf(buffer, "%d %c %d", &FirstNum, &op, &SecondNum) == 3) {
            // Ensure the numbers are single-digit natural numbers
            if (FirstNum >= 0 && FirstNum <= 9 && SecondNum >= 0 && SecondNum <= 9) {
                switch (op) {
                case '+':
                    result = FirstNum + SecondNum;
                    break;
                case '-':
                    result = FirstNum - SecondNum;
                    break;
                case '*':
                    result = FirstNum * SecondNum;
                    break;
                case '/':
                    result = FirstNum / SecondNum;
                    break;
                default:
                    printf("%s\n", buffer); // Print the original input if operator is invalid
                    continue; // Skip to next iteration
                }
                printf("%d\n", result); // Print the result
            }
            else {
                printf("%s\n", buffer); // Print the original input if numbers are not single-digit
            }
        }
        else {
            printf("%s\n", buffer); // Print the original input if the format is incorrect
        }
    }

    return 0;
}
