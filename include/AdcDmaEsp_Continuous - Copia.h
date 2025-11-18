#pragma once
#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/adc.h"

class AdcDmaEsp_Continuous {
public:
    typedef void (*BlockCallback)(uint16_t*, size_t);

    AdcDmaEsp_Continuous() {}

    // ============================================================
    // 1) NOVA FUNÇÃO — CONFIGURAÇÃO VIA GPIO AUTOMÁTICO
    // ============================================================
    bool beginGPIO(int gpio, uint32_t sample_rate, size_t block_size, BlockCallback cb = nullptr)
    {
        adc1_channel_t ch;

        switch (gpio)
        {
            case 36: ch = ADC1_CHANNEL_0; break;
            case 37: ch = ADC1_CHANNEL_1; break;
            case 38: ch = ADC1_CHANNEL_2; break;
            case 39: ch = ADC1_CHANNEL_3; break;
            case 32: ch = ADC1_CHANNEL_4; break;
            case 33: ch = ADC1_CHANNEL_5; break;
            case 34: ch = ADC1_CHANNEL_6; break;
            case 35: ch = ADC1_CHANNEL_7; break;

            default:
                // GPIO não é ADC1
                return false;
        }

        return begin(ch, sample_rate, block_size, cb);
    }

    // ============================================================
    // 2) Função begin REAL (internamente usa ADC1_CHANNEL_x)
    // ============================================================
    bool begin(adc1_channel_t channel,
               uint32_t sample_rate,
               size_t block_size,
               BlockCallback cb = nullptr)
    {
        _channel     = channel;
        _sample_rate = sample_rate;
        _block_size  = block_size;
        _callback    = cb;

        // Configura ADC1
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(_channel, ADC_ATTEN_DB_11);

        // Configura I2S
        i2s_config_t cfg = {};
        cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
        cfg.sample_rate = sample_rate;
        cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        cfg.dma_buf_count = 4;
        cfg.dma_buf_len = block_size;
        cfg.use_apll = false;

        if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK)
            return false;

        if (i2s_set_adc_mode(ADC_UNIT_1, _channel) != ESP_OK)
            return false;

        if (i2s_adc_enable(I2S_NUM_0) != ESP_OK)
            return false;

        return true;
    }

    // ============================================================
    // 3) LÊ BLOCO COMPLETO
    // ============================================================
    size_t readBlock(uint16_t* out)
    {
        size_t bytes_read = 0;

        esp_err_t err = i2s_read(
            I2S_NUM_0,
            out,
            _block_size * sizeof(uint16_t),
            &bytes_read,
            portMAX_DELAY
        );

        if (err != ESP_OK || bytes_read == 0)
            return 0;

        size_t samples = bytes_read / sizeof(uint16_t);

        for (size_t i = 0; i < samples; i++)
            out[i] &= 0x0FFF;  // 12 bits úteis

        if (_callback)
            _callback(out, samples);

        return samples;
    }

    void end()
    {
        i2s_adc_disable(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_0);
    }

private:
    adc1_channel_t _channel;
    uint32_t       _sample_rate;
    size_t         _block_size;
    BlockCallback  _callback = nullptr;
};