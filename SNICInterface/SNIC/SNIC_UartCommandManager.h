/* Copyright (C) 2014 Murata Manufacturing Co.,Ltd., MIT License
 *  muRata, SWITCH SCIENCE Wi-FI module TypeYD SNIC-UART.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _SNIC_UART_COMMAND_MANAGER_H_
#define _SNIC_UART_COMMAND_MANAGER_H_
#include "MurataObject.h"
#include "mbed.h"
#include "rtos.h"

/** Max length of SSID */
#define SSID_MAX_LENGTH 32
/** Max length of BSSID */
#define BSSID_MAC_LENTH 6
/** Length of Country code */
#define COUNTRYC_CODE_LENTH 2
    
/** Wait signal ID of UART command */
#define UART_COMMAND_SIGNAL        0x00000001
#define TCP_CONNECTION_STATUS_SIGNAL    0x00000004
/** Timeout of UART command wait(ms)*/
#define UART_COMMAND_WAIT_TIMEOUT  10000

/** Scan result structure used by scanresults handler
*/
typedef struct {
    bool          is_complete;
    /** Channel */
    unsigned char channel;
    /** RSSI */
    signed   char rssi;
    /** Security type */
    unsigned char security;
    /** BSSID */
    unsigned char bssid[BSSID_MAC_LENTH];
    /** Network type */
    unsigned char network_type;
    /** Max data rate */
    unsigned char max_rate;
    /** SSID */
    char          ssid[SSID_MAX_LENGTH+1];
}tagSCAN_RESULT_T;

/** Internal class for managing the SNIC UART command.
 */
class C_SNIC_UartCommandManager: public C_MurataObject
{
friend class C_SNIC_Core;
friend class C_SNIC_WifiInterface;
friend class UDPSocket;
friend class TCPSocketConnection;
friend class Socket;

private:
    virtual ~C_SNIC_UartCommandManager();
    
    /** Set Command ID
        @param cmd_id Command ID
    */
    void setCommandID( unsigned char cmd_id );

    /** Get Command ID
        @return Command ID
    */
    unsigned char getCommandID();

    /** Set Command SubID
        @param cmd_sid Command Sub ID
    */
    void setCommandSID( unsigned char cmd_sid );

    /** Get Command SubID
        @return Command Sub ID
    */
    unsigned char getCommandSID();
    
    /** Set Command status
        @param status Command status
    */
    void setCommandStatus( unsigned char status );

    /** Get Command status
        @return Command status
    */
    unsigned char getCommandStatus();

    /** Set Response buffer
        @param buf_p Pointer of response buffer
    */
    void setResponseBuf( unsigned char *buf_p );

    /** Get Response buffer
        @return Pointer of response buffer
    */
    unsigned char *getResponseBuf();

    /** Set scan result callback hander
        @param handler_p Pointer of callback function
    */
    void setScanResultHandler( void (*handler_p)(tagSCAN_RESULT_T *scan_result) );
    
    void bufferredPacket( unsigned char *payload_p, int payload_len );
    
    void bufferredUDPPacket( unsigned char *payload_p, int payload_len );

    void scanResultIndicate( unsigned char *payload_p, int payload_len );
    
    void connectStatusTCPClient( unsigned char *payload_p, int payload_len );
    
    void connectedTCPClient( unsigned char *payload_p, int payload_len );
    
    /** Checks in the command which is waiting from Command ID and Sub ID.
        @param  command_id  Command ID
        @param  payload_p   Command payload
        @return true: Waiting command / false: Not waiting command
    */
    bool isWaitingCommand( unsigned int command_id, unsigned char *payload_p );

    int wait();
    
    int signal();

private:
    /** Command request thread ID */
    osThreadId    mCommandThreadID;
    /** Command ID */
    unsigned char mCommandID;
    /** Command SubID */
    unsigned char mCommandSID;
    /** Status of command response */
    unsigned char mCommandStatus;
    /** ResponseData of command response */
    unsigned char *mResponseBuf_p;
    /** Scan result handler */
    void (*mScanResultHandler_p)(tagSCAN_RESULT_T *scan_result);
};
    
#endif
