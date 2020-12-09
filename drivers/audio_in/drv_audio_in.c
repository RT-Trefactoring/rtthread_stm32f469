#include <board.h>
#include <pdm2pcm_glo.h>
#include <string.h>
#include <rthw.h>
#include <rtthread.h>

/* PDM buffer input size */
#define INTERNAL_BUFF_SIZE                  (128*I2S_AUDIOFREQ_16K/16000*2)//256
/* PCM buffer output size */
#define PCM_OUT_SIZE                        (I2S_AUDIOFREQ_16K/1000*2)//32
#define CHANNEL_DEMUX_MASK                  ((uint8_t)0x55)

ALIGN(4)
static uint16_t mic_pdm_buffer[INTERNAL_BUFF_SIZE];
static rt_mailbox_t mic_mb;
static rt_mp_t mic_mp;
/* PDM filters params */
PDM_Filter_Handler_t  PDM_FilterHandler[2];
PDM_Filter_Config_t   PDM_FilterConfig[2];

uint8_t Channel_Demux[128] =
{
    0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
    0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
    0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
    0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
    0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
    0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
    0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
    0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
    0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
    0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
    0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
    0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
    0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
    0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f
};
static I2S_HandleTypeDef haudio_in_i2s;
static int i2s_init(I2S_HandleTypeDef *hi2s, SPI_TypeDef *Instace);
static int pass_tim_init(TIM_TypeDef *instance, uint32_t in_channel, uint32_t out_channel);
static int dma_config(I2S_HandleTypeDef *hi2s);
static int PDMDecoder_Init(uint32_t AudioFreq, uint32_t ChnlNbrIn, uint32_t ChnlNbrOut);
int mic_init(void)
{
    pass_tim_init(TIM4, TIM_CHANNEL_1, TIM_CHANNEL_2);
    i2s_init(&haudio_in_i2s, SPI3);
    PDMDecoder_Init(I2S_AUDIOFREQ_16K, 2, 2);
    return 0;
}
INIT_APP_EXPORT(mic_init);

void HAL_I2S_MspInit(I2S_HandleTypeDef* hi2s)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hi2s->Instance==SPI3)
    {
        /* USER CODE BEGIN SPI3_MspInit 0 */

        /* USER CODE END SPI3_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        /**I2S3 GPIO Configuration    
        PB3     ------> I2S3_CK
        PD6     ------> I2S3_SD 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_I2S3ext;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* USER CODE BEGIN SPI3_MspInit 1 */

        /* USER CODE END SPI3_MspInit 1 */
    }

}
static int i2s_init(I2S_HandleTypeDef *hi2s, SPI_TypeDef *Instace)
{
    hi2s->Instance = SPI3;

    hi2s->Init.AudioFreq = 4 * I2S_AUDIOFREQ_16K;//1.022MHz
    hi2s->Init.ClockSource = I2S_CLOCK_PLL;
    hi2s->Init.CPOL = I2S_CPOL_LOW;
    hi2s->Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s->Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    hi2s->Init.Mode = I2S_MODE_MASTER_RX;
    hi2s->Init.Standard = I2S_STANDARD_LSB;
    if (HAL_I2S_Init(hi2s) != HAL_OK)
    {
        return -1;
    }
    dma_config(hi2s);
    return 0;
}

static int pass_tim_init(TIM_TypeDef *instance, uint32_t in_channel, uint32_t out_channel)
{
    TIM_IC_InitTypeDef     s_ic_config;
    TIM_OC_InitTypeDef     s_oc_config;
    TIM_ClockConfigTypeDef s_clk_source_config;
    TIM_SlaveConfigTypeDef s_slave_config;
    GPIO_InitTypeDef       gpio_init_structure;
    static TIM_HandleTypeDef haudio_tim;

    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;
    gpio_init_structure.Alternate = GPIO_AF2_TIM4;

    /* Configure TIM input channel */
    gpio_init_structure.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* Configure TIM output channel */
    gpio_init_structure.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    haudio_tim.Instance = instance;

    haudio_tim.Init.Period        = 1;
    haudio_tim.Init.Prescaler     = 0;
    haudio_tim.Init.ClockDivision = 0;
    haudio_tim.Init.CounterMode   = TIM_COUNTERMODE_UP;

    HAL_TIM_IC_Init(&haudio_tim);

    s_ic_config.ICPolarity  = TIM_ICPOLARITY_FALLING;
    s_ic_config.ICSelection = TIM_ICSELECTION_DIRECTTI;
    s_ic_config.ICPrescaler = TIM_ICPSC_DIV1;
    s_ic_config.ICFilter    = 0;
    HAL_TIM_IC_ConfigChannel(&haudio_tim, &s_ic_config, in_channel);

    s_clk_source_config.ClockSource = TIM_CLOCKSOURCE_ETRMODE1;
    s_clk_source_config.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
    s_clk_source_config.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    s_clk_source_config.ClockFilter = 0;
    HAL_TIM_ConfigClockSource(&haudio_tim, &s_clk_source_config);

    s_slave_config.InputTrigger = TIM_TS_TI1FP1;
    s_slave_config.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
    s_slave_config.TriggerPolarity = TIM_TRIGGERPOLARITY_NONINVERTED;
    s_slave_config.TriggerPrescaler = TIM_CLOCKPRESCALER_DIV1;
    s_slave_config.TriggerFilter = 0;
    HAL_TIM_SlaveConfigSynchronization(&haudio_tim, &s_slave_config);

    s_oc_config.OCMode = TIM_OCMODE_PWM1;
    s_oc_config.OCIdleState = TIM_OCIDLESTATE_SET;
    s_oc_config.Pulse = 1;
    s_oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    s_oc_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    s_oc_config.OCFastMode = TIM_OCFAST_DISABLE;
    s_oc_config.OCNIdleState = TIM_OCNIDLESTATE_SET;

    HAL_TIM_PWM_ConfigChannel(&haudio_tim, &s_oc_config, out_channel);

    HAL_TIM_PWM_Start(&haudio_tim, out_channel);

    HAL_TIM_IC_Start(&haudio_tim, in_channel);

    return 0;
}

static int dma_config(I2S_HandleTypeDef *hi2s)
{
    static DMA_HandleTypeDef hdma_i2s_rx;
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* Configure the hdma_i2sRx handle parameters */
    hdma_i2s_rx.Init.Channel             = DMA_CHANNEL_0;
    hdma_i2s_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_i2s_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2s_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2s_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_i2s_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_i2s_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_i2s_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2s_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_i2s_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_i2s_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_i2s_rx.Init.PeriphBurst         = DMA_MBURST_SINGLE;

    hdma_i2s_rx.Instance = DMA1_Stream2;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hi2s, hdmarx, hdma_i2s_rx);

    /* Deinitialize the Stream for new transfer */
    HAL_DMA_DeInit(&hdma_i2s_rx);

    /* Configure the DMA Stream */
    HAL_DMA_Init(&hdma_i2s_rx);

    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    return 0;
}

static int PDMDecoder_Init(uint32_t AudioFreq, uint32_t ChnlNbrIn, uint32_t ChnlNbrOut)
{
    uint32_t index = 0;

    /* Enable CRC peripheral to unlock the PDM library */
    __HAL_RCC_CRC_CLK_ENABLE();

    for (index = 0; index < ChnlNbrIn; index++)
    {
        /* Init PDM filters */
        PDM_FilterHandler[index].bit_order  = PDM_FILTER_BIT_ORDER_LSB;
        PDM_FilterHandler[index].endianness = PDM_FILTER_ENDIANNESS_LE;
        PDM_FilterHandler[index].high_pass_tap = 2122358088;
        PDM_FilterHandler[index].out_ptr_channels = ChnlNbrOut;
        PDM_FilterHandler[index].in_ptr_channels  = ChnlNbrIn;
        PDM_Filter_Init((PDM_Filter_Handler_t *)(&PDM_FilterHandler[index]));

        /* PDM lib config phase */
        PDM_FilterConfig[index].output_samples_number = AudioFreq / 1000;
        PDM_FilterConfig[index].mic_gain = 24;
        PDM_FilterConfig[index].decimation_factor = PDM_FILTER_DEC_FACTOR_64;
        PDM_Filter_setConfig((PDM_Filter_Handler_t *)&PDM_FilterHandler[index], &PDM_FilterConfig[index]);
    }
    return 0;
}

void DMA1_Stream2_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(haudio_in_i2s.hdmarx);

    /* leave interrupt */
    rt_interrupt_leave();
}

static int pdm2pcm(uint16_t *pdmbuf, uint16_t *pcmbuf)
{
    static uint8_t app_pdm[INTERNAL_BUFF_SIZE * 2];
    uint8_t byte1 = 0, byte2 = 0;
    uint32_t index = 0;

    /* PDM Demux */
    for (index = 0; index < INTERNAL_BUFF_SIZE / 2; index++)
    {
        byte2 = (pdmbuf[index] >> 8) & 0xFF;
        byte1 = (pdmbuf[index] & 0xFF);
        app_pdm[(index * 2) + 1] = Channel_Demux[byte1 & CHANNEL_DEMUX_MASK] | Channel_Demux[byte2 & CHANNEL_DEMUX_MASK] << 4;
        app_pdm[(index * 2)] = Channel_Demux[(byte1 >> 1) & CHANNEL_DEMUX_MASK] | Channel_Demux[(byte2 >> 1) & CHANNEL_DEMUX_MASK] << 4;
    }

    for (index = 0; index < 2; index++)
    {
        /* PDM to PCM filter */
        PDM_Filter((uint8_t *)&app_pdm[index], (uint16_t *) & (pcmbuf[index]), &PDM_FilterHandler[index]);
    }

    /* Return 0 when all operations are correctly done */
    return 0;
}
static uint16_t * ppcm_buffer;
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    uint16_t * ppdm_buffer = &mic_pdm_buffer[INTERNAL_BUFF_SIZE/2];
    ppcm_buffer = rt_mp_alloc(mic_mp, 0);
    if(ppcm_buffer != RT_NULL)
    {
        pdm2pcm(ppdm_buffer, &ppcm_buffer[0]);
        rt_mb_send(mic_mb, (rt_ubase_t)ppcm_buffer);
        ppcm_buffer = RT_NULL;
    }
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    uint16_t * ppdm_buffer = &mic_pdm_buffer[0];
    ppcm_buffer = rt_mp_alloc(mic_mp, 0);
    if(ppcm_buffer != RT_NULL)
    {
        pdm2pcm(ppdm_buffer, ppcm_buffer);
        rt_mb_send(mic_mb, (rt_ubase_t)ppcm_buffer);
        ppcm_buffer = RT_NULL;
    }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    rt_kprintf("HAL_I2S_ErrorCallback\n");
}

rt_mailbox_t mic_start_record(int fifo_len)
{
    mic_mb = rt_mb_create("mic_mb", fifo_len, RT_IPC_FLAG_FIFO);
    mic_mp = rt_mp_create("mic_mp", fifo_len, PCM_OUT_SIZE * 2);
    HAL_I2S_Receive_DMA(&haudio_in_i2s, mic_pdm_buffer, INTERNAL_BUFF_SIZE);
    return mic_mb;
}

int mic_stop_record(void)
{
    HAL_I2S_DMAStop(&haudio_in_i2s);
    rt_mb_delete(mic_mb);
    mic_mb = RT_NULL;
    rt_mp_delete(mic_mp);
    mic_mp = RT_NULL;
    return 0;
}

void pcm_stereo_to_mono(const uint16_t * stereo_buffer, size_t stereo_len, uint16_t * mono_buffer)
{
    int i;
    for(i = 0; i < stereo_len / 2; i++)
    {
        mono_buffer[i] = stereo_buffer[2*i];
    }
}
