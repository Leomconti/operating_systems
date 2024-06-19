#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#define YELLOW_LED 8
#define GREEN_LED 4
#define BLUE_LED 7

#define BUTTON1 2
#define BUTTON2 3

TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t commandTaskHandle = NULL;

QueueHandle_t commandQueue;

SemaphoreHandle_t semaphore;

/*
1. Criar uma struct para guardar as informacoes necessarias para ver se eh preciso ligar a led e por quanto tempo
2. Instanciar variaveis de controle para saber se o sistema esta rodando, pausado e qual o indice do comando atual
3. Criar um array de comandos para armazenar os comandos lidos do terminal
4. Criar uma funcao para ler os comandos do terminal
5. Criar uma funcao para controlar as leds
6. Criar uma funcao para controlar o botao
7. Criar uma funcao para controlar a leitura e fila de comandos
8. Setup
8.1 No setup, inicizliamos a conexao com o serial, inicializamos os pinos das leds e botoes que serao utilizaods
8.2 Criamos a fila de comandos
8.3 Criamos o semaforo que sera utilizado para o controle, play / pause das leds
8.4 Criamos as tarefas
*/

// Command structure to define which LEDs to light up and the duration
struct Command {
  bool yellow;
  bool green;
  bool blue;
  int duration;
};

bool isRunning = false;
bool isPaused = false;
int currentIndex = 0;
Command commandList[10];
int commandCount = 0;

void ledTask(void *pvParameters);
void buttonTask(void *pvParameters);
void commandTask(void *pvParameters);
void readCommands();

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize LED and button pins
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // create command queue
  commandQueue = xQueueCreate(80, sizeof(Command));

  // Create semaphore
  semaphore = xSemaphoreCreateBinary();

  // Create tasks
  xTaskCreate(ledTask, "LED Task", 128, NULL, 1, &ledTaskHandle);
  xTaskCreate(buttonTask, "Button Task", 128, NULL, 1, &buttonTaskHandle);
  xTaskCreate(commandTask, "Command Task", 256, NULL, 1, &commandTaskHandle);

  // Initialize command sequence
  readCommands();

  // Start task scheduler
  vTaskStartScheduler();
}

void loop() {
  // No code in loop since FreeRTOS is managing the tasks
}

// Task to control LEDs
void ledTask(void *pvParameters) {
  Command command;
  while (1) {
    if (xQueueReceive(commandQueue, &command, portMAX_DELAY) == pdPASS) {
      if (command.yellow) {
        digitalWrite(YELLOW_LED, HIGH);
        Serial.println("Yellow On");
      }
      if (command.green) {
        digitalWrite(GREEN_LED, HIGH);
        Serial.println("Green On");
      }
      if (command.blue) {
        digitalWrite(BLUE_LED, HIGH);
        Serial.println("Blue On");
      }
      Serial.println("Duration: " + String(command.duration) + " sec");
      vTaskDelay(command.duration * 1000 / portTICK_PERIOD_MS); // Convert to ms
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      Serial.println("Stopping all LEDs");
      xSemaphoreGive(semaphore);
    }
  }
}


// Task que controla o botao
void buttonTask(void *pvParameters) {
  while (1) {
    if (digitalRead(BUTTON1) == HIGH) {
      if (isRunning) {
        isPaused = !isPaused;
        if (isPaused) {
          isRunning = false;
        } else {
          xSemaphoreGive(semaphore);
        }
      } else {
        isRunning = true;
        xSemaphoreGive(semaphore);
      }
      vTaskDelay(300 / portTICK_PERIOD_MS); // Debounce
    }
  }
}

// Task que controla o envio dos comandos para a fila
void commandTask(void *pvParameters) {
  while (1) {
    if (isRunning && !isPaused && currentIndex < commandCount) { 
      xQueueSend(commandQueue, &commandList[currentIndex], portMAX_DELAY);
      currentIndex++;
      if (currentIndex >= commandCount) {
        isRunning = false;
      }
      xSemaphoreTake(semaphore, portMAX_DELAY);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Funcao que le os comandos do terminal, nao eh uma task, sera chamada no setup
void readCommands() {
  String inputString = "";
  while (true) {
    while (Serial.available()) {
      Serial.println("Getting char from serial");
      char inChar = (char)Serial.read();
      inputString += inChar;
      if (inChar == 'E') {
        // Process the input string
        commandCount = 0;
        int i = 0;
        while (i < inputString.length() && inputString[i] != 'E') {
          Command cmd = {false, false, false, 0};
          while (i < inputString.length() && !isdigit(inputString[i])) {
            switch (inputString[i]) {
              case 'Y':
                cmd.yellow = true;
                break;
              case 'G':
                cmd.green = true;
                break;
              case 'B':
                cmd.blue = true;
                break;
            }
            i++;
          }
          if (i < inputString.length() && isdigit(inputString[i])) {
            cmd.duration = inputString[i] - '0'; // Convert char to int
            i++;
          }
          commandList[commandCount++] = cmd;
        }
        Serial.println("E! Got the commands sequence");
        return;
      }
    }
  }
}
