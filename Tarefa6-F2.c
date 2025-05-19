/*
 *  Estação de Monitoramento de Cheias
 *  Por: Ronaldo César Santos Rocha
 *  Data: 19/05/2025
 *
 *  Usa FreeRTOS no RP2040 (BitDog Lab) e filas p/ comunicações.
 */

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lib/font.h"
#include "lib/buzzer.h"
#include "lib/matrixws.h"
#include "lib/rgb.h"
#include "lib/display_init.h"

// --- Pinos e constantes --- //
#define ADC_JOYSTICK_X    26    // GPIO26 = ADC0 = chuva
#define ADC_JOYSTICK_Y    27    // GPIO27 = ADC1 = nível

#define LED_R             13    // PWM vermelho
#define LED_G             11    // PWM verde
#define LED_B             12    // PWM azul

#define BUZZER_PIN        buzzer      // definido em buzzer.h
#define MATRIX_PIN        PINO_MATRIZ // definido em matrixws.h

#define I2C_SDA           14
#define I2C_SCL           15
#define I2C_PORT          i2c1
#define OLED_ADDR         0x3C

#define WIDTH             128
#define HEIGHT            64

// Limiar de alerta em ADC (0–4095)
#define LEVEL_ALERT_TH    2867  // ~70%
#define RAIN_ALERT_TH     3276  // ~80%

typedef enum { NORMAL = 0, ALERTA = 1 } SystemStatus;

typedef struct {
    uint16_t rain;
    uint16_t level;
} SensorData;

// Filas
static QueueHandle_t qSensor;    // SensorData
static QueueHandle_t qDisplay;   // SensorData para display
static QueueHandle_t qStatus;    // SystemStatus para todos os atuadores

// Protótipos
void vTaskSensorRead(void *p);
void vTaskProcess(void *p);
void vTaskLedRGB(void *p);
void vTaskBuzzer(void *p);
void vTaskMatrix(void *p);
void vTaskDisplay(void *p);

int main() {
    stdio_init_all();

    // 1) ADC joystick
    adc_init();
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_gpio_init(ADC_JOYSTICK_Y);

    // 2) I2C + OLED
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    display();  // inicializa SSD1306

    // 3) PWM LED RGB
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    uint slice_r = pwm_gpio_to_slice_num(LED_R);
    pwm_config config_r = pwm_get_default_config();
    pwm_config_set_wrap(&config_r, 255);
    pwm_init(slice_r, &config_r, true);

    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    uint slice_g = pwm_gpio_to_slice_num(LED_G);
    pwm_config config_g = pwm_get_default_config();
    pwm_config_set_wrap(&config_g, 255);
    pwm_init(slice_g, &config_g, true);

    gpio_set_function(LED_B, GPIO_FUNC_PWM);
    uint slice_b = pwm_gpio_to_slice_num(LED_B);
    pwm_config config_b = pwm_get_default_config();
    pwm_config_set_wrap(&config_b, 255);
    pwm_init(slice_b, &config_b, true);

    // Estado inicial: verde ligado (modo Normal)
    pwm_set_gpio_level(LED_R, 0);
    pwm_set_gpio_level(LED_G, 255);
    pwm_set_gpio_level(LED_B, 0);

    // 4) Buzzer parado
    buzzer_init(BUZZER_PIN, 2000);
    buzzer_stop(BUZZER_PIN);

    // 5) Matriz apagada
    controle(MATRIX_PIN);
    desliga();

    // 6) Filas (tamanho 1 + overwrite)
    qSensor  = xQueueCreate(1, sizeof(SensorData));
    qDisplay = xQueueCreate(1, sizeof(SensorData));
    qStatus  = xQueueCreate(1, sizeof(SystemStatus));

    // 7) Tasks
    xTaskCreate(vTaskSensorRead, "Sensor",  256, NULL, 3, NULL);
    xTaskCreate(vTaskProcess,    "Process", 256, NULL, 4, NULL);
    xTaskCreate(vTaskLedRGB,     "RGB",     128, NULL, 2, NULL);
    xTaskCreate(vTaskBuzzer,     "Buzz",    128, NULL, 2, NULL);
    xTaskCreate(vTaskMatrix,     "Matrix",  256, NULL, 2, NULL);
    xTaskCreate(vTaskDisplay,    "Disp",    512, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1) tight_loop_contents();
    return 0;
}

// Task 1: lê joystick e envia para qSensor
void vTaskSensorRead(void *p) {
    SensorData sd;
    for (;;) {
        adc_select_input(0);
        sd.rain  = adc_read();
        adc_select_input(1);
        sd.level = adc_read();
        xQueueOverwrite(qSensor, &sd);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Task 2: processa e envia para qDisplay e qStatus
void vTaskProcess(void *p) {
    SensorData sd;
    SystemStatus st;
    for (;;) {
        if (xQueueReceive(qSensor, &sd, portMAX_DELAY)) {
            st = (sd.level >= LEVEL_ALERT_TH || sd.rain >= RAIN_ALERT_TH)
                 ? ALERTA : NORMAL;
            xQueueOverwrite(qDisplay, &sd);
            xQueueOverwrite(qStatus, &st);
        }
    }
}

// Task 3: LED RGB
void vTaskLedRGB(void *p) {
    SystemStatus st;
    for (;;) {
        xQueuePeek(qStatus, &st, portMAX_DELAY);
        if (st == ALERTA) {
            pwm_set_gpio_level(LED_R, 255);
            pwm_set_gpio_level(LED_G, 0);
            pwm_set_gpio_level(LED_B, 0);
        } else {
            pwm_set_gpio_level(LED_R, 0);
            pwm_set_gpio_level(LED_G, 255);
            pwm_set_gpio_level(LED_B, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Task 4: Buzzer
void vTaskBuzzer(void *p) {
    SystemStatus st;
    for (;;) {
        xQueuePeek(qStatus, &st, portMAX_DELAY);
        if (st == ALERTA) {
            buzzer_init(BUZZER_PIN, 1000);
            buzzer_set_freq(BUZZER_PIN, 1000);
            sleep_ms(200);
            buzzer_stop(BUZZER_PIN);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            buzzer_stop(BUZZER_PIN);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// Task 5: Matriz WS2818B
void vTaskMatrix(void *p) {
    SystemStatus st;
    const uint8_t tri[][2] = {
        {1,2}, {2,1},{2,2},{2,3}, {3,0},{3,1},{3,2},{3,3},{3,4}
    };
    const size_t tri_len = sizeof(tri)/sizeof(tri[0]);

    for (;;) {
        xQueuePeek(qStatus, &st, portMAX_DELAY);
        if (st == ALERTA) {
            for (int i = 0; i < NUM_LEDS; i++) cores(i, 255, 0, 0);
            for (size_t t = 0; t < tri_len; t++) {
                int pos = getIndex(tri[t][0], tri[t][1]);
                cores(pos, 255, 255, 0);
            }
        } else {
            desliga();
        }
        bf();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// Task 6: Display OLED
void vTaskDisplay(void *p) {
    SensorData sd;
    SystemStatus st;
    ssd1306_t disp;
    ssd1306_init(&disp, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&disp);

    char buf[32];
    for (;;) {
        xQueueReceive(qDisplay, &sd, portMAX_DELAY);
        xQueuePeek(qStatus, &st, portMAX_DELAY);
        ssd1306_fill(&disp, false);

        snprintf(buf,sizeof(buf),"Chuva: %4u", sd.rain);
        ssd1306_draw_string(&disp, buf, 0, 0);
        snprintf(buf,sizeof(buf),"Nivel: %4u", sd.level);
        ssd1306_draw_string(&disp, buf, 0,10);

        if (st == ALERTA) {
            ssd1306_draw_string(&disp,"!!! ALERTA !!!", 0,50);
        } else {
            ssd1306_draw_string(&disp,"   NORMAL    ", 0,50);
        }
        ssd1306_send_data(&disp);
    }
}
