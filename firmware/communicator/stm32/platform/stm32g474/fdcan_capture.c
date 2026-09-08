/* SPDX-License-Identifier: GPL-3.0-only */
#include "fdcan_capture.h"

#include "board_pins.h"
#include "core_hw.h"
#include "safe_gpio.h"
#include "stm32g474xx.h"

#include <string.h>

#if defined(CANVIEW_STM_FDCAN_TEST)
extern bool canview_stm_fdcan_test_wait_should_timeout(void);
#endif

#define CANVIEW_STM_FDCAN_POLL_LIMIT (UINT32_C(100000))
#define CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES (UINT32_C(848))
#define CANVIEW_STM_FDCAN_RX_FIFO0_OFFSET_BYTES (UINT32_C(176))
#define CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_BYTES (UINT32_C(72))
#define CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_COUNT (UINT32_C(3))
#define CANVIEW_STM_FDCAN_GPIO_MODE_ALTERNATE (UINT32_C(2))
#define CANVIEW_STM_FDCAN_AF_HIGH_SPEED (UINT32_C(9))
#define CANVIEW_STM_FDCAN_AF_FAULT_TOLERANT (UINT32_C(11))
#define CANVIEW_STM_FDCAN_TIMESTAMP_PRESCALER                                             \
    (CANVIEW_STM_SYSCLK_HZ / UINT32_C(1000000) - 1U)

#if CANVIEW_BOARD_CAN1_RX_PORT != 0U || CANVIEW_BOARD_CAN1_RX_PIN != 11U ||                  \
    CANVIEW_BOARD_CAN1_TX_REQ_PORT != 0U || CANVIEW_BOARD_CAN1_TX_REQ_PIN != 12U ||          \
    CANVIEW_BOARD_CAN2_RX_PORT != 1U || CANVIEW_BOARD_CAN2_RX_PIN != 12U ||                  \
    CANVIEW_BOARD_CAN2_TX_REQ_PORT != 1U || CANVIEW_BOARD_CAN2_TX_REQ_PIN != 13U ||          \
    CANVIEW_BOARD_CAN3_RX_PORT != 0U || CANVIEW_BOARD_CAN3_RX_PIN != 8U ||                   \
    CANVIEW_BOARD_CAN3_TX_REQ_PORT != 0U || CANVIEW_BOARD_CAN3_TX_REQ_PIN != 15U
#error Invalid_R1_FDCAN_pin_contract
#endif

#define CANVIEW_STM_FDCAN_RX_INTERRUPTS                                                   \
    (FDCAN_IR_RF0N | FDCAN_IR_RF0F | FDCAN_IR_RF0L)
#define CANVIEW_STM_FDCAN_STATUS_INTERRUPTS                                               \
    (FDCAN_IR_RF0F | FDCAN_IR_RF0L | FDCAN_IR_EP | FDCAN_IR_EW | FDCAN_IR_BO | FDCAN_IR_MRAF | \
     FDCAN_IR_PEA |                                                                            \
     FDCAN_IR_PED)
#define CANVIEW_STM_FDCAN_INTERRUPT_MASK                                                  \
    (CANVIEW_STM_FDCAN_RX_INTERRUPTS | CANVIEW_STM_FDCAN_STATUS_INTERRUPTS)
#define CANVIEW_STM_FDCAN_ENABLE_INTERRUPTS                                               \
    (FDCAN_IE_RF0NE | FDCAN_IE_RF0FE | FDCAN_IE_RF0LE | FDCAN_IE_EPE | FDCAN_IE_EWE |        \
     FDCAN_IE_BOE | FDCAN_IE_MRAFE | FDCAN_IE_PEAE | FDCAN_IE_PEDE)

typedef char canview_stm_fdcan_message_ram_fits[
    (CANVIEW_STM_FDCAN_RX_FIFO0_OFFSET_BYTES +
         CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_COUNT * CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_BYTES <=
     CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES)
        ? 1
        : -1];
typedef char canview_stm_fdcan_wire_channel_count_matches[
    (CANVIEW_STM_FDCAN_CHANNEL_COUNT == CANVIEW_WIRE_CAN_BUS_COUNT) ? 1 : -1];
typedef char canview_stm_fdcan_raw_ring_counter_is_unambiguous[
    (CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY <= (UINT8_MAX / 2U)) ? 1 : -1];

typedef struct
{
    GPIO_TypeDef *port;
    uint8_t pin;
    uint8_t alternate;
    uint32_t clock_mask;
} canview_stm_fdcan_gpio_t;

static const canview_stm_fdcan_gpio_t rx_pins[CANVIEW_STM_FDCAN_CHANNEL_COUNT] = {
    {GPIOA, UINT8_C(11), (uint8_t)CANVIEW_STM_FDCAN_AF_HIGH_SPEED, RCC_AHB2ENR_GPIOAEN},
    {GPIOB, UINT8_C(12), (uint8_t)CANVIEW_STM_FDCAN_AF_HIGH_SPEED, RCC_AHB2ENR_GPIOBEN},
    {GPIOA, UINT8_C(8), (uint8_t)CANVIEW_STM_FDCAN_AF_FAULT_TOLERANT, RCC_AHB2ENR_GPIOAEN}};

static FDCAN_GlobalTypeDef *const instances[CANVIEW_STM_FDCAN_CHANNEL_COUNT] = {
    FDCAN1, FDCAN2, FDCAN3};

/* STM32에는 FDCAN adapter가 하나만 존재한다. IRQ vector는 이 singleton만 참조한다. */
static canview_stm_fdcan_platform_t *volatile active_platform;

static void reset_runtime_state(canview_stm_fdcan_platform_t *platform)
{
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        platform->raw_read_index[index] = 0U;
        platform->raw_write_index[index] = 0U;
        platform->raw_drops[index] = 0U;
        platform->reported_raw_drops[index] = 0U;
        platform->pending_interrupts[index] = 0U;
        platform->fifo_loss_unknown[index] = false;
        platform->message_ram_fault[index] = false;
        platform->raw_ring_overflow[index] = false;
        platform->session_fault_flags[index] = 0U;
        platform->bus_off_count[index] = 0U;
        platform->sink_failures[index] = 0U;
        platform->started[index] = false;
        platform->previous_state[index] = platform->config.profiles[index].enabled
                                               ? CANVIEW_STM_FDCAN_BUS_NO_DATA
                                               : CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE;
    }
    platform->servicing = false;
}

typedef struct
{
    uint8_t port;
    uint8_t pin;
    bool high;
} canview_stm_fdcan_output_t;

static canview_status_t set_capture_outputs(
    const canview_stm_fdcan_profile_t profiles[CANVIEW_STM_FDCAN_CHANNEL_COUNT], bool receive_enabled)
{
    const canview_stm_fdcan_output_t outputs[] = {
        {CANVIEW_BOARD_STB1_REQ_PORT, CANVIEW_BOARD_STB1_REQ_PIN,
         !receive_enabled || !profiles[0].enabled},
        {CANVIEW_BOARD_STB2_REQ_PORT, CANVIEW_BOARD_STB2_REQ_PIN,
         !receive_enabled || !profiles[1].enabled},
        {CANVIEW_BOARD_FT_EN_REQ_PORT, CANVIEW_BOARD_FT_EN_REQ_PIN,
         receive_enabled && profiles[2].enabled},
        {CANVIEW_BOARD_CAN1_TX_REQ_PORT, CANVIEW_BOARD_CAN1_TX_REQ_PIN, true},
        {CANVIEW_BOARD_CAN2_TX_REQ_PORT, CANVIEW_BOARD_CAN2_TX_REQ_PIN, true},
        {CANVIEW_BOARD_CAN3_TX_REQ_PORT, CANVIEW_BOARD_CAN3_TX_REQ_PIN, true}};
    canview_status_t result = CANVIEW_OK;
    for (size_t index = 0U; index < sizeof(outputs) / sizeof(outputs[0]); ++index)
    {
        const canview_status_t status =
            canview_stm_output(outputs[index].port, outputs[index].pin, outputs[index].high);
        if (status != CANVIEW_OK && result == CANVIEW_OK)
        {
            result = status;
        }
    }
    return result;
}

static bool timestamp_clock_ready(void)
{
    return (RCC->CR & (RCC_CR_HSERDY | RCC_CR_PLLRDY)) ==
               (RCC_CR_HSERDY | RCC_CR_PLLRDY) &&
           (RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL &&
           (TIM2->CR1 & TIM_CR1_CEN) != 0U && TIM2->PSC == CANVIEW_STM_FDCAN_TIMESTAMP_PRESCALER &&
           TIM2->ARR == UINT32_MAX;
}

static bool wait_register(volatile const uint32_t *reg, uint32_t mask, uint32_t wanted)
{
#if defined(CANVIEW_STM_FDCAN_TEST)
    if (canview_stm_fdcan_test_wait_should_timeout())
    {
        return false;
    }
#endif
    for (uint32_t attempt = 0U; attempt < CANVIEW_STM_FDCAN_POLL_LIMIT; ++attempt)
    {
        if ((*reg & mask) == wanted)
        {
            return true;
        }
    }
    return false;
}

static uint32_t nbtp_value(const canview_stm_fdcan_timing_t *timing)
{
    return (((uint32_t)timing->sync_jump_width - 1U) << FDCAN_NBTP_NSJW_Pos) |
           (((uint32_t)timing->time_segment1 - 1U) << FDCAN_NBTP_NTSEG1_Pos) |
           (((uint32_t)timing->prescaler - 1U) << FDCAN_NBTP_NBRP_Pos) |
           (((uint32_t)timing->time_segment2 - 1U) << FDCAN_NBTP_NTSEG2_Pos);
}

static void configure_rx_pin(const canview_stm_fdcan_gpio_t *pin)
{
    const uint32_t mode_shift = (uint32_t)pin->pin * 2U;
    const uint32_t mode_mask = UINT32_C(3) << mode_shift;
    const uint32_t alternate_shift = ((uint32_t)pin->pin % 8U) * 4U;
    const uint32_t alternate_mask = UINT32_C(0xf) << alternate_shift;
    const size_t alternate_index = (size_t)(pin->pin / UINT8_C(8));

    pin->port->AFR[alternate_index] =
        (pin->port->AFR[alternate_index] & ~alternate_mask) |
        ((uint32_t)pin->alternate << alternate_shift);
    pin->port->PUPDR &= ~mode_mask;
    pin->port->MODER = (pin->port->MODER & ~mode_mask) |
                       (CANVIEW_STM_FDCAN_GPIO_MODE_ALTERNATE << mode_shift);
}

static void disable_instance(FDCAN_GlobalTypeDef *instance)
{
    instance->IE = 0U;
    instance->ILE = 0U;
    instance->IR = CANVIEW_STM_FDCAN_INTERRUPT_MASK;
    instance->CCCR |= FDCAN_CCCR_INIT;
}

static bool configure_instance(FDCAN_GlobalTypeDef *instance,
                               const canview_stm_fdcan_profile_t *profile)
{
    instance->CCCR |= FDCAN_CCCR_INIT;
    if (!wait_register(&instance->CCCR, FDCAN_CCCR_INIT, FDCAN_CCCR_INIT))
    {
        return false;
    }
    instance->CCCR = FDCAN_CCCR_INIT | FDCAN_CCCR_CCE | FDCAN_CCCR_MON | FDCAN_CCCR_DAR;
    instance->NBTP = nbtp_value(&profile->nominal_timing);
    instance->DBTP = 0U;
    /* TIM2 is the one-microsecond hardware timestamp sampled in the RX IRQ. */
    instance->TSCC = 0U;
    instance->TSCV = 0U;
    instance->TOCC = 0U;
    /* LSS/LSE=0 plus ANFS/ANFE=FIFO0 accepts every standard/extended frame. */
    instance->RXGFC = 0U;
    instance->XIDAM = FDCAN_XIDAM_EIDM;
    instance->IR = CANVIEW_STM_FDCAN_INTERRUPT_MASK;
    instance->IE = CANVIEW_STM_FDCAN_ENABLE_INTERRUPTS;
    instance->ILS = 0U;
    instance->ILE = FDCAN_ILE_EINT0;
    instance->CCCR &= ~FDCAN_CCCR_INIT;
    return wait_register(&instance->CCCR, FDCAN_CCCR_INIT, 0U);
}

static void configure_interrupt(size_t channel_index)
{
    static const IRQn_Type interrupt_numbers[CANVIEW_STM_FDCAN_CHANNEL_COUNT] = {
        FDCAN1_IT0_IRQn, FDCAN2_IT0_IRQn, FDCAN3_IT0_IRQn};
    NVIC_ClearPendingIRQ(interrupt_numbers[channel_index]);
    NVIC_SetPriority(interrupt_numbers[channel_index], UINT32_C(5));
    NVIC_EnableIRQ(interrupt_numbers[channel_index]);
}

static void disable_interrupt(size_t channel_index)
{
    static const IRQn_Type interrupt_numbers[CANVIEW_STM_FDCAN_CHANNEL_COUNT] = {
        FDCAN1_IT0_IRQn, FDCAN2_IT0_IRQn, FDCAN3_IT0_IRQn};
    NVIC_DisableIRQ(interrupt_numbers[channel_index]);
    NVIC_ClearPendingIRQ(interrupt_numbers[channel_index]);
}

canview_status_t canview_stm_fdcan_platform_init(
    canview_stm_fdcan_platform_t *platform,
    const canview_stm_fdcan_platform_config_t *config)
{
    if (platform == NULL || config == NULL || config->frame_sink == NULL ||
        config->drop_sink == NULL || config->status_sink == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->initialized || active_platform != NULL)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        const canview_status_t profile_status =
            canview_stm_fdcan_channel_profile_validate(index, &config->profiles[index]);
        if (profile_status != CANVIEW_OK)
        {
            return profile_status;
        }
    }
    memset(platform, 0, sizeof(*platform));
    platform->config = *config;
    reset_runtime_state(platform);
    platform->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_platform_start(canview_stm_fdcan_platform_t *platform)
{
    if (platform == NULL || !platform->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->servicing)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (active_platform != NULL)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    bool any_enabled = false;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (platform->started[index])
        {
            return CANVIEW_RESOURCE_BUSY;
        }
        any_enabled = any_enabled || platform->config.profiles[index].enabled;
    }
    reset_runtime_state(platform);
    if (!any_enabled || (RCC->CCIPR & RCC_CCIPR_FDCANSEL) != RCC_CCIPR_FDCANSEL_0 ||
        !timestamp_clock_ready())
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN;
    (void)RCC->AHB2ENR;
    (void)RCC->APB1ENR1;

    const canview_status_t safe_status = set_capture_outputs(platform->config.profiles, false);
    if (safe_status != CANVIEW_OK)
    {
        return safe_status;
    }

    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (!platform->config.profiles[index].enabled)
        {
            continue;
        }
        configure_rx_pin(&rx_pins[index]);
        if (!configure_instance(instances[index], &platform->config.profiles[index]))
        {
            disable_instance(instances[index]);
            for (size_t rollback = 0U; rollback < index; ++rollback)
            {
                if (platform->started[rollback])
                {
                    disable_interrupt(rollback);
                    disable_instance(instances[rollback]);
                    platform->started[rollback] = false;
                }
            }
            const canview_status_t rollback_status =
                set_capture_outputs(platform->config.profiles, false);
            return rollback_status == CANVIEW_OK ? CANVIEW_TIMEOUT : rollback_status;
        }
        platform->started[index] = true;
    }
    const canview_status_t receive_status =
        set_capture_outputs(platform->config.profiles, true);
    if (receive_status != CANVIEW_OK)
    {
        for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
        {
            if (platform->started[index])
            {
                disable_instance(instances[index]);
                platform->started[index] = false;
            }
        }
        const canview_status_t rollback_status =
            set_capture_outputs(platform->config.profiles, false);
        return rollback_status == CANVIEW_OK ? receive_status : rollback_status;
    }
    active_platform = platform;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (platform->started[index])
        {
            configure_interrupt(index);
        }
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_platform_stop(canview_stm_fdcan_platform_t *platform)
{
    if (platform == NULL || !platform->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->servicing)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (active_platform != NULL && active_platform != platform)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    /* Detach the singleton before touching pins so a pending IRQ cannot use a
     * context that is being stopped. */
    if (active_platform == platform)
    {
        active_platform = NULL;
    }
    canview_status_t result = CANVIEW_OK;
    const canview_status_t safe_status = set_capture_outputs(platform->config.profiles, false);
    if (safe_status != CANVIEW_OK)
    {
        result = safe_status;
    }
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (!platform->started[index])
        {
            continue;
        }
        disable_interrupt(index);
        disable_instance(instances[index]);
        if (!wait_register(&instances[index]->CCCR, FDCAN_CCCR_INIT, FDCAN_CCCR_INIT))
        {
            result = CANVIEW_TIMEOUT;
        }
        platform->pending_interrupts[index] = 0U;
        platform->started[index] = false;
    }
    reset_runtime_state(platform);
    return result;
}

static void drain_fifo(canview_stm_fdcan_platform_t *platform, size_t channel_index,
                       FDCAN_GlobalTypeDef *instance)
{
    const uintptr_t ram_base = (uintptr_t)SRAMCAN_BASE +
                               (uintptr_t)channel_index *
                                   (uintptr_t)CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES +
                               (uintptr_t)CANVIEW_STM_FDCAN_RX_FIFO0_OFFSET_BYTES;
    for (uint32_t drained = 0U; drained < CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_COUNT; ++drained)
    {
        const uint32_t fifo_status = instance->RXF0S;
        const uint32_t fill_level = fifo_status & FDCAN_RXF0S_F0FL;
        if ((fifo_status & FDCAN_RXF0S_RF0L) != 0U)
        {
            platform->fifo_loss_unknown[channel_index] = true;
            platform->pending_interrupts[channel_index] |= FDCAN_IR_RF0L;
        }
        if (fill_level == 0U)
        {
            break;
        }
        if (fill_level > CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_COUNT)
        {
            platform->fifo_loss_unknown[channel_index] = true;
            platform->pending_interrupts[channel_index] |= FDCAN_IR_RF0L;
            break;
        }
        const uint32_t get_index =
            (fifo_status & FDCAN_RXF0S_F0GI) >> FDCAN_RXF0S_F0GI_Pos;
        if (get_index >= CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_COUNT)
        {
            platform->fifo_loss_unknown[channel_index] = true;
            platform->pending_interrupts[channel_index] |= FDCAN_IR_RF0L;
            break;
        }
        const uintptr_t element_address =
            ram_base + (uintptr_t)get_index * (uintptr_t)CANVIEW_STM_FDCAN_RX_FIFO0_ELEMENT_BYTES;
        volatile const uint32_t *const element = (volatile const uint32_t *)element_address;
        const uint32_t element_words[4] = {element[0], element[1], element[2], element[3]};
        const uint32_t timestamp_us = canview_stm_now_us(NULL);
        const uint8_t write_index = platform->raw_write_index[channel_index];
        const uint8_t read_index = platform->raw_read_index[channel_index];
        if ((uint8_t)(write_index - read_index) >=
            CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY)
        {
            if (platform->raw_drops[channel_index] != UINT32_MAX)
            {
                ++platform->raw_drops[channel_index];
            }
            platform->raw_ring_overflow[channel_index] = true;
            platform->pending_interrupts[channel_index] |= FDCAN_IR_RF0L;
        }
        else
        {
            canview_stm_fdcan_raw_element_t *const raw =
                &platform->raw_ring[channel_index]
                                     [write_index % CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY];
            raw->words[0] = element_words[0];
            raw->words[1] = element_words[1];
            raw->words[2] = element_words[2];
            raw->words[3] = element_words[3];
            raw->source_timestamp_us = timestamp_us;
            __DMB();
            platform->raw_write_index[channel_index] =
                (uint8_t)(write_index + 1U);
        }
        instance->RXF0A = get_index;
    }
}

static bool peek_raw_element(const canview_stm_fdcan_platform_t *platform, size_t channel_index,
                             canview_stm_fdcan_raw_element_t *element)
{
    const uint8_t read_index = platform->raw_read_index[channel_index];
    const uint8_t write_index = platform->raw_write_index[channel_index];
    if (read_index == write_index)
    {
        return false;
    }
    __DMB();
    *element = platform->raw_ring[channel_index]
                              [read_index % CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY];
    return true;
}

static void release_raw_element(canview_stm_fdcan_platform_t *platform, size_t channel_index)
{
    const uint8_t read_index = platform->raw_read_index[channel_index];
    platform->raw_read_index[channel_index] = (uint8_t)(read_index + 1U);
}

static canview_status_t service_raw_elements(canview_stm_fdcan_platform_t *platform,
                                             size_t channel_index)
{
    canview_status_t result = CANVIEW_OK;
    for (size_t count = 0U; count < CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY; ++count)
    {
        canview_stm_fdcan_raw_element_t raw = {0};
        if (!peek_raw_element(platform, channel_index, &raw))
        {
            break;
        }
        canview_stm_fdcan_rx_frame_t frame = {0};
        const canview_status_t decode_status = canview_stm_fdcan_decode_element(
            raw.words, raw.source_timestamp_us, &frame);
        if (decode_status != CANVIEW_OK)
        {
            release_raw_element(platform, channel_index);
            if (result == CANVIEW_OK)
            {
                result = decode_status;
            }
            continue;
        }
        const canview_status_t sink_status =
            platform->config.frame_sink(platform->config.sink_context, channel_index, &frame);
        if (sink_status != CANVIEW_OK)
        {
            if (platform->sink_failures[channel_index] != UINT32_MAX)
            {
                ++platform->sink_failures[channel_index];
            }
            if (result == CANVIEW_OK)
            {
                result = sink_status;
            }
            return result;
        }
        release_raw_element(platform, channel_index);
    }
    return result;
}

static canview_status_t service_raw_drops(canview_stm_fdcan_platform_t *platform,
                                          size_t channel_index)
{
    const uint32_t mask = canview_stm_critical_enter(NULL);
    const uint32_t current = platform->raw_drops[channel_index];
    const uint32_t previous = platform->reported_raw_drops[channel_index];
    const uint32_t delta = current >= previous ? current - previous : current;
    canview_stm_critical_leave(NULL, mask);
    if (delta == 0U)
    {
        return CANVIEW_OK;
    }
    const canview_status_t sink_status =
        platform->config.drop_sink(platform->config.sink_context, channel_index, delta);
    if (sink_status != CANVIEW_OK)
    {
        return sink_status;
    }
    const uint32_t update_mask = canview_stm_critical_enter(NULL);
    platform->reported_raw_drops[channel_index] = current;
    canview_stm_critical_leave(NULL, update_mask);
    return CANVIEW_OK;
}

static void handle_interrupt(size_t channel_index)
{
    canview_stm_fdcan_platform_t *const platform = active_platform;
    if (platform == NULL || !platform->initialized || !platform->started[channel_index])
    {
        return;
    }
    FDCAN_GlobalTypeDef *const instance = instances[channel_index];
    const uint32_t interrupt_flags = instance->IR & CANVIEW_STM_FDCAN_INTERRUPT_MASK;
    const uint32_t receive_flags = interrupt_flags & CANVIEW_STM_FDCAN_RX_INTERRUPTS;
    const uint32_t non_receive_flags = interrupt_flags & ~CANVIEW_STM_FDCAN_RX_INTERRUPTS;
    if ((interrupt_flags & FDCAN_IR_RF0L) != 0U)
    {
        platform->fifo_loss_unknown[channel_index] = true;
    }
    if ((interrupt_flags & FDCAN_IR_MRAF) != 0U)
    {
        platform->message_ram_fault[channel_index] = true;
    }
    /* Acknowledge the snapshot before reading message RAM.  RX flags raised
     * while drain_fifo() runs are deliberately not W1C-cleared by this ISR;
     * they remain asserted for the next interrupt and cannot be lost. */
    if (non_receive_flags != 0U)
    {
        instance->IR = non_receive_flags;
    }
    if (receive_flags != 0U)
    {
        instance->IR = receive_flags;
        drain_fifo(platform, channel_index, instance);
    }
    if (interrupt_flags != 0U)
    {
        platform->pending_interrupts[channel_index] |= interrupt_flags;
    }
}

void FDCAN1_IT0_IRQHandler(void)
{
    handle_interrupt(0U);
}

void FDCAN2_IT0_IRQHandler(void)
{
    handle_interrupt(1U);
}

void FDCAN3_IT0_IRQHandler(void)
{
    handle_interrupt(2U);
}

static canview_stm_fdcan_bus_state_t state_from_registers(uint32_t protocol_status)
{
    if ((protocol_status & FDCAN_PSR_BO) != 0U)
    {
        return CANVIEW_STM_FDCAN_BUS_OFF;
    }
    if ((protocol_status & FDCAN_PSR_EP) != 0U)
    {
        return CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE;
    }
    return CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE;
}

canview_status_t canview_stm_fdcan_platform_service(canview_stm_fdcan_platform_t *platform,
                                                    uint32_t source_timestamp_us)
{
    if (platform == NULL || !platform->initialized || platform->config.frame_sink == NULL ||
        platform->config.drop_sink == NULL || platform->config.status_sink == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->servicing)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    platform->servicing = true;
    canview_status_t result = CANVIEW_OK;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (!platform->started[index])
        {
            continue;
        }
        const canview_status_t drop_status = service_raw_drops(platform, index);
        if (drop_status != CANVIEW_OK && result == CANVIEW_OK)
        {
            result = drop_status;
        }
        const canview_status_t frame_status = service_raw_elements(platform, index);
        if (frame_status != CANVIEW_OK && result == CANVIEW_OK)
        {
            result = frame_status;
        }
        const uint32_t mask = canview_stm_critical_enter(NULL);
        const uint32_t pending = platform->pending_interrupts[index];
        const bool fifo_loss = platform->fifo_loss_unknown[index];
        const bool message_ram_fault = platform->message_ram_fault[index];
        const bool raw_ring_overflow = platform->raw_ring_overflow[index];
        const uint32_t event_fault_flags =
            (fifo_loss ? CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS : 0U) |
            (message_ram_fault ? CANVIEW_STM_FDCAN_PLATFORM_ERROR_MESSAGE_RAM : 0U) |
            (raw_ring_overflow ? CANVIEW_STM_FDCAN_PLATFORM_ERROR_RAW_RING_OVERFLOW : 0U);
        platform->session_fault_flags[index] |= event_fault_flags;
        const uint32_t session_fault_flags = platform->session_fault_flags[index];
        platform->pending_interrupts[index] = 0U;
        platform->fifo_loss_unknown[index] = false;
        platform->message_ram_fault[index] = false;
        platform->raw_ring_overflow[index] = false;
        canview_stm_critical_leave(NULL, mask);
        if (pending == 0U)
        {
            continue;
        }
        const FDCAN_GlobalTypeDef *const instance = instances[index];
        const uint32_t protocol_status = instance->PSR;
        const uint32_t error_count = instance->ECR;
        const canview_stm_fdcan_bus_state_t state =
            session_fault_flags != 0U ? CANVIEW_STM_FDCAN_BUS_FAULT
                                      : state_from_registers(protocol_status);
        if (state == CANVIEW_STM_FDCAN_BUS_OFF &&
            platform->previous_state[index] != CANVIEW_STM_FDCAN_BUS_OFF)
        {
            if (platform->bus_off_count[index] != UINT32_MAX)
            {
                ++platform->bus_off_count[index];
            }
        }
        platform->previous_state[index] = state;
        const uint16_t rx_error_count = (uint16_t)((error_count >> 8U) & UINT32_C(0xff));
        const uint16_t tx_error_count = (uint16_t)(error_count & UINT32_C(0xff));
        const uint32_t last_error = ((protocol_status & FDCAN_PSR_LEC) >> FDCAN_PSR_LEC_Pos) |
                                    (pending & CANVIEW_STM_FDCAN_STATUS_INTERRUPTS) |
                                    session_fault_flags;
        platform->config.status_sink(platform->config.sink_context, index, state, rx_error_count,
                                     tx_error_count, platform->bus_off_count[index], last_error,
                                     source_timestamp_us);
    }
    platform->servicing = false;
    return result;
}
