#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#define MAX_WORDS 20
#define MAX_WORD_LENGTH 500
#define DIRECTIONS 8

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
} findInDirectionArgs;

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

    int lookX = currX+x;
    int lookY = currY+y;
    int wordI = 1;

    while (wordI<wordLength){
        if (lookX >= maxX || lookY >= maxY || lookX < 0 || lookY < 0){
            return (void *)0;
        }
        
        // Now we look and check if it's what we want to find
        if (matrix[lookX][lookY] == word[wordI]) {
            // Look again in that direction, and up the index
            char oldTarget = word[wordI];
            wordI++;
            lookX +=x;
            lookY +=y;
            
        }
        else {
            return (void *)0;
        }
    }

    if (wordI==wordLength){
        return (void *)1;
    }
    return (void *) 0;
} 

void *exploreDirection(char **matrix, int ROW, int COL, char *word, int startX, int startY, int wordLength) {
    int result;
    int i = 0;
    pthread_t directionThreads[DIRECTIONS];
    findInDirectionArgs directionArgs[DIRECTIONS];
    int results[DIRECTIONS] = {0};
    int directionCount = 0;
    // TODO: instead of for loop, I need 8 directions
    // cima, baixo, direita, esquerda, cima direita, cima esquerda, baixo direita, baixo esquerda
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;

            directionArgs[i].x = x;
            directionArgs[i].y = y;
            directionArgs[i].matrix = matrix;
            directionArgs[i].maxX = ROW;
            directionArgs[i].maxY = COL;
            directionArgs[i].word = word;
            directionArgs[i].currX = startX;
            directionArgs[i].currY = startY;
            directionArgs[i].wordLength = wordLength;

            pthread_create(&directionThreads[i], NULL, findInDirection, &directionArgs[i]);
            i++;
        }
    }

    for (int j = 0; j< i; j++) {
        void *status;
        pthread_join(directionThreads[j], &status);
        results[j] = (int)(size_t)status;
        if (results[j] == 1) {
            printf("Word found.\n");
            break;
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
    int result = 1; 
    char curr;
    int wordLength = strlen(word);

    for (int i=0; i<ROW; i++){
        for (int j=0; j<COL; j++){
            curr=matrix[i][j];
            // Find the first letter of the word to start searching
            // TODO: convert to threads too, and stop as soon as one gets
            // the result
            if (curr == word[0]){
                exploreDirection(matrix, ROW, COL, word, i, j, wordLength);
            }
        }
    }
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
            printf("%c", cell);
            matrix[i][j] = cell;
            count++;
        }
        printf("\n");
    }

    printf("Cell Count: %d\n", count);

    char tempWord[MAX_WORD_LENGTH + 1];
    char* words[MAX_WORDS];
    int wordCount = 0;
    
    // now scan the words that we'll search for and put into an array
   while (fscanf(file, "%s\n", tempWord) == 1) {
        printf("Read line: %s\n", tempWord);
        words[wordCount] = malloc(strlen(tempWord)+1);
        // Copy that word into the array of words
        strcpy(words[wordCount], tempWord);
        // Check if it was properly added to the array of words
        printf("Added to array: %s\n", words[wordCount]);
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

        pthread_create(&searchThreads[i], NULL, searchWord, &argsSearch[i]);
    }
    
    // Wait for all to finish
    for (i=0; i<wordCount; i++){
        pthread_join(&searchThreads[i], NULL);
    }

    printf("Freeing memory for the matrix\n");
    for (i = 0; i < ROW; i++) {
        free(matrix[i]);
    }
    free(matrix);
    return 1;
}

