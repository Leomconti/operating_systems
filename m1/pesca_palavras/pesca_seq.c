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

// definir os argumentos para as funcoes, como podem ser threads, so recebem um argumento
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

// para cada direcao, temos um x e y para "caminhar" na matriz, e o nome da direcao
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

// isso eh para guardar as palavras encontradas
// para threads tb vou adicionar um mutex para garantir que nao vai ter problema de race condition
FoundWordDetail foundWords[MAX_WORDS];
int foundWordCount = 0;


void findInDirection(findInDirectionArgs *a) {
    // parse dos argumentos da struct
    int lookX = a->currX;
    int lookY = a->currY;
    char **matrix = a->matrix;
    char *word = a->word;
    int maxX = a->maxX;
    int maxY = a->maxY;
    int x = a->x;
    int y = a->y;
    int currX = a->currX;
    int currY = a->currY;
    char *directionName = a->directionName;
    int wordLength = a->wordLength;

    for (int i = 1; i < wordLength; ++i) {
        lookX += x;
        lookY += y;
        // checa se a palavra nao bate com a matriz, ou se passou dos limites
        if (lookX < 0 || lookX >= maxX || lookY < 0 || lookY >= maxY || matrix[lookX][lookY] != word[i]) {
            return;  // sai da funcao
        }
    }

    // Se chegou aqui eh pq achou a palavra
    printf("%s - (%d,%d):%s.\n", word, currX, currY, directionName);

    // Record the finding for later writing to file
    if (foundWordCount < MAX_WORDS) {
        strncpy(foundWords[foundWordCount].word, word, MAX_WORD_LENGTH);
        foundWords[foundWordCount].row = currX;
        foundWords[foundWordCount].col = currY;
        strncpy(foundWords[foundWordCount].direction, directionName, MAX_WORD_LENGTH);
        foundWordCount++;
    }
}


// comecando de um ponto encontrado, explora todas as direcoes possiveis
// dai chama o find in direction, que pega a direcao e realmente procura as palavras la
void exploreDirection(char **matrix, int ROW, int COL, char *word, int startX, int startY) {
    findInDirectionArgs args;

    for (int i = 0; i < DIRECTIONS; ++i) {
        // Temos que criar uma struct para passar os argumentos
        args = (findInDirectionArgs){
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

        findInDirection(&args);
    }
}


// funcao sequencial para procurar cada palavra na matriz
void searchWord(searchWordArgs *a) {
    char **matrix = a->matrix;
    int ROW = a->ROW;
    int COL = a->COL;
    char *word = a->word;

    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            if (matrix[i][j] == word[0]) {
                exploreDirection(matrix, ROW, COL, word, i, j);
            }
        }
    }
}


// como tem um formato definido para o input, lemos o tamanho da matriz e entao ela
char** readMatrixFromFile(FILE* file, int* ROW, int* COL) {
    fscanf(file, "%d %d\n", ROW, COL);
    // alocar memoria para a matriz ixj
    char **matrix = (char **)malloc(*ROW * sizeof(char *));
    for (int i = 0; i < *ROW; ++i) {
        // alocar memoria para cada linha da matriz
        matrix[i] = (char *)malloc(*COL * sizeof(char));
        for (int j = 0; j < *COL; ++j) {
            // usa a regex %c\n para ler cada caractere
            fscanf(file, "%c\n", &matrix[i][j]);
        }
    }
    return matrix;
}


char** readWordsFromFile(FILE* file, int* wordCount) {
    // pra ler as palavras do arquivo, vamos continuar com o mesmo arquivo aberto
    // como o ponteiro do arquivo vai estar no final da matriz, onde as palavras começam,
    // vamos ler uma palavra por linha usando o regex %s\n.
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


char** capitalizeFoundWords(char **matrix, FoundWordDetail *foundWords, int foundWordCount) {
    // Pega a palavra, a posicao, a direcao, e vai capitaliznado ate dar o tamanho da palavra
    for (int i = 0; i < foundWordCount; i++) {
        int x = foundWords[i].row;
        int y = foundWords[i].col;
        char *word = foundWords[i].word;
        
        int dirX = 0, dirY = 0;
        for (int d = 0; d < DIRECTIONS; d++) {
            // compara a direcao com a da struct, para achar a direcao x y para "caminhar"
            if (strcmp(directions[d].directionName, foundWords[i].direction) == 0) {
                dirX = directions[d].x;
                dirY = directions[d].y;
                break;
            }
        }

        // Caminha na direcao ate dar o tamanho da palavra
        for (int j = 0; j < strlen(word); j++) {
            matrix[x][y] = toupper(matrix[x][y]);
            // printf("Toupper %c em: (%d, %d)\n", matrix[x][y], x, y);
            x += dirX;
            y += dirY;
        }
    }
    return matrix;
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

    // pega tempo real e de cpu
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


    // pegar o tempo de execucao, em tempo real e de clock cpu
    endTime = clock();
    gettimeofday(&end, NULL);
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf("Real Execution Time: %ld seconds, %ld microseconds\n", seconds, micros);
    cpuTimeUsed = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;
    printf("CPU Execution time: %f seconds\n", cpuTimeUsed);
    
    // atualizar a matriz que sera escrita no result, com capitalizacao das palavras encontradas
    matrix = capitalizeFoundWords(matrix, foundWords, foundWordCount);

    printf("Writing found words to file\n");
    FILE *resultFile = fopen("result_seq.txt", "w");
    if (resultFile != NULL) {
        for (int i = 0; i < ROW; i++) {
            for (int j = 0; j < COL; j++) {
                fprintf(resultFile, "%c", matrix[i][j]);
            }
            fprintf(resultFile, "\n");
        }
        for (int i = 0; i < foundWordCount; i++) {
            fprintf(resultFile, "%s - (%d, %d):%s\n", foundWords[i].word, foundWords[i].row, foundWords[i].col, foundWords[i].direction);
        }
        fprintf(resultFile, "Real Execution Time: %ld seconds, %ld microseconds\n", seconds, micros);
        fprintf(resultFile, "CPU Execution time: %f seconds\n", cpuTimeUsed);
        fclose(resultFile);
    } else {
        perror("Could not open result.txt for writing");
    }

    return 0;
}
