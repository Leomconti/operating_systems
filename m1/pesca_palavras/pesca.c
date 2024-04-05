#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

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

void *findInDirection(void *args) {
    // Parse args
    findInDirectionArgs *a = (findInDirectionArgs *)args;
    
    int maxY = a->maxY;
    int maxX = a->maxX;
    int x = a->x;
    int y = a->y;
    int wordLength = a->wordLength;
    int currX = a->currX;
    int currY = a->currY;
    char *word = a->word;
    char **matrix = a->matrix;
    char *directionName = a->directionName;

    int lookX, lookY;

    // Iterate over each character in the word, checking if it matches the matrix at the expected position.
    for (int i = 1; i < wordLength; ++i) {
        // If the current position is out of bounds or the character does not match, return NULL.
        if (lookX < 0 || lookX >= maxX|| lookY < 0 || lookY >= maxY || matrix[lookX][lookY] != word[i]) {
            return NULL;
        }

        // Move to the next position in the specified direction for the next iteration.
        lookX += x;
        lookY += y;
    }

    // If the loop completes, the word was found. Lock the mutex to safely update the foundWords array.
    pthread_mutex_lock(&foundWordsMutex);
    
    // Check if there's space left in the foundWords array to log another found word.
    if (foundWordCount < MAX_FOUND_WORDS) {
        // Copy the found word, its starting position, and the direction into the foundWords array.
        strcpy(foundWords[foundWordCount].word, word);
        foundWords[foundWordCount].row = currX;
        foundWords[foundWordCount].col = currY;
        strcpy(foundWords[foundWordCount].direction, directionName);
        foundWordCount++;
    }

    // Unlock the mutex after updating.
    pthread_mutex_unlock(&foundWordsMutex);

    // Since the function is used with pthread_create, it must return NULL.
    return NULL;
}

// Explores all directions from a starting point to find a word
void *exploreDirection(char **matrix, int ROW, int COL, char *word, int startX, int startY) {
    pthread_t threads[DIRECTIONS];
    findInDirectionArgs args[DIRECTIONS];

    for (int i = 0; i < DIRECTIONS; ++i) {
        // Preparacao dos argumentos enviando para a struct definida para a thread
        args[i] = (findInDirectionArgs){
            .x = directions[i].x,
            .y = directions[i].y,
            .matrix = matrix,
            .maxX = ROW, 
            .maxY = COL, 
            .word = word, 
            .currX = startX,
            .currY = startY,
            .wordLength = strlen(word),
            .directionName = directions[i].directionName
        };
        pthread_create(&threads[i], NULL, findInDirection, &args[i]);
    }

    for (int i = 0; i < DIRECTIONS; ++i) {
        pthread_join(threads[i], NULL);
    }
}

// Main function to search for each word in the matrix
void *searchWord(void *args) {
    // Parse args
    searchWordArgs *a = (searchWordArgs *)args;

    int ROW = a->ROW;
    int COL = a->COL;
    char **matrix = a->matrix;
    char *word = a->word;

    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            if (matrix[i][j] == word[0]) {
                exploreDirection(matrix, ROW, COL, word, i, j);
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

    clock_t startTime, endTime;
    double cpuTimeUsed;

    startTime = clock();


    int ROW, COL, wordCount;
    char **matrix = readMatrixFromFile(file, &ROW, &COL);
    char **words = readWordsFromFile(file, &wordCount);
    fclose(file);

    pthread_mutex_init(&foundWordsMutex, NULL);
    pthread_t searchThreads[MAX_WORDS];
    searchWordArgs searchArgs[MAX_WORDS];

    for (int i = 0; i < wordCount; ++i) {
        searchArgs[i] = (searchWordArgs){.matrix = matrix, .ROW = ROW, .COL = COL, .word = words[i]};
        pthread_create(&searchThreads[i], NULL, searchWord, &searchArgs[i]);
    }

    for (int i = 0; i < wordCount; ++i) {
        pthread_join(searchThreads[i], NULL);
    }

    // Record the end time just before cleanup starts
    endTime = clock();
    // Calculate the elapsed time in seconds
    cpuTimeUsed = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;

    printf("Execution time: %f seconds\n", cpuTimeUsed);

    // Write results to a file
    printf("Writing results to file\n");
    FILE *resultFile = fopen("result.txt", "w");
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            fprintf(resultFile, "%c", matrix[i][j]);
        }
        fprintf(resultFile, "\n");
    }
    for (int i = 0; i < foundWordCount; ++i) {
        fprintf(resultFile, "%s found at [%d, %d] heading %s.\n",
                foundWords[i].word, foundWords[i].row, foundWords[i].col, foundWords[i].direction);
    }
    fclose(resultFile);
    printf("Cleaning up memory\n");
    // Clean-up
    for (int i = 0; i < ROW; ++i) {
        free(matrix[i]);
    }
    free(matrix);
    for (int i = 0; i < wordCount; ++i) {
        free(words[i]);
    }
    free(words);
    pthread_mutex_destroy(&foundWordsMutex);
    printf("Program finished\n");

    return 0;
}
