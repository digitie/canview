/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "canview_gpio.h"
#include <stdio.h>
#include <stddef.h>
#if CANVIEW_TEST_BOARD == 3
#include "safe_gpio.h"
#endif

typedef struct
{
    uint8_t port;
    uint8_t pin;
    bool high;
    bool open_drain;
    bool input;
} call_t;
static call_t calls[16];
static size_t count;
static size_t fail_at;

static canview_status_t record(uint8_t port, uint8_t pin, bool high, bool od, bool input)
{
    if (count >= sizeof(calls) / sizeof(calls[0]))
    {
        return CANVIEW_OVERSIZE;
    }
    calls[count].port = port;
    calls[count].pin = pin;
    calls[count].high = high;
    calls[count].open_drain = od;
    calls[count].input = input;
    ++count;
    return count == fail_at ? CANVIEW_INVALID_ARGUMENT : CANVIEW_OK;
}
canview_status_t canview_gpio_output(uint8_t pin, bool high, bool open_drain)
{
    return record(0U, pin, high, open_drain, false);
}
canview_status_t canview_gpio_input(uint8_t pin)
{
    return record(0U, pin, false, false, true);
}
void canview_platform_idle(void *context)
{
    (void)context;
}
#if CANVIEW_TEST_BOARD == 3
canview_status_t canview_stm_output(uint8_t port, uint8_t pin, bool high)
{
    return record(port, pin, high, false, false);
}
void canview_stm_idle(void *context)
{
    (void)context;
}
#endif
int main(void)
{
#if CANVIEW_TEST_BOARD == 0
    const call_t expected[] = {{0, 6, false, false, false}, {0, 41, false, false, true}};
#elif CANVIEW_TEST_BOARD == 1
    const call_t expected[] = {{0, 7, false, false, false},  {0, 2, false, false, false},
                               {0, 47, false, false, false}, {0, 1, true, false, false},
                               {0, 9, true, true, false},    {0, 8, false, false, true}};
#elif CANVIEW_TEST_BOARD == 2
    const call_t expected[] = {{0, 5, false, false, false}, {0, 4, false, false, true}};
#else
    const call_t expected[] = {{0, 4, true, false, false},  {0, 5, true, false, false},
                               {0, 6, false, false, false}, {0, 7, false, false, false},
                               {1, 0, false, false, false}, {0, 12, true, false, false},
                               {1, 13, true, false, false}, {0, 15, true, false, false}};
#endif
    const size_t length = sizeof(expected) / sizeof(expected[0]);
    const canview_platform_port_t port = canview_board_port();
    if (port.enter_safe_state == NULL || port.idle == NULL)
    {
        return 1;
    }
    for (size_t fault = 0U; fault <= length; ++fault)
    {
        count = 0U;
        fail_at = fault;
        const canview_status_t status = port.enter_safe_state(port.context);
        if ((fault == 0U && (status != CANVIEW_OK || count != length)) ||
            (fault != 0U && (status != CANVIEW_INVALID_ARGUMENT || count != fault)))
        {
            return 1;
        }
        for (size_t index = 0U; index < count; ++index)
        {
            const call_t *a = &calls[index], *b = &expected[index];
            if (a->port != b->port || a->pin != b->pin || a->high != b->high ||
                a->open_drain != b->open_drain || a->input != b->input)
            {
                return 1;
            }
        }
    }
    (void)puts("PASS: BSP order, safe levels, open-drain and every GPIO failure");
    return 0;
}
