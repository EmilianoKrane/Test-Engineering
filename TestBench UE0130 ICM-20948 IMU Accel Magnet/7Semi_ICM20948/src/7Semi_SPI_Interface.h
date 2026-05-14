#pragma once
#include "7Semi_Interface.h"

/**
 * 7Semi SPI Interface Implementation
 */
class SPI_Interface : public Interface_7Semi
{

public:
    SPIClass *spi = nullptr;
    uint8_t cs = 255;
    uint32_t speed = 1000000;

    /**
     * beginSPI()
     *
     * - Initialize SPI interface
     * - Configure CS and SPI bus
     */
    bool beginSPI(uint8_t csPin,
                  SPIClass &spiPort,
                  uint32_t spiSpeed,
                  uint8_t sck = 255,
                  uint8_t miso = 255,
                  uint8_t mosi = 255) override
    {
        if (csPin == 255)
            return false;

        spi = &spiPort;
        cs = csPin;
        speed = spiSpeed;

        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);

#if defined(ESP32)
        if (sck != 255 && miso != 255 && mosi != 255)
            spi->begin(sck, miso, mosi, cs);
        else
            spi->begin();
#else
        spi->begin();
#endif

        delay(1);
        return true;
    }

    bool beginI2C(uint8_t, TwoWire &, uint32_t,
                  uint8_t, uint8_t) override
    {
        return false;
    }

    /**
     * read()
     *
     * - Read multiple bytes from register
     */
    int8_t read(uint8_t reg, uint8_t *data, uint32_t len) override
    {
        if (!spi || !data || len == 0)
            return -1;

        SPISettings settings(speed, MSBFIRST, SPI_MODE0);

        spi->beginTransaction(settings);
        digitalWrite(cs, LOW);
        delayMicroseconds(10);

        /**
         * Send register address (read)
         */
        spi->transfer(reg | 0x80);

        delayMicroseconds(10);

        /**
         * Read data
         */
        for (uint32_t i = 0; i < len; i++)
            data[i] = spi->transfer(0x00);

        digitalWrite(cs, HIGH);
        delayMicroseconds(10);
        spi->endTransaction();

        // Serial.print("R-Reg: ");
        // Serial.print(reg, HEX);
        // Serial.print(" |Data: ");
        // for (int i = 0; i < len; i++)
        // {
        //     Serial.print(" ");
        //     Serial.print(data[i], HEX);
        // }
        // Serial.println();

        return 0;
    }

    /**
     * write()
     *
     * - Write multiple bytes to register
     */
    int8_t write(uint8_t reg, const uint8_t *data, uint32_t len) override
    {
        if (!spi || !data || len == 0)
            return -1;

        SPISettings settings(speed, MSBFIRST, SPI_MODE0);

        spi->beginTransaction(settings);
        digitalWrite(cs, LOW);
        delayMicroseconds(10);

        /**
         * Send register address (write)
         */
        spi->transfer(reg & 0x7F);

        /**
         * Write data
         */
        for (uint32_t i = 0; i < len; i++)
            spi->transfer(data[i]);

        digitalWrite(cs, HIGH);
        delayMicroseconds(10);
        spi->endTransaction();

        // Serial.print("W-Reg: ");
        // Serial.print(reg, HEX);
        // Serial.print(" |Data: ");
        // for (int i = 0; i < len; i++)
        // {
        //     Serial.print(" ");
        //     Serial.print(data[i], HEX);
        // }
        // Serial.println();

        return 0;
    }
};