#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

// Include this for embedded wordlist
#include "genpass.h"

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 1000
#define SYMBOLS "!@#$%^&*=+?-"

// Function prototypes
void usage(void);
char *get_random_symbols(int count);
char *get_random_numbers(int count);
char *get_random_suffix(void);
int load_words(const char *filename, char words[][MAX_WORD_LENGTH], int max_words);
void shuffle(int *array, int n);
char *build_password(char *words[], int word_count, int symbol_count, int num_count, int use_suffix);

int main(int argc, char *argv[]) {
    // Default settings
    int word_count = 5 + (rand() % 3);
    int symbol_count = 0;
    int num_count = 0;
    int use_suffix = 1;

    // Seed random number generator
    srand(time(NULL));

    // Parse command line arguments
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--words") == 0) {
            if (i + 1 < argc) {
                word_count = atoi(argv[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Error: Missing argument for %s\n", argv[i]);
                usage();
                return 1;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--symbols") == 0) {
            if (i + 1 < argc) {
                symbol_count = atoi(argv[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Error: Missing argument for %s\n", argv[i]);
                usage();
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numbers") == 0) {
            if (i + 1 < argc) {
                num_count = atoi(argv[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Error: Missing argument for %s\n", argv[i]);
                usage();
                return 1;
            }
        } else if (strcmp(argv[i], "--no-suffix") == 0) {
            use_suffix = 0;
            i++;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage();
            return 1;
        }
    }

    int word_count_in_file;
    char word_list[MAX_WORDS][MAX_WORD_LENGTH];

#ifdef USE_EMBEDDED_WORDLIST
    // Use the embedded wordlist
    word_count_in_file = EMBEDDED_WORDLIST_SIZE;
    if (word_count_in_file > MAX_WORDS) {
        word_count_in_file = MAX_WORDS;
    }

    for (i = 0; i < word_count_in_file; i++) {
        strncpy(word_list[i], embedded_wordlist[i], MAX_WORD_LENGTH - 1);
        word_list[i][MAX_WORD_LENGTH - 1] = '\0';
    }
#else
    // Get path to words file
    char words_file[256];
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set\n");
        return 1;
    }

    snprintf(words_file, sizeof(words_file), "%s/.local/bin/words.txt", home);

    // Load words from file
    word_count_in_file = load_words(words_file, word_list, MAX_WORDS);

    if (word_count_in_file == 0) {
        fprintf(stderr, "Error: Required wordlist not found or empty: %s\n", words_file);
        return 1;
    }
#endif

    // Select random words
    int indices[MAX_WORDS];
    for (i = 0; i < word_count_in_file; i++) {
        indices[i] = i;
    }

    shuffle(indices, word_count_in_file);

    // Use only as many words as we need
    if (word_count > word_count_in_file) {
        word_count = word_count_in_file;
        fprintf(stderr, "Warning: Only %d words available\n", word_count);
    }

    // Create array of selected words
    char *selected_words[word_count];
    for (i = 0; i < word_count; i++) {
        selected_words[i] = word_list[indices[i]];

        // Capitalize first letter of each word
        if (strlen(selected_words[i]) > 0) {
            selected_words[i][0] = toupper(selected_words[i][0]);
        }
    }

    // Build and output password
    char *password = build_password(selected_words, word_count, symbol_count, num_count, use_suffix);
    printf("%s\n", password);

    free(password);
    return 0;
}

void usage(void) {
    printf("Usage: genpass [OPTIONS]\n");
    printf("Generates a secure password using random words and symbols\n\n");
    printf("Options:\n");
    printf("    -w, --words NUM     Number of words (default: random 5-7)\n");
    printf("    -s, --symbols NUM   Number of symbols to add between words\n");
    printf("    -n, --numbers NUM   Number of additional numbers to add between words\n");
    printf("    --no-suffix        Don't add default trailing numbers\n");
    printf("    -h, --help         Show this help message\n\n");
    printf("Example outputs:\n");
    printf("    genpass            => Castle_Wizard_Storm_Dragon_Forest757\n");
    printf("    genpass -s 2       => Castle#Wizard_Storm@Dragon_Forest333\n");
    printf("    genpass -n 2 -s 2  => Castle#Wizard42@Storm_Dragon%%Forest989\n");
}

// Generate random symbols
char *get_random_symbols(int count) {
    char *result = (char *)malloc(count + 1);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    const char *symbols = SYMBOLS;
    int symbols_len = strlen(symbols);

    for (int i = 0; i < count; i++) {
        int idx = rand() % symbols_len;
        result[i] = symbols[idx];
    }
    result[count] = '\0';

    return result;
}

// Generate random numbers (1-99) for between words
char *get_random_numbers(int count) {
    if (count <= 0) {
        char *result = (char *)malloc(1);
        if (result == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        result[0] = '\0';
        return result;
    }

    // Allocate space for the string
    // Each number can be 1-2 digits, so allocate conservatively
    char *result = (char *)malloc(count * 3);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Generate the numbers
    char *current = result;
    for (int i = 0; i < count; i++) {
        int num = 1 + (rand() % 99);

        // Write the number to the string
        if (num < 10) {
            // Single digit
            *current++ = '0' + num;
        } else {
            // Two digits
            *current++ = '0' + (num / 10);
            *current++ = '0' + (num % 10);
        }
    }
    *current = '\0';

    return result;
}

// Generate random 3-digit suffix
char *get_random_suffix(void) {
    char *result = (char *)malloc(4);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    sprintf(result, "%03d", rand() % 1000);
    return result;
}

// Load words from file into array
int load_words(const char *filename, char words[][MAX_WORD_LENGTH], int max_words) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }

    int count = 0;
    while (count < max_words && fgets(words[count], MAX_WORD_LENGTH, file) != NULL) {
        // Remove newline character
        char *newline = strchr(words[count], '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        // Skip empty lines
        if (strlen(words[count]) > 0) {
            count++;
        }
    }

    fclose(file);
    return count;
}

// Fisher-Yates shuffle algorithm
void shuffle(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Build password with random separators
char *build_password(char *words[], int word_count, int symbol_count, int num_count, int use_suffix) {
    // Calculate maximum size needed
    int max_size = word_count * MAX_WORD_LENGTH + (word_count - 1) + symbol_count + num_count * 2 + 4;

    char *password = (char *)malloc(max_size);
    if (password == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    password[0] = '\0';

    // Calculate maximum possible separators
    int max_separators = word_count - 1;
    int total_separators = symbol_count + num_count;

    // Don't add more separators than spaces between words
    if (total_separators > max_separators) {
        int excess = total_separators - max_separators;
        fprintf(stderr, "Warning: Can only fit %d separators between %d words. Ignoring %d.\n",
                max_separators, word_count, excess);
        symbol_count -= (excess + 1) / 2;
        num_count -= excess / 2;
        total_separators = max_separators;
    }

    // Get symbols and numbers
    char *symbols = get_random_symbols(symbol_count);
    char *numbers = get_random_numbers(num_count);

    // Create array of separator positions (0 to max_separators-1)
    int *separator_positions = (int *)malloc(max_separators * sizeof(int));
    if (separator_positions == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < max_separators; i++) {
        separator_positions[i] = i;
    }

    // Shuffle positions to randomly assign special separators
    if (total_separators > 0) {
        shuffle(separator_positions, max_separators);
    }

    // Build password
    int symbol_idx = 0;
    int number_idx = 0;

    for (int i = 0; i < word_count; i++) {
        // Add word
        strcat(password, words[i]);

        // Add separator if not the last word
        if (i < word_count - 1) {
            int separator_type = -1;  // -1 = default, 0 = symbol, 1 = number

            // Check if this position should get a special separator
            for (int j = 0; j < total_separators; j++) {
                if (separator_positions[j] == i) {
                    // Decide between symbol and number
                    if (j < symbol_count) {
                        separator_type = 0;  // symbol
                    } else {
                        separator_type = 1;  // number
                    }
                    break;
                }
            }

            // Add the appropriate separator
            if (separator_type == 0 && symbol_idx < symbol_count) {
                // Add symbol
                char sep[2] = {symbols[symbol_idx++], '\0'};
                strcat(password, sep);
            } else if (separator_type == 1 && number_idx < strlen(numbers)) {
                // Add number (either 1 or 2 digits)
                if (number_idx + 1 < strlen(numbers)) {
                    // Try for 2 digits
                    char num_str[3] = {numbers[number_idx], numbers[number_idx+1], '\0'};
                    strcat(password, num_str);
                    number_idx += 2;
                } else {
                    // Just 1 digit left
                    char num_str[2] = {numbers[number_idx], '\0'};
                    strcat(password, num_str);
                    number_idx++;
                }
            } else {
                // Default separator
                strcat(password, "_");
            }
        }
    }

    // Add default 3-digit suffix unless disabled
    if (use_suffix) {
        char *suffix = get_random_suffix();
        strcat(password, suffix);
        free(suffix);
    }

    // Free memory
    free(symbols);
    free(numbers);
    free(separator_positions);

    return password;
}
