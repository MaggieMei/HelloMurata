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
#include "SNIC_UartCommandManager.h"
#include "SNIC_Core.h"

C_SNIC_UartCommandManager::~C_SNIC_UartCommandManager()
{
}

void C_SNIC_UartCommandManager::setCommandID( unsigned char cmd_id )
{
    mCommandID = cmd_id;
}

unsigned char C_SNIC_UartCommandManager::getCommandID()
{
    return mCommandID;
}

void C_SNIC_UartCommandManager::setCommandSID( unsigned char cmd_sid )
{
    mCommandSID = cmd_sid;
}

unsigned char C_SNIC_UartCommandManager::getCommandSID()
{
    return mCommandSID;
}

void C_SNIC_UartCommandManager::setCommandStatus( unsigned char status )
{
    mCommandStatus = status;
}

unsigned char C_SNIC_UartCommandManager::getCommandStatus()
{
    return mCommandStatus;
}

void C_SNIC_UartCommandManager::setResponseBuf( unsigned char *buf_p )
{
    mResponseBuf_p = buf_p;
}

unsigned char *C_SNIC_UartCommandManager::getResponseBuf()
{
    return mResponseBuf_p;
}

void C_SNIC_UartCommandManager::setScanResultHandler( void (*handler_p)(tagSCAN_RESULT_T *scan_result) )
{
    mScanResultHandler_p = handler_p;
}


int C_SNIC_UartCommandManager::wait()
{
    int ret = 0;

    // Get thread ID
    mCommandThreadID = osThreadGetId();
    
    // Signal flags that are reported as event are automatically cleared.
    osEvent event_ret = osSignalWait( UART_COMMAND_SIGNAL, UART_COMMAND_WAIT_TIMEOUT);
    if( event_ret.status != osEventSignal )
    {
        ret = -1;
    }

    return ret;
}

int C_SNIC_UartCommandManager::signal()
{
    // set signal
    return osSignalSet(mCommandThreadID, UART_COMMAND_SIGNAL);
}

bool C_SNIC_UartCommandManager::isWaitingCommand( unsigned int command_id, unsigned char *payload_p )
{
    bool ret = false;

    //printf("command_id = 0x%x\n", command_id);
    if( (command_id == getCommandID())
        && (payload_p[0] == getCommandSID()) )
    {
        ret = true;
    }

    //printf("ret = %x\n", ret);
    return ret;
}

void C_SNIC_UartCommandManager::scanResultIndicate( unsigned char *payload_p, int payload_len )
{
    if( (payload_p == NULL) || (mScanResultHandler_p == NULL) )
    {
        return;
    }
    
    tagSCAN_RESULT_T scan_result;
    int ap_count  = payload_p[2];
    
    if( ap_count == 0 )
    {
        mScanResultHandler_p( NULL );
    }
    
    unsigned char *ap_info_p = &payload_p[3];
    int ap_info_idx = 0;

    for( int i = 0; i < ap_count; i++ )
    {
        scan_result.channel = ap_info_p[ap_info_idx];
        ap_info_idx++;
        scan_result.rssi    = (signed)ap_info_p[ap_info_idx];
        ap_info_idx++;
        scan_result.security= ap_info_p[ap_info_idx];
        ap_info_idx++;
        memcpy( scan_result.bssid, &ap_info_p[ap_info_idx], BSSID_MAC_LENTH );
        ap_info_idx += BSSID_MAC_LENTH;
        scan_result.network_type= ap_info_p[ap_info_idx];
        ap_info_idx++;
        scan_result.max_rate= ap_info_p[ap_info_idx];
        ap_info_idx++;
        ap_info_idx++;  // reserved
        strcpy( scan_result.ssid, (char *)&ap_info_p[ap_info_idx] );
        ap_info_idx += strlen( (char *)&ap_info_p[ap_info_idx] );
        ap_info_idx++;
        
        // Scanresult callback
        mScanResultHandler_p( &scan_result );
    }
}

void C_SNIC_UartCommandManager::bufferredPacket( unsigned char *payload_p, int payload_len )
{
    if( (payload_p == NULL) || (payload_len == 0) )
    {
        return;
    }
    
    C_SNIC_Core *instance_p = C_SNIC_Core::getInstance();
    
    int             socket_id;
    unsigned short  recv_len;
    
    // Get socket id from payload
    socket_id = payload_p[2];
    
//  DEBUG_PRINT("bufferredPacket socket id:%d\r\n", socket_id);
    // Get Connection information
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = instance_p->getConnectInfo( socket_id );
    if( con_info_p == NULL )
    {
        return;
    }
    
    if( con_info_p->is_connected == false )
    {
        DEBUG_PRINT(" Socket id \"%d\" is not connected\r\n", socket_id);
        return;
    }
    
    // Get receive length from payload
    recv_len= ((payload_p[3]<<8) & 0xFF00) | payload_p[4];
    
    while( con_info_p->is_receive_complete == false )
    {
        Thread::yield();
    }
    
  DEBUG_PRINT("bufferredPacket recv_len:%d\r\n", recv_len);
    int i;
    for(i = 0; i < recv_len; i++ )
    {
        if( con_info_p->recvbuf_p->isFull() )
        {
            DEBUG_PRINT("Receive buffer is full.\r\n");
            break;
        }
        // Add to receive buffer
        con_info_p->recvbuf_p->queue( payload_p[5+i] );
    }
    DEBUG_PRINT("###Receive queue[%d]\r\n", i);
    con_info_p->mutex.lock();
    con_info_p->is_receive_complete = false;
    con_info_p->is_received = true;
    con_info_p->mutex.unlock();
//  Thread::yield();
}

void C_SNIC_UartCommandManager::connectStatusTCPClient( unsigned char *payload_p, int payload_len )
{  
    if( (payload_p == NULL) || (payload_len == 0) )
    {
        return;
    }
    
    C_SNIC_Core *instance_p = C_SNIC_Core::getInstance();
    int          socket_id;
    
    // Get socket id of client from payload
    socket_id = payload_p[3];
    
    int status = payload_p[2];
    //printf("TCP socket status = 0x%x\n", status);
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = instance_p->getConnectInfo( socket_id );
    
    if (status == 0x12)
    {
        DEBUG_PRINT("[connectedTCPClient] socket id:%d\r\n", socket_id);
        printf("Connect to TCP server successfully.\n");
    }
    else if(status == 0xb)
    {
        con_info_p->is_connected  = false;
        //printf("TCP socket : %d closed.\n", socket_id);
        return;
    }
    else
        return;

    if( con_info_p == NULL )
    {
        return;
    }
     if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", socket_id);
        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
    }

    con_info_p->is_connected  = true;
    con_info_p->is_received = false;
}

void C_SNIC_UartCommandManager::connectedTCPClient( unsigned char *payload_p, int payload_len )
{
    if( (payload_p == NULL) || (payload_len == 0) )
    {
        return;
    }
    
    C_SNIC_Core *instance_p = C_SNIC_Core::getInstance();
    int          socket_id;
    
    // Get socket id of client from payload
    socket_id = payload_p[3];

    DEBUG_PRINT("[connectedTCPClient] socket id:%d\r\n", socket_id);
    
    // Get Connection information
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = instance_p->getConnectInfo( socket_id );
    if( con_info_p == NULL )
    {
        return;
    }

    if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", socket_id);
        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
    }
    con_info_p->is_connected  = true;
    con_info_p->is_received   = false;
    con_info_p->is_accept     = true;
    con_info_p->parent_socket = payload_p[2];
    con_info_p->from_ip   = ((payload_p[4] << 24) | (payload_p[5] << 16) | (payload_p[6] << 8) | payload_p[7]);
    con_info_p->from_port = ((payload_p[8] << 8)  | payload_p[9]);
}

void C_SNIC_UartCommandManager::bufferredUDPPacket( unsigned char *payload_p, int payload_len )
{
    if( (payload_p == NULL) || (payload_len == 0) )
    {
        return;
    }
    
    C_SNIC_Core *instance_p = C_SNIC_Core::getInstance();

    // Get Connection information
    C_SNIC_Core::tagUDP_RECVINFO_T *con_info_p = instance_p->getUdpRecvInfo( payload_p[2] );
    if( con_info_p == NULL )
    {
        return;
    }

    if( con_info_p->recvbuf_p == NULL )
    {
//        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", payload_p[2]);
        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
    }
    con_info_p->mutex.lock();
    con_info_p->is_received = true;
    con_info_p->mutex.unlock();
    
    // Set remote IP address and remote port
    con_info_p->from_ip   = ((payload_p[3] << 24) | (payload_p[4] << 16) | (payload_p[5] << 8) | payload_p[6]);
    con_info_p->from_port = ((payload_p[7] << 8)  | payload_p[8]);
    
    unsigned short recv_len;
    // Get receive length from payload
    recv_len= ((payload_p[9]<<8) & 0xFF00) | payload_p[10];
    for( int i = 0; i < recv_len; i++ )
    {
        if( con_info_p->recvbuf_p->isFull() )
        {
            DEBUG_PRINT("Receive buffer is full.\r\n");
            break;
        }
        
        // Add to receive buffer
        con_info_p->recvbuf_p->queue( payload_p[11+i] );
    }
}
