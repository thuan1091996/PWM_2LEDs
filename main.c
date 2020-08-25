/* Auto change PWM after 2s using interrupt, each time +20%
 * Click button PF4 to change the colour of LED
 */

// Include needed libraries
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.c"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.c"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/interrupt.c"
#include "driverlib/timer.h"
#include "driverlib/timer.c"
#include "driverlib/pwm.h"

//Define variable and devices
#define PORT_F GPIO_PORTF_BASE
#define RED_LED                 GPIO_PIN_1
#define BLUE_LED                GPIO_PIN_2
#define SW                      GPIO_PIN_4
#define TimerUse_BASE           TIMER1_BASE
#define PWM_MODULE_BASE         PWM1_BASE
uint32_t freq;
uint32_t freq_pwm;
uint32_t duty_red,duty_blue;
uint8_t state=1,Duty_RED=1,Duty_BLUE=1 ;
//Subroutine
void GPIO_ISR(void)
{
    GPIOIntClear(PORT_F, GPIO_INT_PIN_4);
    SysCtlDelay( SysCtlClockGet()/3/2);
    state=(state+1)%2;
}
void Timer_ISR(void)
{
    TimerIntClear(TimerUse_BASE, TIMER_TIMA_TIMEOUT);
    if (state==0)
    {
        Duty_RED=(Duty_RED+10)%100;
        Duty_BLUE=1;

    }
    else
    {
        Duty_BLUE=(Duty_BLUE+10)%100;
        Duty_RED=1;
    }
}
void Process(void)
{
    if (state==0)
    {
        PWMGenDisable(PWM_MODULE_BASE, PWM_GEN_3);
        PWMOutputState(PWM_MODULE_BASE, PWM_OUT_6_BIT, false);
        PWMGenEnable(PWM_MODULE_BASE, PWM_GEN_2);
        PWMPulseWidthSet(PWM_MODULE_BASE, PWM_OUT_5, duty_red);
        PWMOutputState(PWM_MODULE_BASE, PWM_OUT_5_BIT, true);
        SysCtlDelay(SysCtlClockGet()/3);
    }
    else
    {
        PWMGenDisable(PWM_MODULE_BASE, PWM_GEN_2);
        PWMOutputState(PWM_MODULE_BASE, PWM_OUT_5_BIT, false);
        PWMGenEnable(PWM_MODULE_BASE, PWM_GEN_3);
        PWMPulseWidthSet(PWM_MODULE_BASE, PWM_OUT_6, duty_blue);
        PWMOutputState(PWM_MODULE_BASE, PWM_OUT_6_BIT, true);
        SysCtlDelay(SysCtlClockGet()/3);
    }
}
int main(void)
{
    //Clock settings
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //50Mhz
    freq=SysCtlClockGet()/0.5; //Fdesired=0.5 Hz
    freq_pwm=SysCtlClockGet()/10000; //Fpwm=10k Hz

    //Enable Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    //GPIO CONFIGURE
    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
    GPIOPinTypePWM(PORT_F, RED_LED);
    GPIOPinTypePWM(PORT_F, BLUE_LED);
    GPIOPinTypeGPIOInput(PORT_F, SW);
    GPIOPadConfigSet(PORT_F, SW, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU); //Pull up

    //TIMER CONFIGURE
    TimerConfigure(TimerUse_BASE, TIMER_CFG_PERIODIC);  //Full width
    TimerLoadSet(TimerUse_BASE, TIMER_A, freq-1);
    TimerEnable(TimerUse_BASE, TIMER_A);

    //PWM CONFIGURE
    //SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
    PWMClockSet(PWM_MODULE_BASE, PWM_SYSCLK_DIV_1);

    PWMGenConfigure(PWM_MODULE_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM_MODULE_BASE, PWM_GEN_2, freq_pwm);
    //PWMGenEnable(PWM_MODULE_BASE, PWM_GEN_2);

    PWMGenConfigure(PWM_MODULE_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM_MODULE_BASE, PWM_GEN_3, freq_pwm);
    //PWMGenEnable(PWM_MODULE_BASE, PWM_GEN_3);

    //INTERRUPT CONFIGURE
    IntMasterEnable();

    TimerIntRegister(TimerUse_BASE, TIMER_A, Timer_ISR);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TimerUse_BASE, TIMER_TIMA_TIMEOUT);

    GPIOIntRegister(PORT_F, GPIO_ISR);
    IntEnable(INT_GPIOF);
    GPIOIntEnable(PORT_F, GPIO_INT_PIN_4);
    while (1)
    {
        duty_red= freq_pwm*Duty_RED/100;
        duty_blue=freq_pwm*Duty_BLUE/100;
        Process();
    }
}
