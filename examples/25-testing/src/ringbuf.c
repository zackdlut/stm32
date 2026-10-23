#include "ringbuf.h"

#include <stddef.h>

bool rb_init(ringbuf_t *rb, uint8_t *storage, uint32_t size)
{
    if (storage == NULL || size == 0 || (size & (size - 1u)) != 0) {
        return false;
    }
    rb->buf = storage;
    rb->mask = size - 1u;
    rb->head = 0;
    rb->tail = 0;
    return true;
}

/* head and tail run freely and wrap at 2^32; unsigned subtraction still gives
 * the right count, and masking gives the array index. */
uint32_t rb_count(const ringbuf_t *rb)
{
    return rb->head - rb->tail;
}

bool rb_put(ringbuf_t *rb, uint8_t byte)
{
    uint32_t head = rb->head;
    if (head - rb->tail > rb->mask) {
        return false;
    }
    rb->buf[head & rb->mask] = byte;
    rb->head = head + 1u;       /* publish only after the byte is stored */
    return true;
}

bool rb_get(ringbuf_t *rb, uint8_t *byte)
{
    uint32_t tail = rb->tail;
    if (rb->head == tail) {
        return false;
    }
    *byte = rb->buf[tail & rb->mask];
    rb->tail = tail + 1u;
    return true;
}
