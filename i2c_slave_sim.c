/***************************************************************************************************
* FILE: i2c_slave_sim.c

* DESCRIPTION:
--

* CREATED: 2017/3/25 by kevin
****************************************************************************************************
* MODIFY:  --

* DESCRIPTION: --

* DATE: --
***************************************************************************************************/

/***************************************************************************************************
* INCLUDE FILES
***************************************************************************************************/
#include "i2c_slave_sim.h"
#include "i2c_parser.h"
/***************************************************************************************************
* LOCAL DEFINES
***************************************************************************************************/
#define I2C_SCL_L()   GPIOA->BRR |= (1<<9)
#define I2C_SCL_H()   GPIOA->BSRR |= (1<<9)
#define I2C_SDA_L()   GPIOA->BRR |= (1<<10)
#define I2C_SDA_H()   GPIOA->BSRR |= (1<<10)

#define I2C_SCL_IN  GPIOA->IDR & (1<<9)
#define I2C_SDA_IN  GPIOA->IDR & (1<<10)


#define I2C_ACK() 	GPIOA->BRR |= (1<<10)
#define I2C_NACK()  GPIOA->BSRR |= (1<<10)

//I2C 
typedef enum
{
    I2C_MODE_IDLE,	//
    I2C_MODE_ADDR,	//
    I2C_MODE_READ,	//
    I2C_MODE_WRITE,	//
    I2C_MODE_BUSY	//
} EN_I2C_SLAVE_MODE;

EN_I2C_SLAVE_MODE I2C_SLAVE_MODE = I2C_MODE_IDLE;

uint8_t Data_Buffer[6] = {0};
static uint8_t Data_Pointer = 0;
static uint8_t Data_Cache = 0;
static uint8_t Counter_SCL_Fall = 0;


void I2C_Slave_Init(void)
{
    I2C_Port_Init();
}

void I2C_Slave_SCL_Raise(void)
{
    switch(I2C_SLAVE_MODE)
    {
        case I2C_MODE_ADDR:
        case I2C_MODE_READ:
        {
            Data_Cache <<= 1;
            if(I2C_SDA_IN)
            {
                Data_Cache++;
            }
        }
        break;
        case I2C_MODE_WRITE:
        {
            
        }
        break;
        default:
        break;
    }
}

void I2C_Slave_SCL_Fall(void)
{
    I2C_SCL_L();
    
    Counter_SCL_Fall++;
    if(Counter_SCL_Fall == 10)
    {
        I2C_NACK();
        Counter_SCL_Fall = 1;
        Data_Cache = 0;
    }
    
    switch(I2C_SLAVE_MODE)
    {
        case I2C_MODE_ADDR:
        if(Counter_SCL_Fall == 9)
        {
            I2C_ACK();		//ACK
            if(I2C_OWN_ADDR == (Data_Cache & 0xFE)) 
            {
                if(Data_Cache & 0x01)
                {
                    I2C_SLAVE_MODE = I2C_MODE_WRITE;
                }
                else									
                {
                    I2C_SLAVE_MODE = I2C_MODE_READ;
                    Data_Pointer = 0;		
                }
            }
            else
            {
                I2C_SLAVE_MODE = I2C_MODE_BUSY;//
            }
        }
        break;
        case I2C_MODE_READ:
        if(Counter_SCL_Fall == 9)
        {                    
            I2C_ACK();	//ACK
            Data_Buffer[Data_Pointer++] = Data_Cache;	
            
            switch(Data_Buffer[0])
            {
                case PICTURE:
                case RESOLUTION:
                case FPS:
                case USB2: 
                if(Data_Pointer == 3) 
                    I2C_Data_Parse(Data_Buffer); 
                break;
                case ZOOM_PTZ:
                switch(Data_Buffer[1])
                {
                    case PT_REL:
                    case PT_ABS: 
                    if(Data_Pointer == 6) 
                        I2C_Data_Parse(Data_Buffer);
                    break;
                    case ZOOM_REL:
                    if(Data_Pointer == 3)
                        I2C_Data_Parse(Data_Buffer);
                    break;
                    case ZOOM_ABS:
                    default:
                    if(Data_Pointer == 4)
                        I2C_Data_Parse(Data_Buffer);
                    break;
                }
                break; 
                default: break;
            }
            
        }
        break;
        case I2C_MODE_WRITE:
        break;
        default:
        break;
    }
    
    I2C_SCL_H();
}

void I2C_Slave_SDA_Raise(void)
{
    if(I2C_SCL_IN)
    {
        I2C_SLAVE_MODE = I2C_MODE_IDLE;
        Counter_SCL_Fall = 0;
        Data_Cache = 0;
    }
}

void I2C_Slave_SDA_Fall(void)
{
    if(I2C_SCL_IN)
    {
        I2C_SLAVE_MODE = I2C_MODE_ADDR;
        Counter_SCL_Fall = 0;
        Data_Cache = 0;
    }
}

/***************************************************************************************************
* Description:
***************************************************************************************************/
void I2C_Port_Init(void)
{    
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    I2C_SCL_H();
    I2C_SDA_H();
    
    
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource9);  //SCL
    EXTI_InitTypeDef EXTI_InitStructure;  
    EXTI_InitStructure.EXTI_Line = EXTI_Line9;  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
    EXTI_Init(&EXTI_InitStructure);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource10);  //SDA
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line10;  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
    EXTI_Init(&EXTI_InitStructure);
    
    NVIC_InitTypeDef NVIC_InitStructure;  
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_15_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0;  
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
    NVIC_Init(&NVIC_InitStructure);  
}

void EXTI4_15_IRQHandler(void)
{
    if(EXTI->PR & (1<<9)) 
    {
        if(GPIOA->IDR & (1<<9))                                         //rising
        {
            I2C_Slave_SCL_Raise();
        }
        else
        {
            I2C_Slave_SCL_Fall();
        }
        EXTI->PR |= (1<<9);
    }
    
    if(EXTI->PR & (1<<10)) 
    {
        if(GPIOA->IDR & (1<<10))                                         //rising
        {
            I2C_Slave_SDA_Raise();  
        }
        else
        {
            I2C_Slave_SDA_Fall();
        }
        EXTI->PR |= (1<<10);
    }
}
/******************************* (C) COPYRIGHT 2017 Shenzhen JJTS ***********END OF FILE***********/

