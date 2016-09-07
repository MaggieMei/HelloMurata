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
#include "mbed.h"
#include "SNIC_Core.h"
#include "SNIC_UartMsgUtil.h"
#include <string>

/** Wait signal ID of UART recv */
#define UART_DISPATCH_SIGNAL    0x00000002

#define UART_RECVBUF_SIZE       2048
#define UART_THREAD_STACK_SIZE  512
#define UART_FIXED_HEADER_SIZE    3
#define UART_FIXED_SIZE_IN_FRAME  6
#define UART_RECV_QUEUE_TIMEOUT   500

typedef struct
{
    tagMEMPOOL_BLOCK_T  *mem_p;
    unsigned int  size;
}tagUART_RECVBUF_T;

/*
    Define the global buffer using the area for Ethernet.
*/
#if defined(TARGET_LPC1768)
unsigned char  gUART_TEMP_BUF[UART_RECVBUF_SIZE]                __attribute__((section("AHBSRAM1")));
unsigned char  gUART_COMMAND_BUF[UART_REQUEST_PAYLOAD_MAX]      __attribute__((section("AHBSRAM1")));
/** MemoryPool for payload of UART response */
MemoryPool<tagMEMPOOL_BLOCK_T, MEMPOOL_PAYLOAD_NUM>     mMemPoolPayload  __attribute__((section("AHBSRAM0")));
/** MemoryPool for UART receive */
MemoryPool<tagMEMPOOL_BLOCK_T, MEMPOOL_UART_RECV_NUM>   mMemPoolUartRecv __attribute__((section("AHBSRAM0")));
#else
unsigned char  gUART_TEMP_BUF[UART_RECVBUF_SIZE];
unsigned char  gUART_COMMAND_BUF[UART_REQUEST_PAYLOAD_MAX];
/** MemoryPool for payload of UART response */
MemoryPool<tagMEMPOOL_BLOCK_T, MEMPOOL_PAYLOAD_NUM>     mMemPoolPayload;
/** MemoryPool for UART receive */
MemoryPool<tagMEMPOOL_BLOCK_T, MEMPOOL_UART_RECV_NUM>   mMemPoolUartRecv;
#endif
Queue<tagMEMPOOL_BLOCK_T, MEMPOOL_UART_RECV_NUM>        mUartRecvQueue;

tagMEMPOOL_BLOCK_T   *gUART_RCVBUF_p;
int gUART_RECV_COUNT = 0;

C_SNIC_Core *C_SNIC_Core::mInstance_p = NULL;

C_SNIC_Core *C_SNIC_Core::getInstance()
{
    if( mInstance_p == NULL )
    {
        mInstance_p    = new C_SNIC_Core();
    }
    return mInstance_p;
}

C_SNIC_Core::C_SNIC_Core()
{
    int i;
    
    mUartCommand_p = new C_SNIC_UartCommandManager();
    for( i = 0; i < MAX_SOCKET_ID+1; i++ )
    {
        mConnectInfo[i].recvbuf_p    = NULL;
        mConnectInfo[i].is_connected = false;
        mConnectInfo[i].is_receive_complete = true;
        
        mUdpRecvInfo[i].recvbuf_p    = NULL;
        mUdpRecvInfo[i].is_received  = false;
    }
    
    mUartRecvThread_p         = NULL;
    mUartRecvDispatchThread_p = NULL;
}

//C_SNIC_Core::~C_SNIC_Core()
//{
//}

int C_SNIC_Core::resetModule( PinName reset )
{
    DigitalOut reset_pin( reset );

    reset_pin = 0;
    wait(0.3);
    reset_pin = 1;
    wait(0.3);
    
    return 0;
}

int C_SNIC_Core::initUart(PinName tx, PinName rx, int baud)
{
    mUartRequestSeq = 0;

    mUart_p = new RawSerial( tx, rx );
    mUart_p->baud( baud );
    mUart_p->format(8, SerialBase::None, 1);
     
    // Initialize uart
    gUART_RCVBUF_p = NULL;

    mUart_p->attach( C_SNIC_Core::uartRecvCallback );
    // Create UART recv dispatch thread
    mUartRecvDispatchThread_p = new Thread( C_SNIC_Core::uartRecvDispatchThread, NULL, osPriorityNormal, UART_THREAD_STACK_SIZE);
    if( mUartRecvDispatchThread_p == NULL )
    {
        DEBUG_PRINT("[C_SNIC_Core::initUart] thread create failed\r\n");
        return -1;
    }

    return 0;
}
unsigned int C_SNIC_Core::preparationSendCommand( unsigned char cmd_id, unsigned char cmd_sid
                                            , unsigned char *req_buf_p,    unsigned int req_buf_len
                                            , unsigned char *response_buf_p, unsigned char *command_p )
{
    unsigned int command_len = 0;
    
    // Make all command request
    command_len = C_SNIC_UartMsgUtil::makeRequest( cmd_id, req_buf_p, req_buf_len, command_p );

    // Set data for response
    mUartCommand_p->setCommandID( cmd_id );
    mUartCommand_p->setCommandSID( cmd_sid | 0x80 );
    mUartCommand_p->setResponseBuf( response_buf_p );
    
    return command_len;
}

int C_SNIC_Core::sendUart( unsigned int len, unsigned char *data )
{
    int ret = 0;
    mUartMutex.lock();
    for( int i = 0; i < len; i++ )
    {
        // Write to UART
        ret = mUart_p->putc( data[i] );
        if( ret == -1 )
        {
            break;
        }
    }
    mUartMutex.unlock();

    return ret;
}

tagMEMPOOL_BLOCK_T *C_SNIC_Core::allocCmdBuf()
{
    // Get buffer from MemoryPool
    return mMemPoolPayload.alloc();
}

void C_SNIC_Core::freeCmdBuf( tagMEMPOOL_BLOCK_T *buf_p )
{
    mMemPoolPayload.free( buf_p );
}

tagMEMPOOL_BLOCK_T *C_SNIC_Core::allocUartRcvBuf()
{
    // Get buffer from MemoryPool
    return mMemPoolUartRecv.alloc();
}

void C_SNIC_Core::freeUartRecvBuf( tagMEMPOOL_BLOCK_T *buf_p )
{
    mMemPoolUartRecv.free( buf_p );
}

C_SNIC_Core::tagCONNECT_INFO_T  *C_SNIC_Core::getConnectInfo( int socket_id )
{
    if( (socket_id < 0) || (socket_id > MAX_SOCKET_ID) )
    {
        return NULL;
    }
    return &mConnectInfo[socket_id];
}

C_SNIC_Core::tagUDP_RECVINFO_T  *C_SNIC_Core::getUdpRecvInfo( int socket_id )
{
    if( (socket_id < 0) || (socket_id > MAX_SOCKET_ID) )
    {
        return NULL;
    }
    return &mUdpRecvInfo[socket_id];
}

C_SNIC_UartCommandManager *C_SNIC_Core::getUartCommand()
{
    return mUartCommand_p;
}

unsigned char *C_SNIC_Core::getCommandBuf()
{
    return gUART_COMMAND_BUF;
}

void C_SNIC_Core::lockAPI( void )
{
    mAPIMutex.lock();
}

void C_SNIC_Core::unlockAPI( void )
{
    mAPIMutex.unlock();
}

void C_SNIC_Core::uartRecvCallback( void )
{
    C_SNIC_Core *instance_p = C_SNIC_Core::getInstance();
    if( instance_p != NULL )
    {
        int  recvdata = 0;

        // Check received data from UART.
        while( instance_p->mUart_p->readable() )
        {
            // Receive data from UART.
            recvdata = instance_p->mUart_p->getc();

            // Check UART receiving buffer
            if( gUART_RCVBUF_p != NULL )
            {
                gUART_RCVBUF_p->buf[ gUART_RCVBUF_p->size ] = (unsigned char)recvdata;
                gUART_RCVBUF_p->size++;
                
                if( gUART_RCVBUF_p->size == UART_FIXED_HEADER_SIZE )
                {
                    // get demand size
                    unsigned short  payload_len = ( ( (gUART_RCVBUF_p->buf[1] & ~0x80) & 0xff) | ( ( (gUART_RCVBUF_p->buf[2] & ~0xC0) << 7) & 0xff80) );
                    gUART_RCVBUF_p->demand_size = payload_len + UART_FIXED_SIZE_IN_FRAME;
                    if( gUART_RCVBUF_p->demand_size > MEMPOOL_BLOCK_SIZE )
                    {
                        gUART_RCVBUF_p->demand_size = MEMPOOL_BLOCK_SIZE;
                    }
                }

                if( gUART_RCVBUF_p->demand_size > 0 )
                {
                    // Check size of received data.
                    if( gUART_RCVBUF_p->size >= gUART_RCVBUF_p->demand_size )
                    {
                        // Add queue 
                        mUartRecvQueue.put( gUART_RCVBUF_p );
                        
                        gUART_RCVBUF_p = NULL;
                    
                        if( gUART_RECV_COUNT >= MEMPOOL_UART_RECV_NUM )
                        {
                            instance_p->mUart_p->attach( NULL );
                        }
                        // set signal for dispatch thread
                        instance_p->mUartRecvDispatchThread_p->signal_set( UART_DISPATCH_SIGNAL );
                        break;
                    }
                }
            }
            else
            {
                // Check  received data is SOM.
                if( recvdata == UART_CMD_SOM )
                {
                    gUART_RCVBUF_p = instance_p->allocUartRcvBuf();
                    gUART_RECV_COUNT++;
                    gUART_RCVBUF_p->size = 0;
                    gUART_RCVBUF_p->demand_size = 0;

                    // get buffer for Uart receive
                    gUART_RCVBUF_p->buf[ 0 ] = (unsigned char)recvdata;
                    
                    gUART_RCVBUF_p->size++;
                }
            }
        }
    }
}

void C_SNIC_Core::uartRecvDispatchThread (void const *args_p)
{
    C_SNIC_Core               *instance_p   = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = instance_p->getUartCommand();
    
    tagMEMPOOL_BLOCK_T  *uartRecvBuf_p;
    osEvent      evt;
    
    for(;;)
    {
        // wait
        Thread::signal_wait( UART_DISPATCH_SIGNAL );

        // Get scanresults from queue
        evt = mUartRecvQueue.get(UART_RECV_QUEUE_TIMEOUT);
        if (evt.status == osEventMessage)
        {
            do
            {
                uartRecvBuf_p = (tagMEMPOOL_BLOCK_T *)evt.value.p;

#if 0   /* for Debug */
                {
                    int i;
                    for(i=0;i<uartRecvBuf_p->size;i++)
                    {
                        DEBUG_PRINT("%02x", uartRecvBuf_p->buf[i]);
                    }
                    DEBUG_PRINT("\r\n");
                }
#endif
                unsigned char command_id;
                // Get payload from received data from UART.
                int payload_len = C_SNIC_UartMsgUtil::getResponsePayload( uartRecvBuf_p->size, uartRecvBuf_p->buf
                                                        , &command_id, gUART_TEMP_BUF );
                //int subCmd = gUART_TEMP_BUF[0]&0x7f;
//                printf("command_id = 0x%x\n", command_id);
//                printf("gUART_TEMP_BUF[0] = 0x%x\n", subCmd);
                // Check receive a TCP packet
                if( (command_id == UART_CMD_ID_SNIC) && (gUART_TEMP_BUF[0] == UART_CMD_SID_SNIC_CONNECTION_RECV_IND) )
                {
                    // Packet buffering
                    uartCmdMgr_p->bufferredPacket( gUART_TEMP_BUF, payload_len );
                }
                //// Check connection status indication for TCP client
//                else if( (command_id == UART_CMD_ID_SNIC) && (gUART_TEMP_BUF[0] == UART_CMD_SID_SNIC_TCP_CONNECTION_STATUS_IND) )
//                {
//                    // Connected status Indication from TCP client
//                    uartCmdMgr_p->connectStatusTCPClient( gUART_TEMP_BUF, payload_len );
//                }
                // Check connected from TCP client
                else if( (command_id == UART_CMD_ID_SNIC) && (gUART_TEMP_BUF[0] == UART_CMD_SID_SNIC_TCP_CLIENT_SOCKET_IND) )
                {
                    // Connected from TCP client
                    uartCmdMgr_p->connectedTCPClient( gUART_TEMP_BUF, payload_len );
                }
                // Check receive UDP packet
                else if( (command_id == UART_CMD_ID_SNIC) && (gUART_TEMP_BUF[0] == UART_CMD_SID_SNIC_UDP_RECV_IND) )
                {
                    // UDP packet buffering
                    uartCmdMgr_p->bufferredUDPPacket( gUART_TEMP_BUF, payload_len );
                }
                // Check scan results indication 
                else if( (command_id == UART_CMD_ID_WIFI) && (gUART_TEMP_BUF[0] == UART_CMD_SID_WIFI_SCAN_RESULT_IND) )
                {
                    // Scan result indicate
                    uartCmdMgr_p->scanResultIndicate( gUART_TEMP_BUF, payload_len );
                }
                // Checks in the command which is waiting.
                else if( uartCmdMgr_p->isWaitingCommand(command_id, gUART_TEMP_BUF) )
                {
                    //printf("333333333333333333333command id = 0x%x\n", command_id);
                    // Get buffer for payload data
                    unsigned char *payload_buf_p = uartCmdMgr_p->getResponseBuf();
                    if( payload_buf_p != NULL )
                    {
                        memcpy( payload_buf_p, gUART_TEMP_BUF, payload_len );
                        uartCmdMgr_p->setResponseBuf( NULL );
                    }
                    // Set status
                    uartCmdMgr_p->setCommandStatus( gUART_TEMP_BUF[2] );
                    // Set signal for command response wait.
                    uartCmdMgr_p->signal();
                }
                else
                {
//                    DEBUG_PRINT(" The received data is not expected.\r\n");
                }
                
                // 
                instance_p->freeUartRecvBuf( uartRecvBuf_p );
                gUART_RECV_COUNT--;
                if( gUART_RECV_COUNT == (MEMPOOL_UART_RECV_NUM-1) )
                {
                    instance_p->mUart_p->attach( C_SNIC_Core::uartRecvCallback );   //debug
                }
                
                evt = mUartRecvQueue.get(500);
            } while( evt.status == osEventMessage );
        }
    }
}
