/** \file
 *  \brief CryptoMemory Hardware Interface Functions - TWI-like bus.
 *
 *  The first version of the CryptoMemory Synchronous Interface is not
 *  a true TWI interface.  Reading the CryptoMemory is not done with
 *  the read bit set in the address, following a restart.
 *  Instead the read is done immediately after setting the address with
 *  a write instruction.
 *  
 *  Use these functions to control the low level hardware of your 
 *  microcontroller.  The pin and port definitions are in the file
 *  ll_port.h, and are defined in two structures:
 *  - ::TWI_PORT defines the pin input, port control and port output registers
 *  - ::TWI_PINS bitwise defines the pins in the port.
 *  
 *  The current delay function uses the avr-libc delay function, which 
 *  requires you to define the F_CPU variable.  This is easiest to do in
 *  the makefile.
 *  
 *  If you are using an AVR processor, you only need to redefine F_CPU and
 *  the structures mentioned above.
 *
 *  If you are using another microprocessor you will probably need to 
 *  change the functions listed here:
 *  - cm_Delay(), which generates an approximate 1 us delay, 
 *  - cm_Clockhigh() to set the clock pin high (float the output if
 *     possible so that the rise time is determined by the pull-up
 *     resistor and not driven by the output driver.)
 *  - cm_Clocklow()  to set the clock pin low.
 *  - cm_Datahigh()  to set the data  pin high.
 *  - cm_Datalow()   to set the data  pin low.
 */

#include <util/delay.h>
#include <stdio.h>
#include "lib_Crypto.h"
#include "ll_port.h"

TWI_PORT *pTWI = (TWI_PORT *)0x33 ;///< TWI port is Port C

/** \brief Delay loop for the bit-banged TWI-like interface
 *
 *  We use this loop to generate an approximate 1 us delay.
 *  A hardware timer might be a better way to implement this, but
 *  to save resources, this example uses a software timer provided
 *  by avr-libc.
 *  The avr-libc _delay_us(double __us) function will link in the 
 *  floating point library if you pass a variable to this function.
 *  That is why we pass a fixed value (1) and use a loop here.
 */
void ll_Delay(uchar ucDelay)
{   
    for ( ; ucDelay != 0 ; ucDelay-- )
    {
        _delay_us(1);
    }
}

/** \brief Sets the clock pin to a high level by floating the port output
 *
 *  See file ll_port.h for port structure definitions.
 *  Uses the internal port pull-up to provide the TWI clock pull-up.
 */
void ll_Clockhigh(void)
{
    ll_Delay(1);
    pTWI->dir_reg.clock_pin = 0 ; 
    pTWI->port_reg.clock_pin = 1 ;
    ll_Delay(1);
}
                                                                                
/** \brief Sets the clock pin to a low level. 
 *
 *  See file ll_port.h for port structure definitions.
 */
void ll_Clocklow(void)
{
    ll_Delay(1);
    pTWI->port_reg.clock_pin = 0 ;
    ll_Delay(2);
    pTWI->dir_reg.clock_pin = 1 ;
}

/** \brief Cycles the clock pin low then high. */
void ll_ClockCycle(void)
{
    ll_Clocklow();
    ll_Clockhigh();
}
                                                                                
/// \brief Calls ll_ClockCycle() ucCount times
void ll_ClockCycles(uchar ucCount)
{
    for ( ; ucCount > 0 ; ucCount-- )
        ll_ClockCycle() ;
}

/** \brief Sets the data pin to a high level by floating the port output
 *
 *  See file ll_port.h for port structure definitions.
 *  Uses the internal port pull-up to provide the TWI data pull-up.
 */
void ll_Datahigh(void)
{
    ll_Delay(1);
    pTWI->dir_reg.data_pin = 0 ;
    pTWI->port_reg.data_pin = 1 ;
    ll_Delay(2);
}
                                                                                
/** \brief Sets the data pin to a low level. 
 *
 *  See file ll_port.h for port structure definitions.
 */
void ll_Datalow(void)
{
    ll_Delay(1);
    pTWI->port_reg.data_pin = 0 ;
    ll_Delay(2);
    pTWI->dir_reg.data_pin = 1 ;
}

/** \brief Reads and returns the data pin value.  Leaves the pin high-impedance.
*/
uchar ll_Data(void)
{
    ll_Delay(1);
    pTWI->dir_reg.data_pin  = 0 ;
    pTWI->port_reg.data_pin = 1 ;
    ll_Delay(4);
    return pTWI->pin_reg.data_pin ;
}

/** \brief Sends a start sequence */
void ll_Start(void)
{
    ll_Clocklow();
    ll_Datahigh();
    ll_Delay(4);
    ll_Clockhigh();
    ll_Delay(4);
    ll_Datalow();
    ll_Delay(4);
    ll_Clocklow();
    ll_Delay(4);
}

/** \brief  Sends a stop sequence */
void ll_Stop(void)
{
    ll_Clocklow();
    ll_Datalow();
    ll_Clockhigh();
    ll_Delay(8);
    ll_Datahigh();
    ll_Delay(4);
}

/** \brief Sends an ACK or NAK to the device (after a read). 
 * 
 *  \param ucAck - if ::TRUE means an ACK will be sent, otherwise
 *  NACK sent.
 */
void ll_AckNak(uchar ucAck)
{
    ll_Clocklow();
    if (ucAck) 
        ll_Datalow() ; //Low data line indicates an ACK
    else    
        ll_Datahigh(); // High data line indicates an NACK
    ll_Clockhigh();
    ll_Clocklow();
}

/** \brief Power On chip sequencing.  Clocks the chip ::LL_PWRON_CLKS times. 
*/
void ll_PowerOn(void)
{                                                                              
    // Sequence for powering on secure memory according to ATMEL spec
    ll_Datahigh() ;    // Data high during reset
    ll_Clocklow() ;    // Clock should start LOW
    ll_ClockCycles(LL_PWRON_CLKS);            
    // Give chip some clocks cycles to get started
    // Chip should now be in sync mode and ready to operate
}
                                                                                

/** \brief  Write a byte on the TWI-like bus.
*
* \return 0 if write success, else 'n' (the number of attempts) 
*  to get ACK that failed (maximum is ::LL_ACK_TRIES).
*/
uchar ll_Write(uchar ucData)
{
    uchar i;

    for(i=0; i<8; i++) {                 // Send 8 bits of data
        ll_Clocklow();
        if (ucData&0x80) 
            ll_Datahigh();
        else   
            ll_Datalow() ;
        ll_Clockhigh();
        ucData = ucData<<1;
    }
    ll_Clocklow();

    // wait for the ack
    ll_Datahigh(); // Set data line to be an input
    ll_Delay(8);
    ll_Clockhigh();
    for ( i = 0 ; i < LL_ACK_TRIES ; i++ )
    {   if ( ll_Data() == 0 )
        {   i = 0 ;
            break ;
        }
    }
    ll_Clocklow();

    return i;
}

/** \brief Read a byte from device, MSB first, no ACK written yet*/
uchar ll_Read(void)
{
    uchar i;
    uchar rByte = 0;
    ll_Datahigh();
    for(i=0x80; i; i=i>>1)
    {
        ll_ClockCycle();
        if (ll_Data()) 
            rByte |= i;
        ll_Clocklow();
    }
    return rByte;
}

/** \brief Send a command (usually 4 bytes) over the bus.
 * 
 * \param pucInsBuf is pointer to a buffer containing numBytes to send
 * \param numBytes is the number of bytes to send
 *
 * \return ::SUCCESS or ::FAIL_CMDSEND (if ll_Write() fails)
 */
RETURN_CODE ll_SendCommand(puchar pucInsBuf, uchar ucLen)
{
    uchar i ;

    i = LL_START_TRIES;
    while (i) {
        ll_Start();
        if (ll_Write(pucInsBuf[0]) == 0) break;
        if (--i == 0) return FAIL_CMDSTART;
    }

    for(i = 1; i< ucLen; i++) {
        if (ll_Write(pucInsBuf[i]) != 0) return FAIL_CMDSEND;
    }
    return SUCCESS;
}

/** \brief Receive ucLen bytes over the bus.
 * 
 * \param pucRecBuf is a pointer to a buffer to contain ucLen bytes received.
 * \param ucLen is the number of bytes to receive
 *
 * \return ::SUCCESS 
 */
RETURN_CODE ll_ReceiveData(puchar pucRecBuf, uchar ucLen)
{
    if ( ucLen > 0 )
    {
        while ( --ucLen )
        {
            *pucRecBuf++ = ll_Read();
            ll_AckNak(TRUE);
        }
        *pucRecBuf = ll_Read() ;
        ll_AckNak(FALSE);
    }
    ll_Stop();
    return SUCCESS;
}

/** \brief Send data by calling ll_Write() and then sends Stop with ll_Stop().
 * 
 * \param pucSendBuf is a pointer to a buffer containing ucLen bytes to send
 * \param ucLen is the number of bytes to send
 *
 * \return ::SUCCESS or ::FAIL_WRDATA (if ll_Write() fails)
 */
RETURN_CODE ll_SendData(puchar pucSendBuf, uchar ucLen)
{
    int i;
    for(i = 0; i< ucLen; i++) {
        if (ll_Write(pucSendBuf[i])!=0) return FAIL_WRDATA;
    }
    // Even when ucLen = 0 (no bytes to send) always send a STOP
    ll_Stop();
    return SUCCESS;
}

/** \brief Sends loop * (Start + 15 clocks + Stop) over the bus.
 */  
void ll_WaitClock(uchar ucLoop)
{
    uchar i, j;
    pTWI->port_reg.data_pin = 0; 
    for(j=0; j<ucLoop; j++) {
        ll_Start();
        for(i = 0; i<15; i++) ll_ClockCycle();
        ll_Stop();
    }
}



