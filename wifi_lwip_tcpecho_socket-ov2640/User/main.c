#include "wwd_network.h"
#include "wwd_management.h"
#include "wwd_wifi.h"
#include "wwd_debug.h"
#include "wwd_assert.h"
#include "platform/wwd_platform_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_buffer_interface.h"
#include "wifi_base_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "./usart/bsp_debug_usart.h"
#include "platform_init.h"
#include <cm_backtrace.h>
#include "./i2c/bsp_i2c.h"
#include "./camera/bsp_ov2640.h"
#include "./sdram/bsp_sdram.h" 
/** @endcond */
#define HARDWARE_VERSION               "V1.0.0"
#define SOFTWARE_VERSION               "V0.1.0"

/** @endcond */

/*****************************************************
 *               ��̬��������
 ******************************************************/

static void tcpip_init_done( void * arg );
static void startup_thread( void *arg );

/******************************************************
 *               ��������
 ******************************************************/

static TaskHandle_t  startup_thread_handle;
void BSP_Init();


/**
 * ������
 */

int main( void )
{
	
		BSP_Init();

	
    /*����һ����ʼ�߳� */									
		BaseType_t xReturn = pdPASS;
		xReturn = xTaskCreate((TaskFunction_t )startup_thread,  /* ������ں��� */
													(const char*    )"app_thread",/* �������� */
													(uint16_t       )APP_THREAD_STACKSIZE/sizeof( portSTACK_TYPE ),  /* ����ջ��С */
													(void*          )NULL,/* ������ں������� */
													(UBaseType_t    )DEFAULT_THREAD_PRIO, /* ��������ȼ� */
													(TaskHandle_t*  )&startup_thread_handle);/* ������ƿ�ָ�� */ 
		 /* ����������� */           
		if(pdPASS == xReturn)
			vTaskStartScheduler();   /* �������񣬿������� */
		else
			return -1;  
    /* ����vTaskStartScheduler�г��ִ��󣬷�����Զ��Ҫ������ */
    WPRINT_APP_ERROR(("Main() function returned - error" ));
    return 0;
}



/**
 *  ��ʼ�̹߳���-����LwIP������app_main
 * �ú���ʹ��tcpip_init��������LwIP��Ȼ��ȴ��ź�����
 * ֱ��LwIPͨ�����ûص�@ref tcpip_init_doneָʾ����
 * ��������ɺ󣬽�����Ӧ�ó����@ref app_main������
 * @param arg :  δʹ��-�����̹߳���ԭ������
 */

static void startup_thread( void *arg )
{
    SemaphoreHandle_t lwip_done_sema;
    UNUSED_PARAMETER( arg);

   // WPRINT_APP_INFO( ( "\nPlatform " PLATFORM " initialised\n" ) );
    WPRINT_APP_INFO( ( "Started FreeRTOS" FreeRTOS_VERSION "\n" ) );
    WPRINT_APP_INFO( ( "Starting LwIP " LwIP_VERSION "\n" ) );

    /* �����ź�������LwIP��ɳ�ʼ��ʱ�����ź� */
    lwip_done_sema = xSemaphoreCreateCounting( 1, 0 );
    if ( lwip_done_sema == NULL )	
    {
        /*�޷������ź��� */
        WPRINT_APP_ERROR(("Could not create LwIP init semaphore"));
        return;
    }

    /*��ʼ��LwIP���ṩ�ص������ͻص��ź��� */
    tcpip_init( tcpip_init_done, (void*) &lwip_done_sema );
    xSemaphoreTake( lwip_done_sema, portMAX_DELAY );
    vQueueDelete( lwip_done_sema );

    /* ������Ӧ�ó����� */
    app_main( );

    /* ����������߳�*/
    vTaskDelete( startup_thread_handle );
}



/**
 *  LwIP��ʼ����ɻص�
 * @param arg : the handle for the semaphore to post (cast to a void pointer)
 */

static void tcpip_init_done( void * arg )
{
    SemaphoreHandle_t * lwip_done_sema = (SemaphoreHandle_t *) arg;
    xSemaphoreGive( *lwip_done_sema );
}


/***********************************************************************
  * @ ������  �� BSP_Init
  * @ ����˵���� �弶�����ʼ�������а����ϵĳ�ʼ�����ɷ��������������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  *********************************************************************/
static void SystemClock_Config(void);
static void BSP_Init(void)
{
	
  /* ϵͳʱ�ӳ�ʼ����400MHz */
	SystemClock_Config();
  
		platform_init_mcu_infrastructure();  
	
  /* ��ʼ��SysTick */
  HAL_SYSTICK_Config( HAL_RCC_GetSysClockFreq() / configTICK_RATE_HZ );	

	/* usart �˿ڳ�ʼ�� */
  DEBUG_USART_Config();
	/* CmBacktrace initialize */
  cm_backtrace_init("Fire_H7", HARDWARE_VERSION, SOFTWARE_VERSION);
	
	SDRAM_Init();//��ʼ���ⲿsdram


  
}

/**
  * @brief  System Clock ����
  *         system Clock ��������: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 400000000 (CPU Clock)
	*            HCLK(Hz)             = 200000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  100MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  100MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  100MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  100MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 160
	*            PLL_P                = 2
	*            PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*ʹ�ܹ������ø��� */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* ��������ʱ��Ƶ�ʵ������ϵͳƵ��ʱ����ѹ���ڿ����Ż����ģ�
		 ����ϵͳƵ�ʵĵ�ѹ����ֵ�ĸ��¿��Բο���Ʒ�����ֲᡣ  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* ����HSE������ʹ��HSE��ΪԴ����PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
	/* ѡ��PLL��Ϊϵͳʱ��Դ����������ʱ�ӷ�Ƶ�� */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK  | \
																 RCC_CLOCKTYPE_HCLK    | \
																 RCC_CLOCKTYPE_D1PCLK1 | \
																 RCC_CLOCKTYPE_PCLK1   | \
                                 RCC_CLOCKTYPE_PCLK2   | \
																 RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}
/****************************END OF FILE***************************/
