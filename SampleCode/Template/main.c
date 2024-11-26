/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "M0518.h"

#include "misc_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_TM2_CAPTURE_RDY  			            (flag_PROJ_CTL.bit1)
#define FLAG_PROJ_REVERSE2                 				(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_REVERSE3                              (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

#define PLL_CLOCK   50000000

volatile uint32_t g_u32CaptureFinish = 0;
volatile uint32_t g_au32TMRCapCountRise = 0; 
volatile uint32_t g_au32TMRCapCountFall = 0; 

uint32_t u32CAPValue = 0;

enum
{
    TMR_IDX_50MS = 1,
    TMR_IDX_100MS,
    TMR_IDX_125MS,
    TMR_IDX_250MS,
    TMR_IDX_400MS,
    TMR_IDX_500MS,
    TMR_IDX_750MS,
    TMR_IDX_875MS,
    TMR_IDX_1000MS,
};

unsigned char TMR_CURRENT_IDX = TMR_IDX_50MS;
unsigned char TMR_TARGET_IDX = TMR_IDX_50MS;

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

unsigned int get_systick(void)
{
	return (counter_systick);
}

void set_systick(unsigned int t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned int delay)
{  
    
    unsigned int tickstart = get_systick(); 
    unsigned int wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(unsigned int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    // if (get_tick() >= 60000)
    // {
    //     set_tick(0);
    // }
}

void delay_ms(uint16_t ms)
{
	#if 1
    uint32_t tickstart = get_tick();
    uint32_t wait = ms;
	uint32_t tmp = 0;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000 -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
	
	#else
	TIMER_Delay(TIMER0, 1000*ms);
	#endif
}


void TMR2_capture_log(void)
{
    if (FLAG_PROJ_TM2_CAPTURE_RDY)
    {
        FLAG_PROJ_TM2_CAPTURE_RDY = 0;
        printf("rising:0x%X (%d)\r\n", g_au32TMRCapCountRise, g_au32TMRCapCountRise);
        printf("falling:0x%X (%d)\r\n", g_au32TMRCapCountFall, g_au32TMRCapCountFall);
        printf("Capture counter for high level duration: 0x%X (%d)\n", u32CAPValue, u32CAPValue);
        printf("The high level duration time should be about (TIMER2 clock) * %d\n", u32CAPValue);
        printf("                                             = %d us\n", u32CAPValue / (SystemCoreClock / (9+1) / 1000000));


        u32CAPValue = 0;
        g_u32CaptureFinish = 0;
        g_au32TMRCapCountRise = 0;
        g_au32TMRCapCountFall = 0;

    }
}

void TMR2_process_irq(void)
{

    if (g_u32CaptureFinish)
    {
        if (g_au32TMRCapCountFall > g_au32TMRCapCountRise)
            u32CAPValue = g_au32TMRCapCountFall - g_au32TMRCapCountRise;
        else
            u32CAPValue = 0xFFFFFF - g_au32TMRCapCountRise + g_au32TMRCapCountFall;

        FLAG_PROJ_TM2_CAPTURE_RDY = 1;
    }

}

void TMR2_process_loop(void)
{
    u32CAPValue = 0;
    g_u32CaptureFinish = 0;
    g_au32TMRCapCountRise = 0;
    g_au32TMRCapCountFall = 0;

    // TIMER_Start(TIMER2);

    while (g_u32CaptureFinish == 0);

    // TIMER_Stop(TIMER2);

    // while (TIMER_IS_ACTIVE(TIMER2)) {};

    // TIMER_ClearCaptureIntFlag(TIMER2);

    if (g_au32TMRCapCountFall > g_au32TMRCapCountRise)
        u32CAPValue = g_au32TMRCapCountFall - g_au32TMRCapCountRise;
    else
        u32CAPValue = 0xFFFFFF - g_au32TMRCapCountRise + g_au32TMRCapCountFall;

    FLAG_PROJ_TM2_CAPTURE_RDY = 1;
}

void TMR2_get_edge(void)
{
    if (PB2)
    {
        /* Save the capture counter for PB2 rising edge */
        g_au32TMRCapCountRise = TIMER_GetCaptureData(TIMER2);
    }
    else
    {
        /* Save the capture counter for PB2 falling edge */
        g_au32TMRCapCountFall = TIMER_GetCaptureData(TIMER2);
        g_u32CaptureFinish = 1;
        TMR2_process_irq();
    }    
}

void TMR2_IRQHandler(void)
{
    if(TIMER_GetCaptureIntFlag(TIMER2) == 1)
    {
        TIMER_ClearCaptureIntFlag(TIMER2);

        TMR2_get_edge();
    }
}

void TIMER2_Init(void)
{
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB2_Msk );
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB2_TM2_EXT );
    SYS->ALT_MFP &= ~SYS_ALT_MFP_PB2_Msk;
    SYS->ALT_MFP |= SYS_ALT_MFP_PB2_TM2_EXT ;

    SYS_ResetModule(TMR2_RST);
    TIMER_Open(TIMER2, TIMER_CONTINUOUS_MODE, 1);

   /* Set TIMER2 clock to HCLK/(1+9) */
    TIMER_SET_PRESCALE_VALUE(TIMER2, 9);    // 5Mz clock source
    TIMER_SET_CMP_VALUE(TIMER2, 0xFFFFFF);
    TIMER_EnableCapture(TIMER2, TIMER_CAPTURE_FREE_COUNTING_MODE, TIMER_CAPTURE_FALLING_AND_RISING_EDGE);
    TIMER_EnableCaptureInt(TIMER2);
    TIMER_Start(TIMER2);

    NVIC_EnableIRQ(TMR2_IRQn);
    NVIC_SetPriority(TMR2_IRQn,0);

}

//
// check_reset_source
//
uint8_t check_reset_source(void)
{
    uint32_t src = SYS_GetResetSrc();

    SYS->RSTSRC |= 0xFF;
    printf("Reset Source <0x%08X>\r\n", src);

    #if 1   //DEBUG , list reset source
    if (src & BIT0)
    {
        printf("0)Power-On Reset Flag\r\n");       
    }
    if (src & BIT1)
    {
        printf("1)Reset Pin Reset Flag\r\n");       
    }
    if (src & BIT2)
    {
        printf("2)Watchdog Timer Reset Flag\r\n");       
    }
    if (src & BIT3)
    {
        printf("3)Low Voltage Reset Flag\r\n");       
    }
    if (src & BIT4)
    {
        printf("4)Brown-Out Detector Reset Flag\r\n");       
    }
    if (src & BIT5)
    {
        printf("5)SYS Reset Flag\r\n");       
    }
    if (src & BIT6)
    {
        printf("6)Reserved.\r\n");       
    }
    if (src & BIT7)
    {
        printf("7)CPU Reset Flag\r\n");       
    }
    if (src & BIT8)
    {
        printf("8)Reserved.\r\n");       
    }
    #endif
    
    if (src & SYS_RSTSRC_RSTS_POR_Msk) {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_POR_Msk);
        
        printf("power on from POR\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_RESET_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_RESET_Msk);
        
        printf("power on from nRESET pin\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSRC_RSTS_WDT_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_WDT_Msk);
        
        printf("power on from WDT Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_LVR_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_LVR_Msk);
        
        printf("power on from LVR Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_BOD_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_BOD_Msk);
        
        printf("power on from BOD Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_SYS_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_SYS_Msk);
        
        printf("power on from System Reset\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSRC_RSTS_CPU_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_CPU_Msk);

        printf("power on from CPU reset\r\n");
        return FALSE;         
    }    
    
    printf("power on from unhandle reset source\r\n");
    return FALSE;
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_1000MS = 1;//set_flag(flag_timer_period_1000ms ,ENABLE);
            
 
            if (TMR_IDX_1000MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 875) == 0)
		{      
            if (TMR_IDX_875MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 750) == 0)
		{      
            if (TMR_IDX_750MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 500) == 0)
		{      
            if (TMR_IDX_500MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 400) == 0)
		{      
            if (TMR_IDX_400MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 250) == 0)
		{      
            if (TMR_IDX_250MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 125) == 0)
		{      
            if (TMR_IDX_125MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 100) == 0)
		{      
            if (TMR_IDX_100MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}
		if ((get_tick() % 50) == 0)
		{  
            if (TMR_IDX_50MS == TMR_TARGET_IDX)
            {
                PA5 ^= 1;
            }
		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	// static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)//(is_flag_set(flag_timer_period_1000ms))
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0;//set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PB13 ^= 1;        
    }


    // TMR2_process_loop();
    TMR2_capture_log();
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		printf("press : %c\r\n" , res);
		switch(res)
		{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
                TMR_TARGET_IDX = res - 0x30;
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART02_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT_Msk | UART_ISR_TOUT_INT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FSR & (UART_FSR_BIF_Msk | UART_FSR_FEF_Msk | UART_FSR_PEF_Msk | UART_FSR_RX_OVER_IF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_ISR_RLS_INT_Msk| UART_ISR_BUF_ERR_INT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_IER_RDA_IEN_Msk | UART_IER_TOUT_IEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	dbg_printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	dbg_printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());	
	dbg_printf("CLK_GetPCLKFreq : %8d\r\n",CLK_GetPCLKFreq());    
	dbg_printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());    
	#endif	

    #if 0
    dbg_printf("FLAG_PROJ_TIMER_PERIOD_1000MS : 0x%2X\r\n",FLAG_PROJ_TIMER_PERIOD_1000MS);
    dbg_printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    dbg_printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    dbg_printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    dbg_printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    dbg_printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    dbg_printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    dbg_printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif

}

void GPIO_Init (void)
{
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB13_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB13_GPIO);

    GPIO_SetMode(PB, BIT13, GPIO_PMD_OUTPUT);


    SYS->GPA_MFP &= ~(SYS_GPA_MFP_PA5_Msk);
    SYS->GPA_MFP |= (SYS_GPA_MFP_PA5_GPIO);

    GPIO_SetMode(PA, BIT5, GPIO_PMD_OUTPUT);

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
 
    /* Enable Internal RC 22.1184MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    // CLK_EnableXtalRC(CLK_PWRCON_IRC10K_EN_Msk);
    // CLK_WaitClockReady(CLK_CLKSTATUS_IRC10K_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /***********************************/
    // CLK_EnableModuleClock(TMR0_MODULE);
    // CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HIRC, 0);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HIRC, 0);
    
	/***********************************/
    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HIRC, CLK_CLKDIV_UART(1));
	
    /* Set GPB multi-function pins for UART0 RXD(PB.0) and TXD(PB.1) */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD);

	/***********************************/

    CLK_EnableModuleClock(TMR2_MODULE);
    CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2_S_HCLK, 0);

	/***********************************/
   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
	TIMER1_Init();
    check_reset_source();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    TIMER2_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
