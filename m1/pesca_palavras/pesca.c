#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_WORDS 20
#define MAX_WORD_LENGTH 500
#define DIRECTIONS 8
#define MAX_FOUND_WORDS 20

// Define arguments for the functions, as they are threads, and only receive one argument
// We'll send them in a struct
typedef struct {
    char **matrix;
    int ROW, COL;
    char *word;
} searchWordArgs;

typedef struct {
    int x, y, maxX, maxY, currX, currY, wordLength;
    char **matrix, *word, *directionName;
} findInDirectionArgs;


typedef struct {
    int row, col;
    char word[MAX_WORD_LENGTH], direction[MAX_WORD_LENGTH];
} FoundWordDetail;


// So we can have directions to search, and the name of them, to put in the result
typedef struct {
    int x, y;
    char *directionName;
} Direction;

Direction directions[] = {
    {-1, 0, "cima"},
    {1, 0, "baixo"},
    {0, 1, "direita"},
    {0, -1, "esquerda"},
    {-1, -1, "cima/esquerda"},
    {-1, 1, "cima/direita"},
    {1, -1, "baixo/esquerda"},
    {1, 1, "baixo/direita"}
};

// This is going to be used as a global variable, so we can store the found words
// And we create the mutex so different threads can use it in sync
FoundWordDetail foundWords[MAX_FOUND_WORDS];
int foundWordCount = 0;
pthread_mutex_t foundWordsMutex;

void findInDirection(findInDirectionArgs *a) {
    int lookX = a->currX;
    int lookY = a->currY;

    // Check if the first character matches before proceeding.
    if (a->matrix[lookX][lookY] != a->word[0]) return;

    for (int i = 1; i < a->wordLength; ++i) {
        lookX += a->x;
        lookY += a->y;
        // Check bounds and character match.
        if (lookX < 0 || lookX >= a->maxX || lookY < 0 || lookY >= a->maxY || a->matrix[lookX][lookY] != a->word[i]) {
            return;  // The word does not match, exit the function.
        }
    }

    // If the code reaches here, the word has been found.
    printf("%s - (%d,%d):%s.\n", a->word, a->currX, a->currY, a->directionName);

    // Record the finding for later writing to file
    if (foundWordCount < MAX_FOUND_WORDS) {
        strncpy(foundWords[foundWordCount].word, a->word, MAX_WORD_LENGTH);
        foundWords[foundWordCount].row = a->currX;
        foundWords[foundWordCount].col = a->currY;
        strncpy(foundWords[foundWordCount].direction, a->directionName, MAX_WORD_LENGTH);
        foundWordCount++;
    }
}


// Function to explore all directions from a starting point
void exploreDirection(char **matrix, int ROW, int COL, char *word, int startX, int startY) {
    findInDirectionArgs args;

    for (int i = 0; i < DIRECTIONS; ++i) {
        args = (findInDirectionArgs){
            .x = directions[i].x, .y = directions[i].y,
            .matrix = matrix, .maxX = ROW, .maxY = COL,
            .word = word, .currX = startX, .currY = startY,
            .wordLength = strlen(word), .directionName = directions[i].directionName
        };

        findInDirection(&args);
    }
}

// Sequential function to search for each word in the matrix
void searchWord(searchWordArgs *a) {
    for (int i = 0; i < a->ROW; ++i) {
        for (int j = 0; j < a->COL; ++j) {
            if (a->matrix[i][j] == a->word[0]) {
                exploreDirection(a->matrix, a->ROW, a->COL, a->word, i, j);
            }
        }
    }
}
// Reads the character matrix from a file
char** readMatrixFromFile(FILE* file, int* ROW, int* COL) {
    fscanf(file, "%d %d\n", ROW, COL);
    // Allocate memory for that matrix ixj
    char **matrix = (char **)malloc(*ROW * sizeof(char *));
    for (int i = 0; i < *ROW; ++i) {
        // Allocate for each row
        matrix[i] = (char *)malloc(*COL * sizeof(char));
        for (int j = 0; j < *COL; ++j) {
            // Scan character
            fscanf(file, "%c\n", &matrix[i][j]);
        }
    }
    return matrix;
}
char** readWordsFromFile(FILE* file, int* wordCount) {
    // To read the words from file we'll continue with the same file opened
    // as the pointer to the file will be at the end of the matrix, where the words start,
    // we'll read one word per line using the %s\n regex.
    char tempWord[MAX_WORD_LENGTH + 1];
    char** words = (char **)malloc(MAX_WORDS * sizeof(char*));
    *wordCount = 0;
    while (fscanf(file, "%s\n", tempWord) == 1 && *wordCount < MAX_WORDS) {
        words[*wordCount] = (char *)malloc(strlen(tempWord) + 1);
        strcpy(words[*wordCount], tempWord);
        (*wordCount)++;
    }
    return words;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Incorrect usage, missing filename argument\n");
        printf("Usage: %s filename.txt\n", argv[0]);
        return 1;
    }
    printf("Starting the program\n");
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Could not open file");
        return 1;
    }
    printf("File opened\n");

    // Get start time from real world and cpu clock
    struct timeval start, end;
    gettimeofday(&start, NULL);
    clock_t startTime, endTime;
    double cpuTimeUsed;
    startTime = clock();

    int ROW, COL, wordCount;
    char **matrix = readMatrixFromFile(file, &ROW, &COL);
    char **words = readWordsFromFile(file, &wordCount);
    fclose(file);

    pthread_t searchThreads[MAX_WORDS];
    searchWordArgs searchArgs[MAX_WORDS];

    pid_t pids[MAX_WORDS];
    int status;

    for (int i = 0; i < wordCount; ++i) {
        searchWordArgs args = {.matrix = matrix, .ROW = ROW, .COL = COL, .word = words[i]};
        searchWord(&args);
    }


    // get end time from real world and cpu clock
    gettimeofday(&end, NULL);
    endTime = clock();
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf("Real Execution Time: %ld seconds, %ld microseconds\n", seconds, micros);
    cpuTimeUsed = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;
    printf("CPU Execution time: %f seconds\n", cpuTimeUsed);

    printf("Writing found words to file\n");
    FILE *resultFile = fopen("result.txt", "w");
    if (resultFile != NULL) {
        for (int i = 0; i < ROW; i++) {
            for (int j = 0; j < COL; j++) {
                fprintf(resultFile, "%c", matrix[i][j]);
            }
            fprintf(resultFile, "\n");
        }
        for (int i = 0; i < foundWordCount; i++) {
            fprintf(resultFile, "%s - (%d, %d):%s.\n", foundWords[i].word, foundWords[i].row, foundWords[i].col, foundWords[i].direction);
        }
        fclose(resultFile);
    } else {
        perror("Could not open result.txt for writing");
    }

    return 0;
}
