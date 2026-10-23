#include "i2c.h"

#include "board.h"

#define TIMEOUT 100000u

/* Spin until (SR1 & flag) is set. Gives up on NACK (AF) or after TIMEOUT polls. */
static int wait_sr1(uint32_t flag)
{
    for (uint32_t n = 0; n < TIMEOUT; n++) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & flag) {
            return I2C_OK;
        }
        if (sr1 & I2C_SR1_AF) {
            I2C1->SR1 = ~I2C_SR1_AF;          /* rc_w0 */
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERR_NACK;
        }
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_ERR_TIMEOUT;
}

void i2c1_init(uint32_t pclk1_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6 = SCL, PB7 = SDA: AF4, open-drain. The bus needs pull-ups (the
     * internal ~40k ones are weak; real boards use 2.2k-10k resistors). */
    for (uint32_t pin = 6; pin <= 7; pin++) {
        gpio_set_af(GPIOB, pin, 4);
        GPIOB->OTYPER |= 1u << pin;
        gpio_set_pull(GPIOB, pin, GPIO_PULL_UP);
    }

    I2C1->CR1 = I2C_CR1_SWRST;               /* clear any stuck state */
    I2C1->CR1 = 0;
    uint32_t mhz = pclk1_hz / 1000000u;
    I2C1->CR2 = mhz;                          /* FREQ: APB1 clock in MHz */
    I2C1->CCR = pclk1_hz / (2u * 100000u);    /* SCL high = low = CCR x Tpclk */
    I2C1->TRISE = mhz + 1u;                   /* 1000 ns max rise time / Tpclk + 1 */
    I2C1->CR1 = I2C_CR1_PE;
}

static int start(uint8_t addr7, int read)
{
    /* Hardware clears STOP once the stop condition is on the bus (a few us).
     * Starting again before that would be ignored. Renode never clears it, so
     * after a short bounded wait drop it by hand. */
    for (uint32_t n = 0; (I2C1->CR1 & I2C_CR1_STOP) && n < 1000u; n++) {
    }
    I2C1->CR1 = (I2C1->CR1 & ~I2C_CR1_STOP) | I2C_CR1_START;
    int rc = wait_sr1(I2C_SR1_SB);            /* start condition sent */
    if (rc) {
        return rc;
    }
    I2C1->DR = (uint8_t)((addr7 << 1) | (read ? 1u : 0u));
    rc = wait_sr1(I2C_SR1_ADDR);              /* address acknowledged */
    return rc;
}

static void clear_addr(void)
{
    (void)I2C1->SR1;                          /* ADDR clears on SR1 then SR2 read */
    (void)I2C1->SR2;
}

int i2c1_probe(uint8_t addr7)
{
    int rc = start(addr7, 0);
    if (rc == I2C_OK) {
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;
    }
    return rc;
}

int i2c1_write(uint8_t addr7, const uint8_t *data, size_t len)
{
    int rc = start(addr7, 0);
    if (rc) {
        return rc;
    }
    clear_addr();
    for (size_t i = 0; i < len; i++) {
        if ((rc = wait_sr1(I2C_SR1_TXE)) != I2C_OK) {
            return rc;
        }
        I2C1->DR = data[i];
    }
    rc = wait_sr1(I2C_SR1_BTF);               /* last byte fully shifted out */
    I2C1->CR1 |= I2C_CR1_STOP;
    return rc;
}

/*
 * Reading needs care on this I2C generation: the master ACKs each byte while
 * CR1.ACK is set and the slave keeps sending until it sees a NACK. ACK has to
 * be cleared, and STOP requested, while the *last* byte is still on the wire.
 * BTF (data register and shift register both full, SCL held low) gives a safe
 * point to do that. These are the three sequences in the "master receiver" part of RM0090's I2C chapter.
 */
int i2c1_read(uint8_t addr7, uint8_t *data, size_t len)
{
    int rc;

    if (len == 0) {
        return I2C_OK;
    }

    if (len == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if ((rc = start(addr7, 1)) != I2C_OK) {
            return rc;
        }
        clear_addr();                         /* byte 1 starts: it will be NACKed */
        I2C1->CR1 |= I2C_CR1_STOP;
        if ((rc = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
            return rc;
        }
        data[0] = (uint8_t)I2C1->DR;
        return I2C_OK;
    }

    if (len == 2) {
        I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_POS;   /* POS: ACK/NACK applies to the *next* byte */
        if ((rc = start(addr7, 1)) != I2C_OK) {
            I2C1->CR1 &= ~I2C_CR1_POS;
            return rc;
        }
        clear_addr();
        I2C1->CR1 &= ~I2C_CR1_ACK;            /* byte 2 will be NACKed */
        rc = wait_sr1(I2C_SR1_BTF);           /* byte 1 in DR, byte 2 in shift reg */
        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;
        I2C1->CR1 &= ~I2C_CR1_POS;
        return rc;
    }

    I2C1->CR1 |= I2C_CR1_ACK;
    if ((rc = start(addr7, 1)) != I2C_OK) {
        return rc;
    }
    clear_addr();
    size_t i = 0;
    for (; i < len - 3; i++) {                /* all but the last three bytes */
        if ((rc = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
            return rc;
        }
        data[i] = (uint8_t)I2C1->DR;
    }
    if ((rc = wait_sr1(I2C_SR1_BTF)) != I2C_OK) {   /* N-2 in DR, N-1 in shift reg */
        return rc;
    }
    I2C1->CR1 &= ~I2C_CR1_ACK;                /* byte N will be NACKed */
    data[i++] = (uint8_t)I2C1->DR;            /* N-2 */
    I2C1->CR1 |= I2C_CR1_STOP;
    data[i++] = (uint8_t)I2C1->DR;            /* N-1 */
    if ((rc = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
        return rc;
    }
    data[i] = (uint8_t)I2C1->DR;              /* N */
    return I2C_OK;
}
