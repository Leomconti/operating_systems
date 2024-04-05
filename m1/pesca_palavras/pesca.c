#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_WORDS 20
#define MAX_WORD_LENGTH 500
#define DIRECTIONS 8
#define MAX_FOUND_WORDS 20

typedef struct {
    int x;
    int y;
    char *directionName;
} Direction;

typedef struct {
    int wordI;
    int row;
    int col;
    char *direction;
} foundResult;

typedef struct{
    char **matrix;
    int ROW;
    int COL;
    char *word;
} searchWordArgs;

typedef struct{
    int x;
    int y;
    char **matrix;
    int maxX;
    int maxY;
    char *word;
    int currX;
    int currY;
    int wordLength;
    char *directionName;
} findInDirectionArgs;


typedef struct {
    char word[MAX_WORD_LENGTH];
    int row;
    int col;
    char direction[MAX_WORD_LENGTH];
} FoundWordDetail;

FoundWordDetail foundWords[MAX_FOUND_WORDS];
int foundWordCount = 0;
pthread_mutex_t foundWordsMutex;

// Linha / Coluna que esta sendo explorada
// String com a direcao que esse representa, para usarmos no results 
Direction directions[] = {
    { -1,  0, "cima"},
    {  1,  0, "baixo"},
    {  0,  1, "direita"},
    {  0, -1, "esquerda"},
    { -1, -1, "cima/esquerda"},
    { -1,  1, "cima/direita"},
    {  1, -1, "baixo/esquerda"}, 
    {  1,  1, "baixo/direita"}
};

void *findInDirection(void *args){
    findInDirectionArgs *parsedArgs = (findInDirectionArgs *)args;

    int x = parsedArgs->x;
    int y = parsedArgs->y;
    char **matrix = parsedArgs->matrix;
    int maxX = parsedArgs->maxX;
    int maxY = parsedArgs->maxY;
    char *word = parsedArgs->word;
    int currX = parsedArgs->currX;
    int currY = parsedArgs->currY;
    int wordLength = parsedArgs->wordLength;
    char *directionName = parsedArgs->directionName;

    int lookX = currX+x;
    int lookY = currY+y;
    int wordI = 1;

    while (wordI<wordLength){
        if (lookX >= maxX || lookY >= maxY || lookX < 0 || lookY < 0){
            return NULL;
        }
        if (matrix[lookX][lookY] == word[wordI]) {
            wordI++;
            lookX += x;
            lookY += y;
        }
        else {
            return NULL;
        }
    }

    if (wordI == wordLength) {
        pthread_mutex_lock(&foundWordsMutex);

        if (foundWordCount < MAX_FOUND_WORDS) {
            // No capitalization, just storing the found word details.
            strcpy(foundWords[foundWordCount].word, word);
            foundWords[foundWordCount].row = currX;
            foundWords[foundWordCount].col = currY;
            strcpy(foundWords[foundWordCount].direction, directionName);
            foundWordCount++;
        }
        pthread_mutex_unlock(&foundWordsMutex);

        foundResult *result = malloc(sizeof(foundResult));
        if (result != NULL) {
            result->row = currX;
            result->col = currY;
            result->direction = directionName;
            return result;
        }
    }

    return NULL;
}


void *exploreDirection(char **matrix, int ROW, int COL, char *word, int startX, int startY, int wordLength) {
    pthread_t directionThreads[DIRECTIONS];
    findInDirectionArgs directionArgs[DIRECTIONS];

    for (int i = 0; i < DIRECTIONS; i++) {
        directionArgs[i].x = directions[i].x;
        directionArgs[i].y = directions[i].y;
        directionArgs[i].matrix = matrix;
        directionArgs[i].maxX = ROW;
        directionArgs[i].maxY = COL;
        directionArgs[i].word = word;
        directionArgs[i].currX = startX;
        directionArgs[i].currY = startY;
        directionArgs[i].wordLength = wordLength;
        directionArgs[i].directionName = directions[i].directionName;

        pthread_create(&directionThreads[i], NULL, findInDirection, (void *)&directionArgs[i]);
    }

    for (int i = 0; i < DIRECTIONS; i++) {
        void *status;
        pthread_join(directionThreads[i], &status);
        foundResult *result = (foundResult *)status;

        if (result) {
            printf("Word found at [%d, %d] heading %s.\n", result->row, result->col, result->direction);
            free(result);
        }
    }
    return NULL;
}


void *searchWord(void *args){
    searchWordArgs *parsedArgs = (searchWordArgs *)args;
    char **matrix = parsedArgs->matrix;
    int ROW = parsedArgs->ROW;
    int COL = parsedArgs->COL;
    char *word = parsedArgs->word;
    int wordLength = strlen(word);

    // The problem of creating threads here is that it could get up to 
    // ROW * COL threads, so we would need a thread pool, or something to control how many threads
    // We have at a time, like letting 8 threads run at a time, like batching them
    for (int i=0; i<ROW; i++){
        for (int j=0; j<COL; j++){
            if (matrix[i][j] == word[0]){
                // printf("Starting search from [%d, %d]\n", i, j);
                exploreDirection(matrix, ROW, COL, word, i, j, wordLength);
            }
        }
    }
    printf("Finished searching for word %s\n", word);
}


char** readMatrixFromFile(FILE* file, int* ROW, int* COL) {
    // Reads the matrix from teh file based on how many ROW / COL it has.
    // It will get the ROW and COl based on regex and then read the matrix.
    // Reading the matrix then is done by reading each character in an row col loop.
    fscanf(file, "%d %d\n", ROW, COL); 
    char **matrix = malloc(*ROW * sizeof(char *));
    for (int i = 0; i < *ROW; i++) {
        matrix[i] = malloc(*COL * sizeof(char));
        for (int j = 0; j < *COL; j++) {
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
    char** words = malloc(MAX_WORDS * sizeof(char*));
    *wordCount = 0;
    while (fscanf(file, "%s\n", tempWord) == 1 && *wordCount < MAX_WORDS) {
        words[*wordCount] = malloc(strlen(tempWord) + 1);
        strcpy(words[*wordCount], tempWord);
        (*wordCount)++;
    }
    return words;
}


int main(int argc, char *argv[]) {
    FILE *file;
    char* filename;
    int ROW, COL;
    pthread_mutex_init(&foundWordsMutex, NULL);


    if (argc != 2){
        printf("Incorrect usage, missing filename argument\n");
        printf("Usage example: %s filename.txt\n", argv[0]);
        return 1;
    }
    printf("Properly Started the code\n");

    filename = argv[1];
    printf("Filename: %s\n", filename);

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Could not open file\n");
        return 1;
    }

    printf("Opened the file\n");
    char **matrix = readMatrixFromFile(file, &ROW, &COL);
    printf("Read the matrix\n");

    int wordCount;
    char** words = readWordsFromFile(file, &wordCount);
    printf("Read the words\n");

    fclose(file);

    for(int i = 0; i < wordCount; i++) {
        printf("%s\n", words[i]);
    }

    // Create one thread for each word
    pthread_t searchThreads[wordCount];
    searchWordArgs argsSearch[wordCount];

    for (int i=0; i<wordCount; i++){
        argsSearch[i].matrix = matrix;
        argsSearch[i].ROW = ROW;
        argsSearch[i].COL = COL;
        argsSearch[i].word = words[i];

        printf("Initial thread for %s\n", words[i]);
        pthread_create(&searchThreads[i], NULL, searchWord, &argsSearch[i]);
        printf("Thread %d created\n", i);
    }

    for (int i = 0; i < wordCount; i++) {
        pthread_join(searchThreads[i], NULL);
    }

    // Write the restults to a file
    FILE *resultFile = fopen("result.txt", "w");
    if (resultFile == NULL) {
        perror("Failed to open result.txt for writing");
        return 1;
    }

    // Write the modified matrix to the file
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COL; j++) {
            fprintf(resultFile, "%c", matrix[i][j]);
        }
        fprintf(resultFile, "\n");
    }

    // Write the details of the found words below the matrix
    for (int i = 0; i < foundWordCount; i++) {
        fprintf(resultFile, "%s found at [%d, %d] heading %s.\n", 
                foundWords[i].word, foundWords[i].row, foundWords[i].col, foundWords[i].direction);
    }

    // Close the file
    fclose(resultFile);

    // Free resources allocated previously:
    for (int i = 0; i < wordCount; i++) {
        free(words[i]);
    }
    free(words);

    for (int i = 0; i < ROW; i++) {
        free(matrix[i]);
    }
    free(matrix);
    pthread_mutex_destroy(&foundWordsMutex);
    return 0;
}
