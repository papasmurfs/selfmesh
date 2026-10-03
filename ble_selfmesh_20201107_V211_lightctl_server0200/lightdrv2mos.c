/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */


#include <stdio.h>
#include <string.h>
#include "nrf_drv_pwm.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"
#include "bsp.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "lightdrv.h"
#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define LIGHTDRV_2MOS_P_INPUT         BSP_LIGHTDRV_2MOS_P_INPUT
#define LIGHTDRV_2MOS_N_INPUT         BSP_LIGHTDRV_2MOS_N_INPUT
#define LIGHTDRV_2MOS_P_OUTPUT        BSP_LIGHTDRV_2MOS_P_OUTPUT
#define LIGHTDRV_2MOS_N_OUTPUT        BSP_LIGHTDRV_2MOS_N_OUTPUT
#define LIGHTDRV_OUT_CTL              BSP_LIGHTDRV_OUT_CTL

#define UNALLOCATED                                0xFFFFFFFFUL

static volatile uint8_t s_is_start = 0;

static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);
static nrf_drv_pwm_t m_pwm1 = NRF_DRV_PWM_INSTANCE(1);

static nrf_ppi_channel_group_t  s_ppi_group = (nrf_ppi_channel_group_t)UNALLOCATED;
static nrf_ppi_channel_t p_nrf_ppi_channel1 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t p_nrf_ppi_channel2 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t n_nrf_ppi_channel1 = (nrf_ppi_channel_t)UNALLOCATED;
static nrf_ppi_channel_t n_nrf_ppi_channel2 = (nrf_ppi_channel_t)UNALLOCATED;


#define USED_PWM(idx) (1UL << idx)
static uint8_t m_used = 0;



enum { // [local constants]
    TOP        = 15000,
    STEP_COUNT = 1
};
// This array cannot be allocated on stack (hence "static") and it must
// be in RAM.
static nrf_pwm_values_common_t p_phase_values[STEP_COUNT];
static nrf_pwm_values_common_t n_phase_values[STEP_COUNT];

        
static nrf_pwm_sequence_t const seq0 =
{
    .values.p_common = p_phase_values,
    .length          = NRF_PWM_VALUES_LENGTH(p_phase_values),
    .repeats         = 1,
    .end_delay       = 0
};

static nrf_pwm_sequence_t const seq1 =
{
    .values.p_common = n_phase_values,
    .length          = NRF_PWM_VALUES_LENGTH(n_phase_values),
    .repeats         = 1,
    .end_delay       = 0
};

void gpiote_input_init(nrfx_gpiote_pin_t pin, uint8_t htl_cfg)
{
    ret_code_t err_code;
    if(htl_cfg)
    {
        nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
        in_config.pull = NRF_GPIO_PIN_PULLUP;
        err_code = nrfx_gpiote_in_init(pin,&in_config,NULL);
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
        in_config.pull = NRF_GPIO_PIN_PULLUP;
        err_code = nrfx_gpiote_in_init(pin,&in_config,NULL);
        APP_ERROR_CHECK(err_code);
    }
    nrf_drv_gpiote_in_event_enable(pin,true);
}

void gpiote_output_init(nrfx_gpiote_pin_t pin)
{
    ret_code_t err_code;
    nrf_drv_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);
    err_code = nrfx_gpiote_out_init(pin,&out_config);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_task_enable(pin);
}


static void phase_config(nrfx_gpiote_pin_t out_ctl_pin, nrfx_gpiote_pin_t p_in_pin, nrfx_gpiote_pin_t n_in_pin, nrfx_gpiote_pin_t p_out_pin, nrfx_gpiote_pin_t n_out_pin)
{
    ret_code_t err_code;  
    uint32_t p_nrfx_gpiote_in_event_addr;
    uint32_t p_nrfx_gpiote_out_task_addr1;
    uint32_t p_nrfx_gpiote_out_task_addr2;
    uint32_t n_nrfx_gpiote_in_event_addr;
    uint32_t n_nrfx_gpiote_out_task_addr1;
    uint32_t n_nrfx_gpiote_out_task_addr2;
    NRF_LOG_INFO("p_phase_config");

    gpiote_input_init(p_in_pin, 1);
    gpiote_input_init(n_in_pin, 0);
    gpiote_output_init(p_out_pin);
    gpiote_output_init(n_out_pin);
    gpiote_output_init(out_ctl_pin);
    nrf_delay_ms(10);
    nrfx_gpiote_out_task_force(LIGHTDRV_OUT_CTL, 1);//do not out here
    err_code = nrf_drv_ppi_init();
    
    
    NRF_LOG_INFO("nrf_drv_ppi_init is %d\n",err_code);
    p_nrfx_gpiote_in_event_addr = nrfx_gpiote_in_event_addr_get(p_in_pin);
    n_nrfx_gpiote_in_event_addr = nrfx_gpiote_in_event_addr_get(n_in_pin);
    err_code = nrf_drv_ppi_group_alloc(&s_ppi_group);
    APP_ERROR_CHECK(err_code);
    err_code = nrfx_ppi_channel_alloc(&p_nrf_ppi_channel1);
    APP_ERROR_CHECK(err_code);
    err_code = nrfx_ppi_channel_alloc(&p_nrf_ppi_channel2);
    APP_ERROR_CHECK(err_code);
    err_code = nrfx_ppi_channel_alloc(&n_nrf_ppi_channel1);
    APP_ERROR_CHECK(err_code);
    err_code = nrfx_ppi_channel_alloc(&n_nrf_ppi_channel2);
    APP_ERROR_CHECK(err_code);
    nrf_drv_ppi_group_clear(s_ppi_group);
    nrf_drv_ppi_channels_include_in_group(
        nrf_drv_ppi_channel_to_mask(p_nrf_ppi_channel1) |
        nrf_drv_ppi_channel_to_mask(p_nrf_ppi_channel2) |
        nrf_drv_ppi_channel_to_mask(n_nrf_ppi_channel1) |
        nrf_drv_ppi_channel_to_mask(n_nrf_ppi_channel2),
        s_ppi_group);

    nrf_drv_pwm_config_t const config0 =
    {
        .output_pins =
        {
            p_out_pin,// | NRF_DRV_PWM_PIN_INVERTED, // channel 0
            NRF_DRV_PWM_PIN_NOT_USED, // channel 1
            NRF_DRV_PWM_PIN_NOT_USED, // channel 2
            NRF_DRV_PWM_PIN_NOT_USED  // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = TOP,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_TRIGGERED
    };
    APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, NULL));
    m_used |= USED_PWM(0);

    nrf_drv_pwm_config_t const config1 =
    {
        .output_pins =
        {
            n_out_pin,// | NRF_DRV_PWM_PIN_INVERTED, // channel 0
            NRF_DRV_PWM_PIN_NOT_USED, // channel 1
            NRF_DRV_PWM_PIN_NOT_USED, // channel 2
            NRF_DRV_PWM_PIN_NOT_USED  // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = TOP,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_TRIGGERED
    };
    APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm1, &config1, NULL));
    m_used |= USED_PWM(1);

    p_nrfx_gpiote_out_task_addr1 = nrf_pwm_task_address_get(m_pwm0.p_registers, NRF_PWM_TASK_SEQSTART0);
    p_nrfx_gpiote_out_task_addr2 = nrf_pwm_task_address_get(m_pwm1.p_registers, NRF_PWM_TASK_STOP);
    nrfx_ppi_channel_assign(p_nrf_ppi_channel1,p_nrfx_gpiote_in_event_addr,p_nrfx_gpiote_out_task_addr1);
    nrfx_ppi_channel_assign(p_nrf_ppi_channel2,p_nrfx_gpiote_in_event_addr,p_nrfx_gpiote_out_task_addr2);
        
    n_nrfx_gpiote_out_task_addr1 = nrf_pwm_task_address_get(m_pwm0.p_registers, NRF_PWM_TASK_STOP);
    n_nrfx_gpiote_out_task_addr2 = nrf_pwm_task_address_get(m_pwm1.p_registers, NRF_PWM_TASK_SEQSTART0);
    nrfx_ppi_channel_assign(n_nrf_ppi_channel1,n_nrfx_gpiote_in_event_addr,n_nrfx_gpiote_out_task_addr1);
    nrfx_ppi_channel_assign(n_nrf_ppi_channel2,n_nrfx_gpiote_in_event_addr,n_nrfx_gpiote_out_task_addr2);

    (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1,
                                          NRF_DRV_PWM_FLAG_STOP);

    (void)nrf_drv_pwm_simple_playback(&m_pwm1, &seq1, 1,
                                          NRF_DRV_PWM_FLAG_STOP);
        
}

static void phase_enable(void)
{
    bool p_h = false;
    if(s_is_start == 0)
    {
        nrfx_gpiote_out_task_force(LIGHTDRV_2MOS_P_OUTPUT, 0);
        nrfx_gpiote_out_task_force(LIGHTDRV_2MOS_N_OUTPUT, 0);
        nrf_delay_ms(15);
        p_h = nrf_drv_gpiote_in_is_set(LIGHTDRV_2MOS_P_INPUT);
        while(p_h == nrf_drv_gpiote_in_is_set(LIGHTDRV_2MOS_P_INPUT))
        {
            nrf_delay_ms(1);
        }
        nrf_delay_ms(1);
        nrf_drv_ppi_group_enable(s_ppi_group);
        nrfx_gpiote_out_task_force(LIGHTDRV_OUT_CTL, 0);//enable out here

        s_is_start = 1;
    }
}

static void phase_disable(void)
{
    if(s_is_start == 1)
    {
        nrfx_gpiote_out_task_force(LIGHTDRV_OUT_CTL, 1);//disable out here
        nrfx_ppi_channel_disable(p_nrf_ppi_channel1); //disable PWM start message first
        nrfx_ppi_channel_disable(n_nrf_ppi_channel2);
        nrf_delay_ms(22);
        nrfx_ppi_channel_disable(p_nrf_ppi_channel2); //disable PWM stop message next
        nrfx_ppi_channel_disable(n_nrf_ppi_channel1);
        nrf_delay_ms(22);

        nrfx_gpiote_out_task_force(LIGHTDRV_2MOS_P_OUTPUT, 1);
        nrfx_gpiote_out_task_force(LIGHTDRV_2MOS_N_OUTPUT, 1);
        s_is_start = 0;
    }
}

static void p_phase_puls_width_set(nrf_pwm_values_common_t p_pwm_val, nrf_pwm_values_common_t n_pwm_val)
{
    p_phase_values[0] = p_pwm_val;
    n_phase_values[0] = n_pwm_val;
}


int lightdrv_init(void)
{
    ret_code_t err_code;
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
    phase_config(LIGHTDRV_OUT_CTL, LIGHTDRV_2MOS_P_INPUT, LIGHTDRV_2MOS_N_INPUT, LIGHTDRV_2MOS_P_OUTPUT, LIGHTDRV_2MOS_N_OUTPUT);


    return 0;
}
#if 1
 struct _light_curve {
    int  fdex;
    int  value;
}light_curve[17]=
{
    {10,  60},
    {50,  500},
    {100, 1000},
    {200, 1500},
    {300, 2000},
    {350, 2500},
    {400, 3000},
    {450, 3500},
    {500, 4000},
    {550, 4500},
    {600, 5000},
    {650, 5500},
    {700, 6000},
    {750, 6500},
    {800, 7000},
    {900, 7500},
    {1000,8000}
};


void light_check_value(int lightness,uint16_t *px, uint16_t *nx)
{

    int segment = 0;
    uint16_t g_light_vlaue = 0;

    if(lightness < light_curve[0].fdex)//100
    {
        g_light_vlaue = 50;
        *px = g_light_vlaue;
        *nx = g_light_vlaue + 2000;
        return ;
    }
    else if(lightness < light_curve[1].fdex)
    {
        segment = 0;
    }
    else if(lightness < light_curve[2].fdex)
    {
        segment = 1;
    }
    else if(lightness < light_curve[3].fdex)
    {
        segment = 2;
    }
    else if(lightness < light_curve[4].fdex)
    {
        segment = 3;
    }
    else if(lightness < light_curve[5].fdex)
    {
        segment = 4;
    }
    else if(lightness < light_curve[6].fdex)
    {
        segment = 5;
    }
    else if(lightness < light_curve[7].fdex)
    {
        segment = 6;
    }
    else if(lightness < light_curve[8].fdex)
    {
        segment = 7;
    }
    else if(lightness < light_curve[9].fdex)
    {
        segment = 8;
    }
    else if(lightness < light_curve[10].fdex)
    {
        segment = 9;
    }
    else if(lightness < light_curve[11].fdex)
    {
        segment = 10;
    }
    else if(lightness < light_curve[12].fdex)
    {
        segment = 11;
    }
    else if(lightness < light_curve[13].fdex)
    {
        segment = 12;
    }
    else if(lightness < light_curve[14].fdex)
    {
        segment = 13;
    }
    else if(lightness < light_curve[15].fdex)
    {
        segment = 14;
    }
    else if(lightness < light_curve[16].fdex)
    {
        segment = 15;
    }
    else 
    {
        g_light_vlaue = 8000;
        *px = g_light_vlaue;
        *nx = g_light_vlaue + 2000;
        return ;
    }

    g_light_vlaue = light_curve[segment].value + (lightness - light_curve[segment].fdex) * (light_curve[segment+1].value - light_curve[segment].value)/(light_curve[segment+1].fdex - light_curve[segment].fdex) ;

    *px = g_light_vlaue;
    *nx = g_light_vlaue + 2000;

    return ;
}


int lightdrv_lightness_set(int lightness)
{
    nrf_pwm_values_common_t p_temp_val = 0;
    nrf_pwm_values_common_t n_temp_val = 0;

    NRF_LOG_INFO("lightdrv_lightness_set: %d\n", lightness); 

    if(lightness > 1000 || lightness < 0)
    {
        return 1;
    }
    if(lightness == 0)
    {
        phase_disable();
        return 0;
    }
    light_check_value(lightness,&p_temp_val,&n_temp_val);
    //NRF_LOG_INFO("lightdrv_lightness_set lightness %d set pulse to p:%d & n:%d.",lightness, p_temp_val, n_temp_val);
    p_phase_puls_width_set(p_temp_val, n_temp_val);
 
    phase_enable(); 
    return 0;
}

#endif

int lightdrv_start(void)
{
    return 0;
}

int lightdrv_stop(void)
{
    return 0;
}

/** @} */
