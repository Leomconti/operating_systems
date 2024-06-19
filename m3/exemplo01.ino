#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

// Define pins
#define YELLOW_LED 8
#define GREEN_LED 4
#define BLUE_LED 7

#define BUTTON1 2
#define BUTTON2 3

// Task handles
TaskHandle_t LEDTaskHandle = NULL;
TaskHandle_t ButtonTaskHandle = NULL;
TaskHandle_t CommandInterpreterHandle = NULL;

QueueHandle_t xQueueCommand;

SemaphoreHandle_t xSemaphore;

// Quais leds devem acender e a duracao
struct Command {
  bool yellow;
  bool green;
  bool blue;
  int duration;
};

bool sequenceRunning = false;
bool sequencePaused = false;
int sequenceIndex = 0;
Command commandSequence[10];
int commandCount = 0;

void LEDTask(void *pvParameters);
void ButtonTask(void *pvParameters);
void CommandInterpreterTask(void *pvParameters);

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // inicializar pinos de led e botoes
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // criar fila de comandos
  xQueueCommand = xQueueCreate(10, sizeof(Command));

  // criar semaforo, vai servir para pausar quando clicar, e para liberar a proxima sequencia ?
  xSemaphore = xSemaphoreCreateBinary();

  // criar uma task para os LEDs, para o botao, e para interpretar os comandos
  xTaskCreate(LEDTask, "LED Task", 128, NULL, 1, &LEDTaskHandle);
  xTaskCreate(ButtonTask, "Button Task", 128, NULL, 1, &ButtonTaskHandle);
  xTaskCreate(CommandInterpreterTask, "Command Interpreter Task", 256, NULL, 1, &CommandInterpreterHandle);

  // hardcoded commands sequence
  commandSequence[0] = {true, false, true, 1};  // Y and B for 1 second
  commandSequence[1] = {false, true, false, 2}; // G for 2 seconds
  commandSequence[2] = {true, true, false, 3};  // Y and G for 3 seconds
  commandSequence[3] = {false, false, true, 1}; // B for 1 second
  commandCount = 4;

  // comecar o escalonador de tarefas
  vTaskStartScheduler();
}

void loop() {
  // Sem loop ja q so vai ser task do FreeRTOS,
  // se tiver algo no loop eh 0 hein!!!
}

// Task para controlar os LEDs
// depois splitar em 3 tasks
void LEDTask(void *pvParameters) {
  Command command;
  while (1) {
    if (xQueueReceive(xQueueCommand, &command, portMAX_DELAY) == pdPASS) {
      if (command.yellow) {
        digitalWrite(YELLOW_LED, HIGH);
      }
      if (command.green) {
        digitalWrite(GREEN_LED, HIGH);
      }
      if (command.blue) {
        digitalWrite(BLUE_LED, HIGH);
      }
      vTaskDelay(command.duration * 1000 / portTICK_PERIOD_MS); // * 1000 pq eh segundo, dai converte para ms
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      xSemaphoreGive(xSemaphore);
    }
  }
}

void ButtonTask(void *pvParameters) {
  while (1) {
    if (digitalRead(BUTTON1) == HIGH) {
      // se estiver rodando PAUSA
      if (sequenceRunning) {
        sequencePaused = !sequencePaused;
        if (sequencePaused) {
          sequenceRunning = false;
        } else {
          xSemaphoreGive(xSemaphore);
        }
      } else {
        // se nao tiver rodando bota pra rodar
        sequenceRunning = true;
        xSemaphoreGive(xSemaphore);
      }
      vTaskDelay(300 / portTICK_PERIOD_MS); // debounce
    }
  // adicionar botao para ler seriais
  }
}

void CommandInterpreterTask(void *pvParameters) {
  while (1) {
    if (sequenceRunning && !sequencePaused && sequenceIndex < commandCount) {
      xQueueSend(xQueueCommand, &commandSequence[sequenceIndex], portMAX_DELAY);
      sequenceIndex++;
      if (sequenceIndex >= commandCount) {
        sequenceRunning = false;
      }
      xSemaphoreTake(xSemaphore, portMAX_DELAY);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
