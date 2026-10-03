/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */


#include <stdio.h>
#include <string.h>
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"
#include "bsp.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "lightdrv.h"
#include "nrf_delay.h"

#define UNALLOCATED                                0xFFFFFFFFUL

static uint8_t s_lightdrv_init = 0;

static const nrf_drv_timer_t p_timer = NRF_DRV_TIMER_INSTANCE(1);
static const nrf_drv_timer_t n_timer = NRF_DRV_TIMER_INSTANCE(2);

static nrf_ppi_channel_group_t  s_ppi_group = (nrf_ppi_channel_group_t)UNALLOCATED;
static nrf_ppi_channel_t p_ppi_channel1 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t p_ppi_channel2 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t p_ppi_channel3 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t n_ppi_channel1 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t n_ppi_channel2 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t n_ppi_channel3 = (nrf_ppi_channel_t)UNALLOCATED;


static uint32_t  p_timer_pulth_w     = 1000;//us
static uint32_t  n_timer_pulth_w     = 1000;//us

#define LIGHTDRV_2MOS_P_INPUT         BSP_LIGHTDRV_2MOS_P_INPUT
#define LIGHTDRV_2MOS_N_INPUT         BSP_LIGHTDRV_2MOS_N_INPUT
#define LIGHTDRV_2MOS_P_OUTPUT        BSP_LIGHTDRV_2MOS_P_OUTPUT
#define LIGHTDRV_2MOS_N_OUTPUT        BSP_LIGHTDRV_2MOS_N_OUTPUT

#define LIGHTDRV_TEST_PIN1            19

// Timer even handler. Not used since timer is used only for PPI.
void timer_event_handler(nrf_timer_event_t event_type, void * p_context)
{
}


static void gpiote_input_init(nrf_drv_gpiote_pin_t pin, uint8_t htl_cfg)
{
    ret_code_t err_code;
    if(htl_cfg)
    {
        nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
        //in_config.pull = NRF_GPIO_PIN_PULLUP;
        err_code = nrf_drv_gpiote_in_init(pin,&in_config,NULL);
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
        //in_config.pull = NRF_GPIO_PIN_PULLUP;
        err_code = nrf_drv_gpiote_in_init(pin,&in_config,NULL);
        APP_ERROR_CHECK(err_code);
    }
    nrf_drv_gpiote_in_event_enable(pin,true);
}

static void gpiote_input_uninit(nrf_drv_gpiote_pin_t pin)
{
    nrf_drv_gpiote_in_uninit(pin);
}

static void gpiote_output_init(nrf_drv_gpiote_pin_t pin)
{
    ret_code_t err_code;
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(1);
    err_code = nrf_drv_gpiote_out_init(pin,&out_config);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_task_enable(pin);
}

static void gpiote_output_uninit(nrf_drv_gpiote_pin_t pin)
{
    nrf_drv_gpiote_out_uninit(pin);
}


static void ptimer_init(void)
{
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    ret_code_t err_code = nrf_drv_timer_init(&p_timer, &timer_cfg, timer_event_handler);
    APP_ERROR_CHECK(err_code);
}

static void ptimer_compare_set(void)
{
    nrf_drv_timer_extended_compare(&p_timer, NRF_TIMER_CC_CHANNEL0, p_timer_pulth_w, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK|NRF_TIMER_SHORT_COMPARE0_STOP_MASK, false);
}

static void ntimer_init(void)
{
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    ret_code_t err_code = nrf_drv_timer_init(&n_timer, &timer_cfg, timer_event_handler);
    APP_ERROR_CHECK(err_code);

}

static void ntimer_compare_set(void)
{
    nrf_drv_timer_extended_compare(&n_timer, NRF_TIMER_CC_CHANNEL0, n_timer_pulth_w, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK|NRF_TIMER_SHORT_COMPARE0_STOP_MASK, false);
}


static void phase_enable()
{
    bool p_h = false;
    //nrf_drv_gpiote_out_task_force(LIGHTDRV_2MOS_P_OUTPUT, 0);
    //nrf_drv_gpiote_out_task_force(LIGHTDRV_2MOS_N_OUTPUT, 0);
    //nrf_gpio_pin_set(LIGHTDRV_TEST_PIN1);
    //p_h = nrf_drv_gpiote_in_is_set(LIGHTDRV_2MOS_P_INPUT);
    //while(p_h == nrf_drv_gpiote_in_is_set(LIGHTDRV_2MOS_P_INPUT))
    //{
    //    nrf_delay_ms(2);
    //}
    //nrf_delay_ms(2);
    //nrf_gpio_pin_clear(LIGHTDRV_TEST_PIN1);
    ptimer_compare_set();
    ntimer_compare_set();
    nrf_drv_ppi_channel_assign(p_ppi_channel1, 
                               nrf_drv_gpiote_in_event_addr_get(LIGHTDRV_2MOS_P_INPUT),
                               nrfx_gpiote_clr_task_addr_get(LIGHTDRV_2MOS_P_OUTPUT));
    nrf_drv_ppi_channel_assign(p_ppi_channel2, 
                               nrf_drv_gpiote_in_event_addr_get(LIGHTDRV_2MOS_P_INPUT),
                               nrf_drv_timer_task_address_get(&p_timer, NRF_TIMER_TASK_START));
    nrf_drv_ppi_channel_assign(p_ppi_channel3, 
                               nrf_drv_timer_event_address_get(&p_timer, NRF_TIMER_EVENT_COMPARE0),
                               nrfx_gpiote_set_task_addr_get(LIGHTDRV_2MOS_P_OUTPUT));
    nrf_drv_ppi_channel_assign(n_ppi_channel1, 
                               nrf_drv_gpiote_in_event_addr_get(LIGHTDRV_2MOS_N_INPUT),
                               nrfx_gpiote_clr_task_addr_get(LIGHTDRV_2MOS_N_OUTPUT));
    nrf_drv_ppi_channel_assign(n_ppi_channel2, 
                               nrf_drv_gpiote_in_event_addr_get(LIGHTDRV_2MOS_N_INPUT),
                               nrf_drv_timer_task_address_get(&n_timer, NRF_TIMER_TASK_START));
    nrf_drv_ppi_channel_assign(n_ppi_channel3, 
                               nrf_drv_timer_event_address_get(&n_timer, NRF_TIMER_EVENT_COMPARE0),
                               nrfx_gpiote_set_task_addr_get(LIGHTDRV_2MOS_N_OUTPUT));
    nrf_drv_timer_enable(&p_timer);
    nrf_drv_timer_enable(&n_timer);
    nrf_drv_ppi_group_enable(s_ppi_group);
}

static void phase_disable()
{
    nrf_drv_ppi_group_disable(s_ppi_group);
    nrf_drv_timer_pause(&p_timer);
    nrf_drv_timer_pause(&n_timer);
    nrf_drv_timer_clear(&p_timer);
    nrf_drv_timer_clear(&n_timer);
    nrf_drv_gpiote_out_task_force(LIGHTDRV_2MOS_P_OUTPUT, 1);
    nrf_drv_gpiote_out_task_force(LIGHTDRV_2MOS_N_OUTPUT, 1);
}

int lightdrv_init(void)
{
    ret_code_t err_code;
    if(s_lightdrv_init == 1)
    {
        return 0;
    }
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
    gpiote_input_init(LIGHTDRV_2MOS_P_INPUT, 1);
    gpiote_input_init(LIGHTDRV_2MOS_N_INPUT, 0);
    gpiote_output_init(LIGHTDRV_2MOS_P_OUTPUT);
    gpiote_output_init(LIGHTDRV_2MOS_N_OUTPUT);
    //nrf_gpio_cfg_output(LIGHTDRV_TEST_PIN1);
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_group_alloc(&s_ppi_group);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&p_ppi_channel1);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&p_ppi_channel2);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&p_ppi_channel3);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&n_ppi_channel1);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&n_ppi_channel2);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&n_ppi_channel3);
    APP_ERROR_CHECK(err_code);
    nrf_drv_ppi_group_clear(s_ppi_group);
    nrf_drv_ppi_channels_include_in_group(
        nrf_drv_ppi_channel_to_mask(p_ppi_channel1) |
        nrf_drv_ppi_channel_to_mask(p_ppi_channel2) |
        nrf_drv_ppi_channel_to_mask(p_ppi_channel3) |
        nrf_drv_ppi_channel_to_mask(n_ppi_channel1) |
        nrf_drv_ppi_channel_to_mask(n_ppi_channel2) |
        nrf_drv_ppi_channel_to_mask(n_ppi_channel3),
        s_ppi_group);
    ptimer_init();
    ntimer_init();
    s_lightdrv_init = 1;
    //phase_enable();
    return 0;
}

int lightdrv_lightness_set(int lightness)
{
    uint32_t temp_val = 0;
    if(s_lightdrv_init != 1)
    {
        return 1;
    }
    if(lightness > 1000 || lightness < 0)
    {
        return 1;
    }
    temp_val = 50+(lightness*99)/10;
    p_timer_pulth_w = temp_val;
    n_timer_pulth_w = temp_val;
    //nrf_timer_cc_write(p_timer.p_reg, NRF_TIMER_CC_CHANNEL0, p_timer_pulth_w);
    //nrf_timer_cc_write(n_timer.p_reg, NRF_TIMER_CC_CHANNEL0, n_timer_pulth_w);
    return 0;
}

int lightdrv_start(void)
{
    if(s_lightdrv_init != 1)
    {
        return 1;
    }
    phase_enable();
    return 0;
}

int lightdrv_stop(void)
{
    if(s_lightdrv_init != 1)
    {
        return 1;
    }
    phase_disable();
    return 0;
}



/** @} */
