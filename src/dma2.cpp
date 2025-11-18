// #include <iikit.h>      // Biblioteca base do framework Arduino
// #include <driver/dac.h>
// #include "util/AdcDmaEsp.h" // Classe para configuração do ADC e DMA

// // Configurações para a geração da senoide:
// #define PWM_CHANNEL  0
// #define NUMSAMPLES 100 // Número de amostras por período da senoide

// // Instância global
// AdcDmaEsp adcDma;

// // Buffer para leitura no loop
// static int16_t readBuffer[256];

// // Gera a tabela de valores para a senoide:
// uint8_t sineTable[NUMSAMPLES];

// void makePoints()
// {
//     // Mapeamos a senoide (de -1 a 1) para valores de 0 a 255, que o DAC espera.
//     for (int i = 0; i < NUMSAMPLES; i++)
//     {
//         float angle = (2 * PI * i) / NUMSAMPLES;
//         float s = sin(angle);
//         int dacValue = (int)((s + 1.0) * 70.0f + 100.0f) ; // mapeia: -1 → 25 e 1 → 225
//         sineTable[i] = (uint8_t) dacValue;
//     }
// }    

// //uint8_t sampleIndex = 0;
// void buildWave()
// {
//     static uint8_t sampleIndex = 0;
//     dacWrite(def_pin_DAC1, sineTable[sampleIndex]);
//     sampleIndex = (sampleIndex + 1) % NUMSAMPLES;
// }

// void setup()
// {
//     IIKit.setup();
//     makePoints();

//     // DAC1	GPIO 25	DAC_CHANNEL_1
//     // DAC2	GPIO 26	DAC_CHANNEL_2
//     dac_output_enable(DAC_CHANNEL_1);

//     wserial::println();
//     wserial::println("=== Teste ADC DMA ESP32 ===");

//     // ADC1_CHANNEL_1 → GPIO39
//     // ADC1_CHANNEL_0 → GPIO36
//     if (!adcDma.begin(ADC1_CHANNEL_0, 100)) {  // 1 kHz
//         wserial::println("Falha ao iniciar ADC DMA");
//         while (true) delay(1000);
//     }
//     wserial::println("ADC DMA iniciado.");
// }

// void loop()
// {
//     IIKit.loop();
//     uint32_t now = millis();

//     // 1) Coleta o que o DMA já produziu
//     adcDma.poll();    

//      // 2) De tempos em tempos, leia tudo o que chegou
//     static uint32_t lastPrint0 = 0; 
//     if (now - lastPrint0 >= 100) {  // a cada 100 ms
//         lastPrint0 = now;
//         size_t n = adcDma.read(readBuffer, 256);
//         if (n > 0) wserial::plot("adcValue", 10, readBuffer, n);
//     }

//     // 3) De tempos em tempos, plota o seno
//     static int sineIndex = 0;
//     static uint32_t lastPrint1 = 0; 
//     if (now - lastPrint1 >= 1) {  // a cada 100 ms
//         lastPrint1 = now;
//         dac_output_voltage(DAC_CHANNEL_1, sineTable[sineIndex++]);// Envia o próximo ponto da senoide para o DAC
//         if (sineIndex >= NUMSAMPLES) sineIndex = 0;  // volta pro início da tabela
//     }
// }


#include <iikit.h>      // Biblioteca base do framework Arduino
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

// 2) Configurações para o DMA_ADC
#define BLOCK_SIZE 512  // 512 amostras
AdcDmaEsp adcDma;
uint16_t samples[BLOCK_SIZE];

void setup()
{
    IIKit.setup();
    // Mapeamos a senoide (de -1 a 1) para valores de 0 a 255, que o DAC espera.
    for (int i = 0; i < SAMPLES; i++)
    {
        float angle = (2 * PI * i) / SAMPLES;
        float s = sin(angle);
        int dacValue = (int)((s + 1.0) * 70.0f + 100.0f) ; // mapeia: -1 → 25 e 1 → 225
        sineTable[i] = (uint8_t) dacValue;
    }

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

    // OBS: begin() agora aceita: (GPIO, sample_rate)
    bool ok = adcDma.beginGPIO(36,4000,10);
    if (!ok) wserial::println("Falha ao iniciar ADC DMA!");
    else wserial::println("ADC DMA iniciado com sucesso.");
}

void loop()
{
    IIKit.loop();
    size_t n = adcDma.read(samples,BLOCK_SIZE); // Lê o bloco completo (bloqueante)
    if (n > 0)  wserial::plot("adcValue", 1, samples, n); // Envia para o LasecPlot
}