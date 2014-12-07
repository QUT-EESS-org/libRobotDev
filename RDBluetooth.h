/*
 * libRobotDev
 * RDBluetooth.h
 * Purpose: Abstracts all Bluetooth functions
 * Created: 29/07/2014
 * Author(s): Lachlan Cesca, Samuel Cunningham-Nelson, Arda Yilmaz,
 *            Jeremy Pearson
 * Status: UNTESTED
 */ 

#include <util/delay.h>
#include "RDUART.h"

#ifndef RDBLUETOOTH_H_
#define RDBLUETOOTH_H_

#define	KEYPIN	(1 << PE5)
#define	BTPWR	(1 << PE4)
#define BTDDR DDRE
#define BTPORT PORTE

static volatile char bluetoothBaud = 0;

/*
 * Sets module name.
 *
 * @param char *name
 *      Name of the module.
 *
 * @return void
 */
static inline void RDBluetoothSetName(char *name) {

    char *preamble = "AT+NAME";		// Preamble
    RDUARTPutsNoNull(preamble);   // Transmit
    RDUARTPutsNoNull(name);	
    _delay_ms(750);
}

/*
 * Sets module pin.
 *
 * @param char *pin
 *      Module pairing pin.
 *
 * @return void
 */
static inline void RDBluetoothSetPin(char *pin) {

    char* preamble = "AT+PIN";  // Preamble
    RDUARTPutsNoNull(preamble); // Transmit
    RDUARTPutsNoNull(pin);
    _delay_ms(750);
}

/*
 * Sets module UART baud rate. For supported baud rates, see RDBluetoothConfig.
 *
 * @param char baud
 *      See RDBluetoothConfig.
 *
 * @return void
 */
static inline void RDBluetoothSetBR(char baud) {

    char* preamble = "AT+BAUD";		// Preamble.
    RDUARTPutsNoNull(preamble);	// Transmit
    RDUARTPutc(baud);
    _delay_ms(750);
}

/*
 * Pings module.
 *
 * @param void
 *
 * @return void
 */
static inline void RDBluetoothSendAT(void) {
    
    char *preamble = "AT";          // Preamble
    RDUARTPutsNoNull(preamble);	// Transmit
    _delay_ms(200);
}

/*
 * Sends one byte of data.
 *
 * @param char byte
 *      Data byte to send.
 *
 * @return void
 */
void RDBluetoothSendByte(char byte){
    
    RDUARTPutc((uint8_t) byte);
}

/* UNTESTED
 * Sends packet of given length.
 *
 * @param char *buffer
 *      Data buffer to send.
 *
 * @param uint16_t *length
 *      Length of buffer (including null terminator for strings).
 *
 * @return void
 */
void RDBluetoothSendBuffer(char* buffer, uint16_t length) {
    
    uint16_t i = 0;
    
    for (i = 0; i < length; ++i) RDBluetoothSendByte(buffer[0]);
}

/*
 * Receives one byte of data.
 *
 * @param void
 *
 * @return char
 *      Data byte
 */
char RDBluetoothReceiveByte(void){
    return (char) RDUARTGetc();
}

/*
 * Check for "OK" response from module.
 *
 * @param void
 *
 * @return uint8_t
 *      1 if valid response,
 *      0 if invalid response.
 */
static inline uint8_t RDBluetoothCheckOk(void) {
    uint8_t i = 5;
    uint8_t okCheckStr = 0;
    while (!RDUARTAvailable() && i) {
        
        _delay_ms(100);
        --i;
    }
    if (i) {
        okCheckStr = RDBluetoothReceiveByte();
    } else {
        return 0;
    }
    return (okCheckStr == 'O') ? 1 : 0;
}

/* UNTESTED
 * Adjusts appropriate control pins to put module into configuration mode.
 *
 * @param void
 *
 * @return void
 */
static inline void RDBluetoothEnterConfigMode(void) {
    
    // Enable control pins
    BTDDR |= BTPWR | KEYPIN;
    
    // Place Bluetooth in config mode
    BTPORT |= BTPWR;			// Turn off module
    BTPORT |= KEYPIN;		// Pull KEY high
    
    _delay_ms(100);
    BTPORT &= ~BTPWR;		// Turn on module
}

/* UNTESTED
 * Adjusts appropriate control pins to restart module.
 *
 * @param void
 *
 * @return void
 */
static inline void RDBluetoothRestart(void) {
    BTPORT &= ~KEYPIN;		// Pull KEY low
    BTPORT |= BTPWR;			// Turn off module
    _delay_ms(100);
    BTPORT &= ~BTPWR;		// Turn on module
}

/*
 * Returns UL baud rate corresponding to designator.
 *
 * @param char
 *      Baud rate designator (see RDBluetoothSetBR header).
 *
 * @return unsigned long
 *      Corresponding UL baud rate. E.g. char "4" -> 9600UL
 */
static unsigned long RDBluetoothReturnBaudUL(char baud) {
    // Convert character to index value (0 : 12)
    uint8_t baudVal = (baud < 'A') ? baud - '0' : (baud - 'A') + 10;
    /*
     * 1 - 6  : 1200 * 2^(baudVal - 1)
     * 7 - 11 : 1200 * 2^(baudVal - 1) - 19200 * 2^(baudVal - 7)
     * 12     : 1382400
     */
    return (baudVal >= 12) ? 1382400 : 1200 * (1 << (baudVal - 1)) - (baudVal >= 7) * ( 19200 * (1 << (baudVal - 7)) );
}

/*
 * Queries module for set baud rate and saves it to bluetoothBaud.
 *
 * @param void
 *
 * @return void
 */
static inline void RDBluetoothGetBaud(void) {
    char* baudSweep = "123456789ABC";
    uint8_t i = 0;
    //RDBluetoothEnterConfigMode();
    for (i = 0; baudSweep[i] != '\0'; ++i) {
        RDUARTInit(RDBluetoothReturnBaudUL(baudSweep[i]));		// Initialise UART
        RDBluetoothSendAT();				// Send ping
        bluetoothBaud = baudSweep[i];       // Store current test baud rate
        if (RDBluetoothCheckOk()) {
            break;    // Check response
        }
    }
    //RDBluetoothRestart();
}

/*
 * Initialises UART to modules current baud rate.
 *
 * @param void
 *
 * @return unsigned long
 *      Baud rate detected from module.
 */
unsigned long RDBluetoothInit(void) {
    // Get current baud rate
    RDBluetoothGetBaud();
    // Initialize UART with last baud rate
    unsigned long baud = RDBluetoothReturnBaudUL(bluetoothBaud);
    RDUARTInit(baud);
    
    return baud;
}

/*
 * Renames device, sets pin, and changes current baud rate. UART is reconfigured
 * automatically to the new baud rate. Module is automatically placed and 
 * brought out of configuration mode.
 * 
 * @param char *name
 *      Name of the module.
 *
 * @param char *pin
 *      Module pairing pin.
 *
 * @param char baud
 *      Corresponding baud rate designator:
 *      char baud	Baud Rate
 *      "1"			1200
 *      "2"			2400
 *      "3"			4800
 *      "4"			9600
 *      "5"			19200
 *      "6"		    38400
 *      "7"			57600
 *      "8"			115200
 *      "9"			230400
 *      "A"			460800
 *      "B"			921600
 *      "C"			1382400
 *
 * @return void
 */
void RDBluetoothConfig(char *name, char* pin, char baud) {
    
    RDBluetoothEnterConfigMode();
    
    _delay_ms(750);    
    // Configure
    RDBluetoothSetName(name);	// Set name
    RDBluetoothSetPin(pin);		// Set pin
    RDBluetoothSetBR(baud);		// Set baud rate
    
    RDBluetoothRestart();
    
    bluetoothBaud = baud;		// Update baud rate
    RDUARTInit( RDBluetoothReturnBaudUL(bluetoothBaud) );	// Reinitialise UART with new baud
}
#endif // RDBLUETOOTH_H_