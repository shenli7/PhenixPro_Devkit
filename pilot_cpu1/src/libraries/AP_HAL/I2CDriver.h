#pragma once

#include <stdint.h>

#include "AP_HAL_Namespace.h"

class AP_HAL::I2CDriver {
public:
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void setTimeout(uint16_t ms) = 0;
    virtual void setHighSpeed(bool active) = 0;

    /* write: for i2c devices which do not obey register conventions */
    virtual uint8_t write(uint8_t addr, uint8_t len, uint8_t* data) = 0;
    /* writeRegister: write a single 8-bit value to a register */
    virtual uint8_t writeRegister(uint8_t addr, uint8_t reg, uint8_t val) = 0;
    /* writeRegisters: write bytes to contigious registers */
    virtual uint8_t writeRegisters(uint8_t addr, uint8_t reg,
                                   uint8_t len, uint8_t* data) = 0;

    /* read: for i2c devices which do not obey register conventions */
    virtual uint8_t read(uint8_t addr, uint8_t len, uint8_t* data) = 0;
    /* readRegister: read from a device register - writes the register,
     * then reads back an 8-bit value. */
    virtual uint8_t readRegister(uint8_t addr, uint8_t reg, uint8_t* data) = 0;

    /* readRegisters: read contigious device registers - writes the first 
     * register, then reads back multiple bytes */
    virtual uint8_t readRegisters(uint8_t addr, uint8_t reg,
                                  uint8_t len, uint8_t* data) = 0;


    virtual bool do_transfer(uint8_t address, const uint8_t *send,
                             uint32_t send_len, uint8_t *recv,
                             uint32_t recv_len) { return false;}
    virtual uint8_t lockup_count() = 0;
    void ignore_errors(bool b) { _ignore_errors = b; }
    virtual AP_HAL::Semaphore* get_semaphore() = 0;
protected:
    bool _ignore_errors;
};
