#include <iikit.h> // Biblioteca base do framework Arduino
#include "util/jtask.h"

// Configurações para a geração da senoide:
#define PWM_CHANNEL  0
#define NUMSAMPLES 100 // Número de amostras por período da senoide
uint8_t sineTable[NUMSAMPLES];

// Gera a tabela de valores para a senoide:
void makePoints()
{
    // Mapeamos a senoide (de -1 a 1) para valores de 0 a 255, que o DAC espera.
    for (int i = 0; i < NUMSAMPLES; i++)
    {
        float angle = (2 * PI * i) / NUMSAMPLES;
        float s = sin(angle);
        sineTable[i] = (uint8_t)((s + 1.0) * 80) + 100; // mapeia: -1 → 25 e 1 → 225
    }
}

//uint8_t sampleIndex = 0;
void buildWave()
{
    static uint8_t sampleIndex = 0;
    //dacWrite(def_pin_DAC1, sineTable[sampleIndex]);
    ledcWrite(PWM_CHANNEL, sineTable[sampleIndex]);
    sampleIndex = (sampleIndex + 1) % NUMSAMPLES;
}

void setup()
{
    IIKit.setup();
    ledcAttachPin(def_pin_DAC1, PWM_CHANNEL);
    ledcSetup(PWM_CHANNEL, 500, 10); // Frequência 500Hz, 10 bits de resolução
    makePoints();
    //Para gerar uma senoide de 100Hz com 100 amostras, o período total é 10000 µs.
    //Assim, cada amostra deve ser atualizada a cada 10000/100 = 100 µs.
    jtaskAttachFunc(buildWave, 1000UL);
}

void loop()
{
    IIKit.loop();
    jtaskLoop(); // Chama o loop de tarefas da jtask para executar as tarefas agendadas.
}
