#ifndef ICM20948_7SEMI_H
#define ICM20948_7SEMI_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "ICM20948_regs.h"

#include "7Semi_Interface.h"
#include "7Semi_I2C_Interface.h"
#include "7Semi_SPI_Interface.h"
#include "BusIO_7Semi.h"

enum class ICM20948_Op_Mode : uint8_t
{
  MODE_POWER_DOWN = 0x00,
  MODE_SINGLE_MEASUREMENT = 0x01,
  MODE_CONTINUOUS_1 = 0x02,
  MODE_CONTINUOUS_2 = 0x04,
  MODE_CONTINUOUS_3 = 0x06,
  MODE_CONTINUOUS_4 = 0x08,
  MODE_SELF_TEST = 0x10
};

enum class ICM20948_Clock_Source : uint8_t
{
  INTERNAL_20MHZ_0 = 0x00, // Internal oscillator
  AUTO_PLL_1 = 0x01,       // Auto select (PLL if ready)
  AUTO_PLL_2 = 0x02,
  AUTO_PLL_3 = 0x03,
  AUTO_PLL_4 = 0x04,
  AUTO_PLL_5 = 0x05,
  INTERNAL_20MHZ_6 = 0x06, // Internal oscillator
  STOP_CLOCK = 0x07        // Stops clock, timing generator reset
};

enum class ICM20948_Gyro_DLPF : uint8_t
{
  DLPF_196HZ = 0x00, // BW ≈ 196.6 Hz, NBW ≈ 229.8 Hz
  DLPF_151HZ = 0x01, // BW ≈ 151.8 Hz, NBW ≈ 187.6 Hz
  DLPF_119HZ = 0x02, // BW ≈ 119.5 Hz, NBW ≈ 154.3 Hz
  DLPF_51HZ = 0x03,  // BW ≈ 51.2 Hz,  NBW ≈ 73.3 Hz
  DLPF_23HZ = 0x04,  // BW ≈ 23.9 Hz,  NBW ≈ 35.9 Hz
  DLPF_11HZ = 0x05,  // BW ≈ 11.6 Hz,  NBW ≈ 17.8 Hz
  DLPF_5HZ = 0x06,   // BW ≈ 5.7 Hz,   NBW ≈ 8.9 Hz
  DLPF_361HZ = 0x07  // BW ≈ 361.4 Hz, NBW ≈ 376.5 Hz
};

enum class ICM20948_Gyro_FullScale : uint8_t
{
  DPS_250 = 0x00,  // ±250 dps
  DPS_500 = 0x01,  // ±500 dps
  DPS_1000 = 0x02, // ±1000 dps
  DPS_2000 = 0x03  // ±2000 dps
};

enum class ICM20948_Accel_FullScale : uint8_t
{
  G_2 = 0x00, // ±2g
  G_4 = 0x01, // ±4g
  G_8 = 0x02, // ±8g
  G_16 = 0x03 // ±16g
};

enum class ICM20948_Accel_Average : uint8_t
{
  AVG_1_OR_4 = 0x00, // Depends on ACCEL_FCHOICE
  AVG_8 = 0x01,      // Average 8 samples
  AVG_16 = 0x02,     // Average 16 samples
  AVG_32 = 0x03      // Average 32 samples
};

/**
 * Class
 * - Manages ICM-20948 over I2C (SPI path disabled in this cut)
 * - Caches scale factors for LSB->physical conversions
 */
class ICM20948_7Semi
{
public:

  ICM20948_7Semi();

  /**
   * beginI2C
   *
   * - Initialize ICM20948 using I2C interface
   * - Sets up communication layer (Interface + BusIO)
   * - Verifies device identity (WHO_AM_I)
   * - Configures basic power and clock settings
   *
   * Returns:
   * - true  → Device initialized successfully
   * - false → Communication or configuration failed
   */
  bool beginI2C(uint8_t address = 0x69, TwoWire &i2cPort = Wire, uint32_t i2cSpeed = 400000);

  bool beginSPI(uint8_t csPin, SPIClass &spiPort = SPI, uint32_t spiSpeed = 1000000);

  /**
   * readWhoAmI
   *
   * - Read WHO_AM_I register (device ID)
   * - Used during initialization to verify ICM20948
   * - Expected value: 0xEA
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool readWhoAmI(uint8_t &whoAmI);

  /**
   * selectBank
   *
   * - Select USER BANK (0–3)
   * - Used to access different register groups inside ICM20948
   *
   * Returns:
   * - true  → Bank switch successful
   * - false → Write failed
   */
  bool selectBank(uint8_t bank);

  /**
   * softReset
   *
   * - Perform software reset of ICM20948
   * - Resets all internal registers to default state
   *
   * Flow:
   * - Select USER BANK 0
   * - Set DEVICE_RESET bit
   * - Wait for device to restart
   *
   * Returns:
   * - true  → Reset successful
   * - false → Operation failed
   */
  bool softReset();

  /**
   * sleep
   *
   * - Enable or disable sleep mode
   * - Maintains clock source (CLKSEL = 1)
   *
   * Flow:
   * - Select USER BANK 0
   * - Set or clear SLEEP bit
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool sleep(bool en);

  /**
   * applyBasicDefaults
   *
   * - Apply basic sensor configuration
   * - Configure power, filters, ranges, and sampling rates
   *
   * Returns:
   * - true  → Configuration successful
   * - false → Any register write failed
   */
  bool applyBasicDefaults();

  /**
   * readAccel
   *
   * - Read accelerometer data (X, Y, Z)
   * - Convert raw values to g using LSB scale
   *
   * Returns:
   * - true  → Read successful
   * - false → Read failed
   */
  bool readAccel(float &x, float &y, float &z);

  /**
   * readGyro
   *
   * - Read gyroscope data (X, Y, Z)
   * - Convert raw values to dps using LSB scale
   *
   * Returns:
   * - true  → Read successful
   * - false → Read failed
   */
  bool readGyro(float &x, float &y, float &z);

  /**
   * readTemperature
   *
   * - Read temperature sensor
   * - Convert raw values to °C
   *
   * Returns:
   * - true  → Read successful
   * - false → Read failed
   */
  bool readTemperature(float &temperature);

  /**
   * readMag
   *
   * - Read magnetometer data via internal I2C master
   * - Data comes from EXT_SLV_SENS_DATA registers
   * - Convert raw values to µT
   *
   * Returns:
   * - true  → Read successful
   * - false → Read failed
   */
  bool readMag(float &x, float &y, float &z);

  /**
   * initMag
   *
   * - Initialize AK09916 magnetometer using internal I2C master
   * - Verifies WHO_AM_I and enables continuous mode
   * - Configures SLV0 for automatic data read
   *
   * Returns:
   * - true  → Initialization successful
   * - false → Operation failed
   */
  bool initMag();

  /**
   * setMagOpMode
   *
   * - Set magnetometer operation mode
   *
   * Returns:
   * - true  → Mode set successfully
   * - false → Write failed
   */
  bool setMagOpMode(ICM20948_Op_Mode opMode);

  /**
   * setLowPower
   *
   * - Enable or disable low power mode
   * - Controls LP_EN bit in PWR_MGMT_1
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setLowPower(bool enable);

  /**
   * getLowPower
   *
   * - Read low power mode status
   * - Returns state of LP_EN bit
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getLowPower(bool &enable);

  /**
   * setClock
   *
   * - Set clock source (CLKSEL [2:0])
   * - Selects internal clock for sensor operation
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setClock(ICM20948_Clock_Source clock);

  /**
   * getClock
   *
   * - Read current clock source (CLKSEL [2:0])
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getClock(uint8_t &clock);

  /**
   * setGyroSampleRate
   *
   * - Set gyroscope sample rate
   * - Uses internal divider (SRD) based on 1100 Hz base rate
   *
   * Returns:
   * - true  → Operation successful
   * - false → Invalid input or write failed
   */
  bool setGyroSampleRate(float sampleRate);

  /**
   * getGyroSampleRate
   *
   * - Get current gyroscope sample rate
   * - Computes value from divider register
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getGyroSampleRate(float &sampleRate);

  /**
   * setDLPF
   *
   * - Configure gyroscope digital low-pass filter (DLPF)
   * - Option to bypass filter (full bandwidth)
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setDLPF(ICM20948_Gyro_DLPF dlpf, bool bypass);

  /**
   * getDLPF
   *
   * - Read gyroscope DLPF configuration
   * - Returns filter setting and bypass state
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getDLPF(uint8_t &dlpf, bool &bypass);

  /**
   * setGyroScale
   *
   * - Set gyroscope full-scale range (FS_SEL)
   * - Controls sensitivity (dps range)
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Write FS_SEL bits [2:1]
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setGyroScale(ICM20948_Gyro_FullScale fullScale);

  /**
   * getGyroScale
   *
   * - Get current gyroscope full-scale range (FS_SEL
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getGyroScale(uint8_t &fullScale);

  /**
   * selfTestGyro
   *
   * - Enable or disable gyroscope self-test per axis
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool selfTestGyro(bool x, bool y, bool z);

  /**
   * setAccelSampleRate
   *
   * - Set accelerometer output data rate (ODR)
   * - Uses 1125 Hz base rate with 12-bit divider
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setAccelSampleRate(uint16_t sampleRate);

  /**
   * getAccelSampleRate
   *
   * - Get accelerometer output data rate (ODR)
   * - Computes value from divider registers
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getAccelSampleRate(float &sampleRate);

  /**
   * selfTestAccel
   *
   * - Enable or disable accelerometer self-test per axis
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool selfTestAccel(bool x, bool y, bool z);

  /**
   * setAccelScale
   *
   * - Set accelerometer full-scale range (FS_SEL)
   * - Controls measurement range (±2g, ±4g, ±8g, ±16g)
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setAccelScale(ICM20948_Accel_FullScale fullScale);

  /**
   * getAccelScale
   *
   * - Get current accelerometer full-scale range (FS_SEL)
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getAccelScale(uint8_t &fullScale);

  /**
   * setAccelDLPF
   *
   * - Configure accelerometer digital low-pass filter (DLPF)
   * - Option to bypass filter (full bandwidth)
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Set or clear bypass bit
   * - Configure DLPF bits if not bypassed
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setAccelDLPF(uint8_t dlpf, bool bypass);

  /**
   * getAccelDLPF
   *
   * - Read accelerometer DLPF configuration
   * - Returns filter setting and bypass state
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Read ACCEL_CONFIG register
   * - Extract bypass and DLPF bits
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getAccelDLPF(uint8_t &dlpf, bool &bypass);

  /**
   * setAccelAveraging
   *
   * - Configure accelerometer averaging / decimation (DEC3)
   * - Controls internal averaging of accel samples
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Write DEC3 bits [1:0]
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setAccelAveraging(ICM20948_Accel_Average avg);

  /**
   * getAccelAveraging
   *
   * - Read accelerometer averaging / decimation setting (DEC3)
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Read DEC3 bits [1:0]
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getAccelAveraging(uint8_t &avg);

  /**
   * setSensors
   *
   * - Enable or disable accel, gyro, and temperature sensor
   * - Controls power gating via PWR_MGMT_2 and TEMP_DIS
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 0
   * - Build PWR_MGMT_2 mask
   * - Configure temperature enable/disable
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setSensors(bool accel_on, bool gyro_on, bool temp_on);

  /**
   * getSensors
   *
   * - Read accel, gyro, and temperature enable state
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 0
   * - Read PWR_MGMT_2 register
   * - Read TEMP_DIS bit
   * - Decode enable states
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getSensors(bool &accel_on, bool &gyro_on, bool &temp_on);

  /**
   * setGyroOffset
   *
   * - Set gyroscope offset values for X, Y, Z axes
   * - Writes offset registers in USER BANK 2
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Pack offsets into byte array
   * - Write to offset registers
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setGyroOffset(uint16_t offsetX, uint16_t offsetY, uint16_t offsetZ);

  /**
   * getGyroOffset
   *
   * - Read gyroscope offset values for X, Y, Z axes
   * - Reads offset registers from USER BANK 2
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 2
   * - Read offset registers
   * - Convert to signed values
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getGyroOffset(int16_t &offsetX, int16_t &offsetY, int16_t &offsetZ);

  /**
   * setAccelOffset
   *
   * - Set accelerometer offset values for X, Y, Z axes
   * - Writes offset registers in USER BANK 1
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 1
   * - Pack offsets into byte array
   * - Write to offset registers
   *
   * Returns:
   * - true  → Operation successful
   * - false → Operation failed
   */
  bool setAccelOffset(int16_t offsetX, int16_t offsetY, int16_t offsetZ);

  /**
   * getAccelOffset
   *
   * - Read accelerometer offset values for X, Y, Z axes
   * - Reads offset registers from USER BANK 1
   *
   * Flow:
   * - Validate bus pointer
   * - Select USER BANK 1
   * - Read offset registers
   * - Convert to signed values
   *
   * Returns:
   * - true  → Read successful
   * - false → Operation failed
   */
  bool getAccelOffset(int16_t &offsetX, int16_t &offsetY, int16_t &offsetZ);

private:
  I2C_Interface i2c;
  SPI_Interface spi;

  Interface_7Semi *iface = nullptr;

  BusIO_7Semi<Interface_7Semi> *bus = nullptr;

  float mg_per_lsb = 16384.0f;        // LSB/g at ±16g
  float degree_per_second = 131.072f; // LSB/dps at ±2000 dps

  bool writeSlave4(uint8_t reg, uint8_t value);
  bool readSlave4(uint8_t reg, uint8_t &value);
};

#endif /* ICM20948_7SEMI_H */
