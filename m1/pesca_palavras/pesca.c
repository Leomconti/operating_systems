#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_WORDS 20 // Maximum number of words we set to 20
#define MAX_WORD_LENGTH 500 // Maximum length of a word to a huge string, but we won't reach that

int findInDirection(int x, int y, char **matrix, int MaxX, int MaxY, char word[], int tX, int tY, int wordLength){
    int lookX;
    int lookY;
    int wordI = 0;
    
    while (wordI<wordLength){
        wordI++;
        printf("Look Around %d %d\n", x, y);
    
        lookX = tY+x;
        lookY = tY+y;
        
        if (lookX > MaxX || lookY > MaxY){
            printf("Out of bounds\n");
            return 0;
        }

        if (matrix[lookX][lookY] == word[wordI]) {
            printf("Found in direction\n");
            printf("%c == %c\n", matrix[lookX][lookY], word[wordI]);
            wordI++;
            tX++;
            tY++;
        }
        else {
            printf("Did not find in this direction\n");
            return 0;
        }
    }

    // If it has finished, return 1
    if (wordI<=wordLength){
        printf("Finished\n");
        return 1;
    }
    return 0;
} 

int searchWord(char **matrix, int ROW, int COL, char word[]){
    // In this function we'll traverse the matrix looking for the words
    // My first idea is:
    // We start reading the matrix, if we find the letter that starts the word, we 
    // Look at each direction and if it contains the second letter we do the same there
    // If it doesn't go through we come back
    // It will use a recursive approach, basically a maze solver
    int result = 1; 
    char curr;
    int wordLength = strlen(word);

    for (int i=0; i<ROW; i++){
        for (int j=0; j<COL; j++){
            curr=matrix[i][j];
            // Let's find the first letter
            if (curr != word[0]){
                continue;
            }
            // Found the first letter of the word somewhere, 
            // Now we have to look around to see if we can find the word in any direction
            printf("Found starting letter");

            // Here we iterate through the possible directions that it'll look at
            for (int x=0; x<=1; x++){
                for (int y=0; y<=1; y++){
                    // The idea is that here we'll laungh one children for each direction
                    // Check if it's not the center
                    if (x==0 && y==0){
                        continue;
                    } 

                    result = findInDirection(x,y,matrix,ROW,COL,word,i,j, wordLength); 
                    if (result == 1) {
                        printf("Finished\n");
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
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
    char **matrix = malloc(ROW * sizeof(char *)); // Allocate an array of pointers to char for the ROW
    for (i = 0; i < ROW; i++) {
        matrix[i] = malloc(COL * sizeof(char)); // Allocate an array of char for each row, to the size of the COL
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
    // Now just missing reading the words at the end

    char tempWord[MAX_WORD_LENGTH + 1]; // Buffer for reading each word
    char* words[MAX_WORDS]; // Array to hold pointers to words
    int wordCount = 0;
    
    // now scan the words that we'll search for and put into an array
   while (fscanf(file, "%s\n", tempWord) == 1) {
        printf("Read line: %s\n", tempWord);
        // Allocate memory for the tempWord
        words[wordCount] = malloc(strlen(tempWord)+1);
        // Copy that word into the array of words
        strcpy(words[wordCount], tempWord);
        // Check if it was properly added to the array of words
        printf("Added to array: %s\n", words[wordCount]);
        wordCount++;
    }
    fclose(file);

    // Loop through the array and print each string
    for(i = 0; i < wordCount; i++) {
        printf("%s\n", words[i]);
    }

    searchWord(matrix, ROW, COL, words[0]);

    // Now we're basically set to start working on finding the words

    // char find;
    // int_t childpid;
    // for (i=0; i<wordCount; i++){
    //     find = words[i];
    //     childpid = fork();
    // }
    
    printf("Freeing memory for the matrix\n");
    for (i = 0; i < ROW; i++) {
        free(matrix[i]); // Free each row
    }
    free(matrix); // free the array of pointers
    //
    return 1;
}

