/*
 * Single-producer / single-consumer ring buffer.
 *
 * The interrupt handler only writes `head`, main() only writes `tail`, so no
 * locking is needed as long as each index is updated with one aligned 32-bit
 * store (which Cortex-M guarantees) and the size is a power of two.
 */
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stdint.h>

#define RINGBUF_SIZE 64u
_Static_assert((RINGBUF_SIZE & (RINGBUF_SIZE - 1u)) == 0, "size must be a power of two");

typedef struct {
    uint8_t data[RINGBUF_SIZE];
    volatile uint32_t head;   /* next slot to write (producer) */
    volatile uint32_t tail;   /* next slot to read  (consumer) */
} ringbuf_t;

/* Free-running indices: head - tail is the fill level even after wrap-around. */
static inline uint32_t ringbuf_count(const ringbuf_t *rb)
{
    return rb->head - rb->tail;
}

static inline bool ringbuf_put(ringbuf_t *rb, uint8_t byte)
{
    if (ringbuf_count(rb) == RINGBUF_SIZE) {
        return false;                                  /* full: drop the byte */
    }
    rb->data[rb->head & (RINGBUF_SIZE - 1u)] = byte;
    __asm__ volatile("" ::: "memory");                 /* data before index */
    rb->head++;
    return true;
}

static inline bool ringbuf_get(ringbuf_t *rb, uint8_t *byte)
{
    if (ringbuf_count(rb) == 0) {
        return false;
    }
    *byte = rb->data[rb->tail & (RINGBUF_SIZE - 1u)];
    __asm__ volatile("" ::: "memory");
    rb->tail++;
    return true;
}

#endif
