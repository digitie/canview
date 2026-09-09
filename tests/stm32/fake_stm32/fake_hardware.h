/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_TEST_FAKE_HARDWARE_H
#define CANVIEW_TEST_FAKE_HARDWARE_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g474xx.h"

extern uint32_t fake_now_us;
extern uint32_t fake_output_calls;
extern uint32_t fake_output_fail_call;
extern uint32_t fake_output_fail_call_2;
extern uint32_t fake_wait_calls;
extern uint32_t fake_wait_fail_call;
extern uint32_t fake_wait_poll_mismatch_call;
extern bool fake_outputs[2][16];

void fake_hardware_reset(void);
void fake_hardware_ready(void);
bool canview_stm_fdcan_test_wait_should_timeout(void);

#endif
