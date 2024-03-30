#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#define MAX_WORDS 20
#define MAX_WORD_LENGTH 500
#define DIRECTIONS 8

// To have the up down right left etc. with the name
typedef struct {
    int x;
    int y;
    char *directionName;
} Direction;

// To return the coordinates and direction name
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
            lookX +=x;
            lookY +=y;
        }
        else {
            return NULL;
        }
    }

    if (wordI == wordLength)
    {
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

    for (int i=0; i<ROW; i++){
        for (int j=0; j<COL; j++){
            // Find the first letter of the word to start searching
            // TODO: convert to threads too, and stop as soon as one gets
            // the result
            if (matrix[i][j] == word[0]){
                // printf("Starting search from [%d, %d]\n", i, j);
                exploreDirection(matrix, ROW, COL, word, i, j, wordLength);
            }
        }
    }
    printf("Finished searching for word %s\n", word);
}

int main(int argc, char *argv[]) {
    FILE *file;
    char* filename;
    int ROW, COL;
    int i, j;

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

    printf("Opened the file!\n");

    // scans the matrix dimensions from first line
    fscanf(file, "%d %d\n", &ROW, &COL); 
    // Now the file pointer is in the start of the "matrix"
    printf("i: %d, j: %d\n", ROW, COL);
    
    printf("Allocating memory for the matrix\n");
    char **matrix = malloc(ROW * sizeof(char *));
    for (i = 0; i < ROW; i++) {
        matrix[i] = malloc(COL * sizeof(char));
    }
    printf("Finished allocating memory for the matrix\n");

    printf("Starting to read the matrix\n");
    int count = 0;
    char cell;
    for (i=0; i<ROW; i++){
        for (j=0; j<COL; j++){
            fscanf(file, "%c\n", &cell);
            // TODO: write to another file
            // printf("%c", cell);
            matrix[i][j] = cell;
            count++;
        }
        // printf("\n");
    }
    printf("Read the matrix\n");

    // printf("Cell Count: %d\n", count);

    char tempWord[MAX_WORD_LENGTH + 1];
    char* words[MAX_WORDS];
    int wordCount = 0;
    
    // now scan the words that we'll search for and put into an array
   while (fscanf(file, "%s\n", tempWord) == 1) {
        // printf("Read line: %s\n", tempWord);
        words[wordCount] = malloc(strlen(tempWord)+1);
        // Copy that word into the array of words
        strcpy(words[wordCount], tempWord);
        // Check if it was properly added to the array of words
        // printf("Added to array: %s\n", words[wordCount]);
        wordCount++;
    }
    fclose(file);

    for(i = 0; i < wordCount; i++) {
        printf("%s\n", words[i]);
    }
    
    // Create one thread for each word
    pthread_t searchThreads[wordCount];
    searchWordArgs argsSearch[wordCount];

    for (i=0; i<wordCount; i++){
        argsSearch[i].matrix = matrix;
        argsSearch[i].ROW = ROW;
        argsSearch[i].COL = COL;
        argsSearch[i].word = words[i];

        printf("Initial thread for %s\n", words[i]);
        pthread_create(&searchThreads[i], NULL, searchWord, &argsSearch[i]);
        printf("Thread %d created\n", i);
    }

    for (i = 0; i < wordCount; i++) {
        pthread_join(searchThreads[i], NULL);
    }

    for (i = 0; i < wordCount; i++) {
        free(words[i]);
    }

    printf("Freeing memory for the matrix\n");
    for (i = 0; i < ROW; i++) {
        free(matrix[i]);
    }
    free(matrix);
    return 1;
}

