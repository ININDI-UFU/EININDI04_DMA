#include <iikit.h> // Biblioteca base do framework Arduino
#include <driver/dac.h>
#include "AdcDmaEsp.h"

// 1) Configurações para o DAC (gerano um seno)
const int SAMPLES = 100;
uint8_t sineTable[SAMPLES];
volatile int sineIndex = 0;
    
// === Timer callback ===
void IRAM_ATTR onTimer(void *param) {
    dac_output_voltage(DAC_CHANNEL_1, sineTable[sineIndex++]);// Envia o próximo ponto da senoide para o DAC
    if (sineIndex >= SAMPLES) sineIndex = 0;  // volta pro início da tabela
}

// Gera a tabela de valores para a senoide:
void makePoints()
{
    // Mapeamos a senoide (de -1 a 1) para valores de 0 a 255, que o DAC espera.
    for (int i = 0; i < SAMPLES; i++)
    {
        float angle = (2 * PI * i) / SAMPLES;
        float s = sin(angle);
        int dacValue = (int)((s + 1.0) * 70.0f + 100.0f) ; // mapeia: -1 → 25 e 1 → 225
        sineTable[i] = (uint8_t) dacValue;
    }
}

void setup()
{
    IIKit.setup();
    makePoints();

    // === Configura o timer de alta precisão ===
    // DAC1	GPIO 25	DAC_CHANNEL_1
    // DAC2	GPIO 26	DAC_CHANNEL_2
    dac_output_enable(DAC_CHANNEL_1);
    esp_timer_create_args_t timerConf = {};
    timerConf.callback = &onTimer;
    timerConf.name = "sineTimer";

    esp_timer_handle_t timer;
    esp_timer_create(&timerConf, &timer);
    esp_timer_start_periodic(timer, 166); // 60 Hz com 100 amostras → 6000 Hz → período 166 microsegundos
}

void loop()
{
    IIKit.loop();// Chama o loop de tarefas da jtask para executar as tarefas agendadas.
}
