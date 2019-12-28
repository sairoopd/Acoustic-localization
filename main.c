#include <stdbool.h>
#include <stdint.h>
#include "math.h"
#include "arm_math.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_adc.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"


#define ADC_SAMPLE_BUF_SIZE     1024
unsigned int samplingFrequency=48000;


// -------------------------------------------------------------------
 // Declare State buffer of size (numTaps + blockSize - 1)
 // ------------------------------------------------------------------

/// ------------------------------------------------------------------
///  Global variables for FIR LPF Example
///  -------------------------------------------------------------------

uint32_t blockSize = ADC_SAMPLE_BUF_SIZE/2;


enum BUFFERSTATUS
                  { EMPTY,
                    FILLING,
                    FULL
                  };

#pragma DATA_ALIGN(ucControlTable, 1024)
uint8_t ucControlTable[1024];

//Buffers to hold alternating samples from each channel
//Two Channel
static uint16_t ADC_Out1[ADC_SAMPLE_BUF_SIZE];
static uint16_t ADC_Out2[ADC_SAMPLE_BUF_SIZE];

static uint32_t Converted_ADC_Out1_PE3[ADC_SAMPLE_BUF_SIZE/4];
static uint32_t Converted_ADC_Out2_PE3[ADC_SAMPLE_BUF_SIZE/4];



static uint32_t Converted_ADC_Out1_PE2[ADC_SAMPLE_BUF_SIZE/4];
static uint32_t Converted_ADC_Out2_PE2[ADC_SAMPLE_BUF_SIZE/4];

static uint32_t Converted_ADC_Out1_PE1[ADC_SAMPLE_BUF_SIZE/4];
static uint32_t Converted_ADC_Out2_PE1[ADC_SAMPLE_BUF_SIZE/4];

static uint32_t Converted_ADC_Out1_PE0[ADC_SAMPLE_BUF_SIZE/4];
static uint32_t Converted_ADC_Out2_PE0[ADC_SAMPLE_BUF_SIZE/4];

static enum BUFFERSTATUS BufferStatus[2];

float32_t average1, average2, average3, average4;
float32_t rms1, rms2, rms3, rms4;
static uint32_t g_ui32DMAErrCount = 0u;
static uint32_t g_ui32SysTickCount;




int firstLoop=1;

void init_DMA(void);
void init_ADC01(void);
void init_TIMER(void);

void uDMAErrorHandler(void)
{
    uint32_t ui32Status;
    ui32Status = MAP_uDMAErrorStatusGet();
    if(ui32Status)
    {
        MAP_uDMAErrorStatusClear();
        g_ui32DMAErrCount++;
    }
}

// Not used in this example, but used to debug to make sure timer interrupts happen
void Timer0AIntHandler(void)
{
    //
    // Clear the timer interrupt flag.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}

void SysTickIntHandler(void)
{
    // Update our system tick counter.
    g_ui32SysTickCount++;
}

void ADCseq0Handler()
{
    ADCIntClear(ADC0_BASE, 0);

    if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT) == UDMA_MODE_STOP)
            && (BufferStatus[0] == FILLING))
    {
        BufferStatus[0] = FULL;
        BufferStatus[1] = FILLING;
    }
    else if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT) == UDMA_MODE_STOP)
            && (BufferStatus[1] == FILLING))

    {
        BufferStatus[0] = FILLING;
        BufferStatus[1] = FULL;
    }
}

int main(void)
{
//    int debugFlag=0;


    uint32_t i, samples_taken;
    uint32_t RMS1, RMS2;

    BufferStatus[0] = FILLING;
    BufferStatus[1] = EMPTY;
    samples_taken = 0u;

    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

    SysCtlDelay(20u);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);  //Enable the clock to TIMER0
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);    //Enable the clock to ADC0 module
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);    //Enable the clock to ADC1 module
		MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);    //Enable the clock to uDMA
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);   //Enables the clock to PORT E
    MAP_SysCtlDelay(30u);

    MAP_SysTickPeriodSet(SysCtlClockGet() / 100000u); //Sets the period of the SysTic counter to 10us
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    init_DMA();
    init_ADC01();
    init_TIMER();

    MAP_IntMasterEnable();
    MAP_TimerEnable(TIMER0_BASE, TIMER_A); // Start everything

    uint32_t offset=128;
    uint32_t mean1=0;
    uint32_t mean2=0;

    while(1)
    {

/*********primary buffer*********************/

        if(BufferStatus[0u] == FULL)
        {
            // Do something with data in ADC_Out1_PE3
          average1 = 0;
					average2 = 0;
					average3 = 0;
					average4 = 0;

            for(i =0u; i < ADC_SAMPLE_BUF_SIZE; i=i+4)
            {
                Converted_ADC_Out1_PE3[i/4] = ADC_Out1[i]*ADC_Out1[i];
                Converted_ADC_Out1_PE2[i/4] = ADC_Out1[i+1]*ADC_Out1[i+1];
								Converted_ADC_Out1_PE1[i/4] = ADC_Out1[i+2]*ADC_Out1[i+2];
                Converted_ADC_Out1_PE0[i/4] = ADC_Out1[i+3]*ADC_Out1[i+3];
            }
            
						for(i =0u; i < ADC_SAMPLE_BUF_SIZE; i=i+4){
							average1 += Converted_ADC_Out1_PE3[i/4]; // sum of squares of 256 readings of mic1
							average2 += Converted_ADC_Out1_PE2[i/4]; // sum of squares of 256 readings of mic2
							average3 += Converted_ADC_Out1_PE1[i/4]; // sum of squares of 256 readings of mic3
							average4 += Converted_ADC_Out1_PE0[i/4]; // sum of squares of 256 readings of mic4
						}
						average1 = sqrt(average1/256); 
						average2= sqrt(average2/256); 
						average3 = sqrt(average3/256); 
						average4 = sqrt(average4/256); 
            
						rms1 = (float32_t)(average1 * 3.3) / 4095;// rms value of 256 readings of mic1
						rms2 = (float32_t)(average2 * 3.3) / 4095;// rms value of 256 readings of mic2
						rms3 = (float32_t)(average3 * 3.3) / 4095;// rms value of 256 readings of mic3
						rms4 = (float32_t)(average4 * 3.3) / 4095;// rms value of 256 readings of mic4
						
						/******servo integration********/
//						if(rms1 > 1.0){
//							if(rms2>rms3){
//								
//							//move servo towards mic2
//								
//							}
//							if(rms3>rms2){
//								if(rms1 <1.3){
//							//move servo towards mic3
//									while (Adjust >275){
//								Adjust = Adjust - 237;
//									PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, Adjust);
//									}
//									break;
//								}
//							}
//						}
//						else if(rms2 > 1.0){
//							if(rms1>rms3){
//								
//							//move servo towards mic1
//							}
//							if(rms3>rms1){
//								
//							//move servo towards mic3
//								}
//						}
//						else if(rms3 > 1.0){
//							if(rms1>rms2){
//								
//							//move servo towards mic1
//							}
//							if(rms2>rms1){
//								
//							//move servo towards mic2
//								}
//						}
//						

            BufferStatus[0u] = EMPTY;
            // Enable for another uDMA block transfer
            uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out1, ADC_SAMPLE_BUF_SIZE);
            uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT); // Enables DMA channel so it can perform transfers
            samples_taken += ADC_SAMPLE_BUF_SIZE;
        }
				
				
				/*********alternate buffer*********************/
        if(BufferStatus[1u] == FULL)
        {
            // Do something with data in ADC_Out2_PE3
            average2 = 0u;

            for(i =0u; i < ADC_SAMPLE_BUF_SIZE; i=i+2)
            {
                Converted_ADC_Out2_PE3[i/2] = (uint32_t)3.3*ADC_Out2[i]/4095;
                Converted_ADC_Out2_PE2[i/2] = (uint32_t)3.3*ADC_Out2[i+1]/4095;
            }
            
						for(i =0u; i < ADC_SAMPLE_BUF_SIZE; i=i+2){
													average2 += Converted_ADC_Out2_PE2[i/2];
						}
						average2 = average2/512;
            


            BufferStatus[1u] = EMPTY;
            // Enable for another uDMA block transfer
            uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out2, ADC_SAMPLE_BUF_SIZE);
            uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);
            samples_taken += ADC_SAMPLE_BUF_SIZE;
           ///purge first loop issues///
            if(firstLoop){
                average1=0u;
                average2=0u;
                samples_taken=0u;
                firstLoop=0;
            }
            
        }
    }
}

void init_TIMER()
{
    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    // Set sample frequency to 50KHz 
    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, MAP_SysCtlClockGet()/50000 -1);   //TODO: Timer Load Value is set here
    MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
    MAP_TimerControlStall(TIMER0_BASE, TIMER_A, true); //Assist in debug by stalling timer at breakpoints
}

void init_ADC01()
{
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1| GPIO_PIN_0); // 0, 1, 2, 3//Configure PE3
    SysCtlDelay(80u);

    // Use ADC0 sequence 0 to sample channel 0 and 1 once for each timer period
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_HALF, 1);
		ADCClockConfigSet(ADC1_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_HALF, 1);
    SysCtlDelay(10); // Time for the clock configuration to set
		//SysCtlADCSpeedSet(SYSCTL_SET0_ADCSPEED_500KSPS);

    IntDisable(INT_ADC0SS0);
    ADCIntDisable(ADC0_BASE, 0u);
		//ADCHardwareOversampleConfigure(ADC0_BASE, 8);
    ADCSequenceDisable(ADC0_BASE,0u);
    // With sequence disabled, it is now safe to load the new configuration parameters

    ADCSequenceConfigure(ADC0_BASE, 0u, ADC_TRIGGER_TIMER, 0u);
		ADCSequenceStepConfigure(ADC0_BASE, 0u, 0u, ADC_CTL_CH0);
		ADCSequenceStepConfigure(ADC0_BASE, 0u, 1u, ADC_CTL_CH1);
		ADCSequenceStepConfigure(ADC0_BASE, 0u, 2u, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE, 0u, 3u, ADC_CTL_CH3| ADC_CTL_END | ADC_CTL_IE);
		ADCSequenceEnable(ADC0_BASE,0u); //Once configuration is set, re-enable the sequencer ADC0 SS0
		ADCIntClear(ADC0_BASE,0u);
		ADCSequenceDMAEnable(ADC0_BASE,0u);
		IntEnable(INT_ADC0SS0);

}

void init_DMA(void)
{

    uDMAEnable(); // Enables uDMA
    uDMAControlBaseSet(ucControlTable);


    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0, UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST);
    // Only allow burst transfers

    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);


    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out1, ADC_SAMPLE_BUF_SIZE);
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out2, ADC_SAMPLE_BUF_SIZE);


    uDMAChannelEnable(UDMA_CHANNEL_ADC0); // Enables DMA channel so it can perform transfers

}
