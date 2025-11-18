#include <Arduino.h>
#include <IIKit.h>
#include "AdcDmaEsp_Continuous.h"

AdcDmaEsp_Continuous adcDma;
static const uint32_t SAMPLE_RATE = 2000;   // 2 kHz
static const size_t   BLOCK_SIZE  = 512;    // 512 amostras
uint16_t samples[BLOCK_SIZE];

void setup() {
    IIKit.setup();

    // OBS: beginGPIO() aceita: (GPIO_PIN, sample_rate, block_size, callback)
    bool ok = adcDma.beginGPIO(
        def_pin_ADC1,
        SAMPLE_RATE,
        BLOCK_SIZE,
        nullptr              // sem callback
    );

    if (!ok) wserial::println("Falha ao iniciar ADC DMA!");
    else wserial::println("ADC DMA iniciado com sucesso.");
}

void loop() {
    IIKit.loop();
    size_t n = adcDma.readBlock(samples); // Lê o bloco completo (bloqueante)
    if (n > 0)  wserial::plot("adcValue", 10, samples, n); // Envia para o LasecPlot
}
