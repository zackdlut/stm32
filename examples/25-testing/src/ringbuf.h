/*
 * Byte ring buffer for one producer (an interrupt) and one consumer (main).
 * No hardware access: compiles for the STM32 and for the PC test runner alike.
 */
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *buf;
    uint32_t mask;              /* size - 1; size is a power of two */
    volatile uint32_t head;     /* total bytes ever written; only the producer changes it */
    volatile uint32_t tail;     /* total bytes ever read; only the consumer changes it */
} ringbuf_t;

/* size must be a non-zero power of two; returns false otherwise. */
bool rb_init(ringbuf_t *rb, uint8_t *storage, uint32_t size);
bool rb_put(ringbuf_t *rb, uint8_t byte);       /* false when full */
bool rb_get(ringbuf_t *rb, uint8_t *byte);      /* false when empty */
uint32_t rb_count(const ringbuf_t *rb);

#endif
