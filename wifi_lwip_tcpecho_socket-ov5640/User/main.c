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
#include "./camera/bsp_ov5640.h"
#include "./sdram/bsp_sdram.h" 
/** @endcond */
#define HARDWARE_VERSION               "V1.0.0"
#define SOFTWARE_VERSION               "V0.1.0"

/** @endcond */

/*****************************************************
 *               静态函数声明
 ******************************************************/

static void tcpip_init_done( void * arg );
static void startup_thread( void *arg );
static void SystemClock_Config(void);

/******************************************************
 *               变量定义
 ******************************************************/

static TaskHandle_t  startup_thread_handle;
void BSP_Init();



//int main(void)

//{  
//	OV5640_IDTypeDef OV5640_Camera_ID;	
//	/* 系统时钟初始化成400MHz */
//	SystemClock_Config();


//	/* 配置串口1为：115200 8-N-1 */
//  DEBUG_USART_Config();

//	CAMERA_DEBUG("STM32H743 DCMI 驱动OV5640例程");
//	I2CMaster_Init();
//	OV5640_HW_Init();			
//	//初始化 I2C
//	
//	/* 读取摄像头芯片ID，确定摄像头正常连接 */
//	OV5640_ReadID(&OV5640_Camera_ID);

//	if(OV5640_Camera_ID.PIDH  == 0x56)
//	{
//		CAMERA_DEBUG("%x%x",OV5640_Camera_ID.PIDH ,OV5640_Camera_ID.PIDL);
//	}
//	else
//	{
//		CAMERA_DEBUG("没有检测到OV5640摄像头，请重新检查连接。");
//		while(1);  
//	}
//		/* 配置摄像头输出像素格式 */
//	//OV5640_RGB565Config();	
//	OV5640_JPEGConfig(JPEG_IMAGE_FORMAT);
//	/* 初始化摄像头，捕获并显示图像 */
//	OV5640_Init();
//	//刷OV5640的自动对焦固件
//	OV5640_AUTO_FOCUS();

//	while(1)
//	{

//	}  
//}

/**
 * 主函数
 */

int main( void )
{
	
		BSP_Init();

	
    /*创建一个初始线程 */									
		BaseType_t xReturn = pdPASS;
		xReturn = xTaskCreate((TaskFunction_t )startup_thread,  /* 任务入口函数 */
													(const char*    )"app_thread",/* 任务名字 */
													(uint16_t       )APP_THREAD_STACKSIZE/sizeof( portSTACK_TYPE ),  /* 任务栈大小 */
													(void*          )NULL,/* 任务入口函数参数 */
													(UBaseType_t    )DEFAULT_THREAD_PRIO, /* 任务的优先级 */
													(TaskHandle_t*  )&startup_thread_handle);/* 任务控制块指针 */ 
		 /* 启动任务调度 */           
		if(pdPASS == xReturn)
			vTaskStartScheduler();   /* 启动任务，开启调度 */
		else
			return -1;  
    /* 除非vTaskStartScheduler中出现错误，否则永远不要到这里 */
    WPRINT_APP_ERROR(("Main() function returned - error" ));
    return 0;
}



/**
 *  初始线程功能-启动LwIP并调用app_main
 * 该函数使用tcpip_init函数启动LwIP，然后等待信号量，
 * 直到LwIP通过调用回调@ref tcpip_init_done指示其已
 * 启动。完成后，将调用应用程序的@ref app_main函数。
 * @param arg :  未使用-符合线程功能原型所需
 */

static void startup_thread( void *arg )
{
    SemaphoreHandle_t lwip_done_sema;
    UNUSED_PARAMETER( arg);

   // WPRINT_APP_INFO( ( "\nPlatform " PLATFORM " initialised\n" ) );
    WPRINT_APP_INFO( ( "Started FreeRTOS" FreeRTOS_VERSION "\n" ) );
    WPRINT_APP_INFO( ( "Starting LwIP " LwIP_VERSION "\n" ) );

    /* 创建信号量以在LwIP完成初始化时发出信号 */
    lwip_done_sema = xSemaphoreCreateCounting( 1, 0 );
    if ( lwip_done_sema == NULL )	
    {
        /*无法创建信号量 */
        WPRINT_APP_ERROR(("Could not create LwIP init semaphore"));
        return;
    }

    /*初始化LwIP，提供回调函数和回调信号量 */
    tcpip_init( tcpip_init_done, (void*) &lwip_done_sema );
    xSemaphoreTake( lwip_done_sema, portMAX_DELAY );
    vQueueDelete( lwip_done_sema );

    /* 运行主应用程序功能 */
    app_main( );

    /* 清理此启动线程*/
    vTaskDelete( startup_thread_handle );
}



/**
 *  LwIP初始化完成回调
 * @param arg : the handle for the semaphore to post (cast to a void pointer)
 */

static void tcpip_init_done( void * arg )
{
    SemaphoreHandle_t * lwip_done_sema = (SemaphoreHandle_t *) arg;
    xSemaphoreGive( *lwip_done_sema );
}


/***********************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  *********************************************************************/

static void BSP_Init(void)
{
	
  /* 系统时钟初始化成400MHz */
	SystemClock_Config();
  
		platform_init_mcu_infrastructure();  
	
  /* 初始化SysTick */
  HAL_SYSTICK_Config( HAL_RCC_GetSysClockFreq() / configTICK_RATE_HZ );	

	/* usart 端口初始化 */
  DEBUG_USART_Config();
	/* CmBacktrace initialize */
  cm_backtrace_init("Fire_H7", HARDWARE_VERSION, SOFTWARE_VERSION);
	
	//SDRAM_Init();//初始化外部sdram


  
}

/**
  * @brief  System Clock 配置
  *         system Clock 配置如下: 
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
  
  /*使能供电配置更新 */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* 当器件的时钟频率低于最大系统频率时，电压调节可以优化功耗，
		 关于系统频率的电压调节值的更新可以参考产品数据手册。  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* 启用HSE振荡器并使用HSE作为源激活PLL */
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
  
	/* 选择PLL作为系统时钟源并配置总线时钟分频器 */
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
