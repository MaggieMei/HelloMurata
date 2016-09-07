/* Copyright (C) 2014 Murata Manufacturing Co.,Ltd., MIT License
 *  muRata, SWITCH SCIENCE Wi-FI module TypeYD-SNIC UART.
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
#include "SNIC-to-AT.h"
#include "SNIC_UartMsgUtil.h"

#define UART_CONNECT_BUF_SIZE   512
unsigned char gCONNECT_BUF[UART_CONNECT_BUF_SIZE];
char gSOCKET_SEND_BUF[2048] __attribute__((section("AHBSRAM1")));
static char ip_addr[17] = "\0";
static char mac_addr[25] = "\0";

C_SNIC_WifiInterface::C_SNIC_WifiInterface( PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm, int baud)
{
    mUART_tx     = tx;
    mUART_rx     = rx;
    mUART_cts    = cts;
    mUART_rts    = rts;;
    mUART_baud   = baud;
    mModuleReset = reset;
}

int C_SNIC_WifiInterface::init()
{   
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    /* Initialize UART */
    snic_core_p->initUart( mUART_tx, mUART_rx, mUART_baud );

    /* Module reset */
    snic_core_p->resetModule( mModuleReset );
    
    wait(1);
    /* Initialize SNIC API */
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("snic_init payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagSNIC_INIT_REQ_T req;
    // Make request
    req.cmd_sid  = UART_CMD_SID_SNIC_INIT_REQ;
    req.seq      = mUartRequestSeq++;
    req.buf_size[0] = 0x08;
    req.buf_size[1] = 0x00;

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_INIT_REQ_T), payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "snic_init failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("snic_init status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );
    
    return ret;
}

int C_SNIC_WifiInterface::getFWVersion( unsigned char *version_p )
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("getFWVersion payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagGEN_FW_VER_GET_REQ_T req;
    // Make request
    req.cmd_sid = UART_CMD_SID_GEN_FW_VER_GET_REQ;
    req.seq     = mUartRequestSeq++;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_GEN, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagGEN_FW_VER_GET_REQ_T), payload_buf_p->buf, command_array_p );

    int ret;
    
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "getFWversion failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() == 0 )
    {
        unsigned char version_len = payload_buf_p->buf[3];
        memcpy( version_p, &payload_buf_p->buf[4], version_len );
    }
    snic_core_p->freeCmdBuf( payload_buf_p );
    return 0;
}

int C_SNIC_WifiInterface::connect(const char *ssid_p, unsigned char ssid_len, E_SECURITY sec_type
                            , const char *sec_key_p, unsigned char sec_key_len)
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Parameter check(SSID)
    if( (ssid_p == NULL) || (ssid_len == 0) )
    {
        DEBUG_PRINT( "connect failed [ parameter NG:SSID ]\r\n" );
        return -1;
    }
    
    // Parameter check(Security key)
    if( (sec_type != e_SEC_OPEN) && ( (sec_key_len == 0) || (sec_key_p == NULL) ) )
    {
        DEBUG_PRINT( "connect failed [ parameter NG:Security key ]\r\n" );
        return -1;
    }
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("connect payload_buf_p NULL\r\n");
        return -1;
    }

    unsigned char *buf = &gCONNECT_BUF[0];
    unsigned int  buf_len = 0;
    unsigned int  command_len;

    memset( buf, 0, UART_CONNECT_BUF_SIZE );
    // Make request
    buf[0] = UART_CMD_SID_WIFI_JOIN_REQ;
    buf_len++;
    buf[1] = mUartRequestSeq++;
    buf_len++;
    // SSID
    memcpy( &buf[2], ssid_p, ssid_len );
    buf_len += ssid_len;
    buf_len++;
    
    // Security mode
    buf[ buf_len ] = (unsigned char)sec_type;
    buf_len++;

    // Security key
    if( sec_type != e_SEC_OPEN )
    {
        buf[ buf_len ] = sec_key_len;
        buf_len++;
        if( sec_key_len > 0 )
        {
            memcpy( &buf[buf_len], sec_key_p, sec_key_len );
            buf_len += sec_key_len;
        }
    }

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, UART_CMD_SID_WIFI_JOIN_REQ, buf
                        , buf_len, payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if(uartCmdMgr_p->getCommandStatus() != UART_CMD_RES_WIFI_ERR_ALREADY_JOINED)
    {
        DEBUG_PRINT( "Already connected\r\n" );
    }
    else
    {
        if( ret != 0 )
        {
            DEBUG_PRINT( "join failed\r\n" );
            snic_core_p->freeCmdBuf( payload_buf_p );
            return -1;
        }
    }
    
    if(uartCmdMgr_p->getCommandStatus() != 0)
    {
        DEBUG_PRINT("join status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );

    return ret;
}

int C_SNIC_WifiInterface::disconnect()
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("disconnect payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagWIFI_DISCONNECT_REQ_T req;
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_DISCONNECT_REQ;
    req.seq = mUartRequestSeq++;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagWIFI_DISCONNECT_REQ_T), payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "disconnect failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("disconnect status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        ret = -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );
    return ret;
}

int C_SNIC_WifiInterface::scan( const char *ssid_p, unsigned char *bssid_p
                        , void (*result_handler_p)(tagSCAN_RESULT_T *scan_result) )
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("scan payload_buf_p NULL\r\n");
        return -1;
    }
    
    C_SNIC_Core::tagWIFI_SCAN_REQ_T req;
    unsigned int  buf_len = 0;
    
    memset( &req, 0, sizeof(C_SNIC_Core::tagWIFI_SCAN_REQ_T) );
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_SCAN_REQ;
    buf_len++;
    req.seq = mUartRequestSeq++;
    buf_len++;
    
    // Set scan type(Active scan)
    req.scan_type = 0;
    buf_len++;
    // Set bss type(any)
    req.bss_type = 2;
    buf_len++;
    // Set BSSID
    if( bssid_p != NULL )
    {
        memcpy( req.bssid, bssid_p, BSSID_MAC_LENTH );
    }
    buf_len += BSSID_MAC_LENTH;
    // Set channel list(0)
    req.chan_list = 0;
    buf_len++;
    //Set SSID
    if( ssid_p != NULL )
    {
        strcpy( (char *)req.ssid, ssid_p );
        buf_len += strlen(ssid_p);
    }
    buf_len++;

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , buf_len, payload_buf_p->buf, command_array_p );

    // Set scan result callback 
    uartCmdMgr_p->setScanResultHandler( result_handler_p );
    
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    DEBUG_PRINT( "scan wait:%d\r\n", ret );
    if( ret != 0 )
    {
        DEBUG_PRINT( "scan failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("scan status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }

    snic_core_p->freeCmdBuf( payload_buf_p );

    return ret;
}

int C_SNIC_WifiInterface::wifi_on( const char *country_p )
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Parameter check
    if( country_p == NULL )
    {
        DEBUG_PRINT("wifi_on parameter error\r\n");
        return -1;
    }
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("wifi_on payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagWIFI_ON_REQ_T req;
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_ON_REQ;
    req.seq = mUartRequestSeq++;
    memcpy( req.country, country_p, COUNTRYC_CODE_LENTH );
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagWIFI_ON_REQ_T), payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "wifi_on failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("wifi_on status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );

    return ret;
}

int C_SNIC_WifiInterface::wifi_off()
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("wifi_off payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagWIFI_OFF_REQ_T req;
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_OFF_REQ;
    req.seq = mUartRequestSeq++;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagWIFI_OFF_REQ_T), payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "wifi_off failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("wifi_off status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );

    return ret;
}

int C_SNIC_WifiInterface::getRssi( signed char *rssi_p )
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    if( rssi_p == NULL )
    {
        DEBUG_PRINT("getRssi parameter error\r\n");
        return -1;
    }
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("getRssi payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagWIFI_GET_STA_RSSI_REQ_T req;
    
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_GET_STA_RSSI_REQ;
    req.seq     = mUartRequestSeq++;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int   command_len;
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagWIFI_GET_STA_RSSI_REQ_T), payload_buf_p->buf, command_array_p );

    int ret;
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "getRssi failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    *rssi_p = (signed char)payload_buf_p->buf[2];

    snic_core_p->freeCmdBuf( payload_buf_p );
    return 0;
}

int C_SNIC_WifiInterface::getWifiStatus( tagWIFI_STATUS_T *status_p)
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    if( status_p == NULL )
    {
        DEBUG_PRINT("getWifiStatus parameter error\r\n");
        return -1;
    }
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("getWifiStatus payload_buf_p NULL\r\n");
        return -1;
    }

    C_SNIC_Core::tagWIFI_GET_STATUS_REQ_T req;
    // Make request
    req.cmd_sid = UART_CMD_SID_WIFI_GET_STATUS_REQ;
    req.seq     = mUartRequestSeq++;
    req.interface = 0;

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int   command_len;
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_WIFI, req.cmd_sid, (unsigned char *)&req
                        , sizeof(C_SNIC_Core::tagWIFI_GET_STATUS_REQ_T), payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "getWifiStatus failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }

    // set status
    status_p->status = (E_WIFI_STATUS)payload_buf_p->buf[2];
    
    // set Mac address
    if( status_p->status != e_STATUS_OFF )
    {
        memcpy( status_p->mac_address, &payload_buf_p->buf[3], BSSID_MAC_LENTH );
    } 

    // set SSID
    if( ( status_p->status == e_STA_JOINED ) || ( status_p->status == e_AP_STARTED ) )
    {
        memcpy( status_p->ssid, &payload_buf_p->buf[9], strlen( (char *)&payload_buf_p->buf[9]) );
    } 

    snic_core_p->freeCmdBuf( payload_buf_p );
    return 0;
}

int C_SNIC_WifiInterface::setIPConfig( bool is_DHCP
    , const char *ip_p, const char *mask_p, const char *gateway_p )
{
    // Parameter check
    if( is_DHCP == false )
    {
        if( (ip_p == NULL) || (mask_p == NULL) ||(gateway_p == NULL) )
        {
            DEBUG_PRINT("setIPConfig parameter error\r\n");
            return -1;
        }            
    }

    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("setIPConfig payload_buf_p NULL\r\n");
        return -1;
    }

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    if( is_DHCP == true )
    {
        C_SNIC_Core::tagSNIC_IP_CONFIG_REQ_DHCP_T req;
        // Make request
        req.cmd_sid   = UART_CMD_SID_SNIC_IP_CONFIG_REQ;
        req.seq       = mUartRequestSeq++;
        req.interface = 0;
        req.dhcp      = 1;
        
        // Preparation of command
        command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_IP_CONFIG_REQ_DHCP_T), payload_buf_p->buf, command_array_p );
    }
    else
    {
        C_SNIC_Core::tagSNIC_IP_CONFIG_REQ_STATIC_T req;
        // Make request
        req.cmd_sid   = UART_CMD_SID_SNIC_IP_CONFIG_REQ;
        req.seq       = mUartRequestSeq++;
        req.interface = 0;
        req.dhcp      = 0;

        // Set paramter of address
        int addr_temp;
        addr_temp = C_SNIC_UartMsgUtil::addrToInteger( ip_p );
        C_SNIC_UartMsgUtil::convertIntToByteAdday( addr_temp, (char *)req.ip_addr );
        addr_temp = C_SNIC_UartMsgUtil::addrToInteger( mask_p );
        C_SNIC_UartMsgUtil::convertIntToByteAdday( addr_temp, (char *)req.netmask );
        addr_temp = C_SNIC_UartMsgUtil::addrToInteger( gateway_p );
        C_SNIC_UartMsgUtil::convertIntToByteAdday( addr_temp, (char *)req.gateway );

        // Preparation of command
        command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_IP_CONFIG_REQ_STATIC_T), payload_buf_p->buf, command_array_p );
    }
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    
    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "setIPConfig failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("setIPConfig status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }

    snic_core_p->freeCmdBuf( payload_buf_p );
    return ret;
}

char* C_SNIC_WifiInterface::getIPAddress() {
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    snic_core_p->lockAPI();
    // Get local ip address.
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("getIPAddress payload_buf_p NULL\r\n");
        snic_core_p->unlockAPI();
        return 0;
    }
 
    C_SNIC_Core::tagSNIC_GET_DHCP_INFO_REQ_T req;
    // Make request
    req.cmd_sid      = UART_CMD_SID_SNIC_GET_DHCP_INFO_REQ;
    req.seq          = mUartRequestSeq++;
    req.interface    = 0;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_GET_DHCP_INFO_REQ_T), payload_buf_p->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    // Wait UART response
    int ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "getIPAddress failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        snic_core_p->unlockAPI();
        return 0;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != UART_CMD_RES_SNIC_SUCCESS )
    {
        snic_core_p->freeCmdBuf( payload_buf_p );
        snic_core_p->unlockAPI();
        return 0;
    }
    
    snic_core_p->freeCmdBuf( payload_buf_p );
    snic_core_p->unlockAPI();
 
    sprintf(ip_addr, "%d.%d.%d.%d\0", payload_buf_p->buf[9], payload_buf_p->buf[10], payload_buf_p->buf[11], payload_buf_p->buf[12]);
 
    return ip_addr;
}

char* C_SNIC_WifiInterface::getMACAddress() {
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    snic_core_p->lockAPI();
    // Get local ip address.
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("getMACAddress payload_buf_p NULL\r\n");
        snic_core_p->unlockAPI();
        return 0;
    }
 
    C_SNIC_Core::tagSNIC_GET_DHCP_INFO_REQ_T req;
    // Make request
    req.cmd_sid      = UART_CMD_SID_SNIC_GET_DHCP_INFO_REQ;
    req.seq          = mUartRequestSeq++;
    req.interface    = 0;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_GET_DHCP_INFO_REQ_T), payload_buf_p->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    // Wait UART response
    int ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "getMACAddress failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        snic_core_p->unlockAPI();
        return 0;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != UART_CMD_RES_SNIC_SUCCESS )
    {
        snic_core_p->freeCmdBuf( payload_buf_p );
        snic_core_p->unlockAPI();
        return 0;
    }
    
    snic_core_p->freeCmdBuf( payload_buf_p );
    snic_core_p->unlockAPI();
 
    sprintf(mac_addr, "%d:%d:%d:%d:%d:%d\0", payload_buf_p->buf[3], payload_buf_p->buf[4], payload_buf_p->buf[5],
             payload_buf_p->buf[6], payload_buf_p->buf[7], payload_buf_p->buf[8]);
 
    return mac_addr;
}

int C_SNIC_WifiInterface::createUDPSocket()
{
    int ret;
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    FUNC_IN();
    // Get local ip address.
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("UDP bind payload_buf_p NULL\r\n");
        FUNC_OUT();
        return -1;
    }

    C_SNIC_Core::tagSNIC_UDP_CREATE_SOCKET_REQ_T create_req;
    
    // Make request
    create_req.cmd_sid  = UART_CMD_SID_SNIC_UDP_CREATE_SOCKET_REQ;
    create_req.seq      = mUartRequestSeq++;
    create_req.bind     = 0;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, create_req.cmd_sid, (unsigned char *)&create_req
                            , sizeof(C_SNIC_Core::tagSNIC_UDP_CREATE_SOCKET_REQ_T), payload_buf_p->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "UDP bind failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }

    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("UDP bind status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    int id = payload_buf_p->buf[3];
    //printf("Create UDP socket successfully with socketID = %d\n", id);

    snic_core_p->freeCmdBuf( payload_buf_p );
    FUNC_OUT();
    return id;
}

int C_SNIC_WifiInterface::bind(int id, const char *ip_addr, short port)
{
    int ret;
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    FUNC_IN();
    // Get local ip address.
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("UDP bind payload_buf_p NULL\r\n");
        FUNC_OUT();
        return -1;
    }

//    C_SNIC_Core::tagSNIC_UDP_CREATE_SOCKET_REQ_T create_req;
//    
//    // Make request
//    create_req.cmd_sid  = UART_CMD_SID_SNIC_UDP_CREATE_SOCKET_REQ;
//    create_req.seq      = mUartRequestSeq++;
//    create_req.bind     = 1;
//    // set ip addr ( byte order )
//    int local_addr = C_SNIC_UartMsgUtil::addrToInteger( ip_addr );
//    C_SNIC_UartMsgUtil::convertIntToByteAdday( local_addr, (char *)create_req.local_addr );
//    create_req.local_port[0] = ( (port & 0xFF00) >> 8 );
//    create_req.local_port[1] = (port & 0xFF);
//
//    unsigned char *command_array_p = snic_core_p->getCommandBuf();
//    unsigned int  command_len;
//    // Preparation of command
//    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, create_req.cmd_sid, (unsigned char *)&create_req
//                            , sizeof(C_SNIC_Core::tagSNIC_UDP_CREATE_SOCKET_REQ_T), payload_buf_p->buf, command_array_p );
//    // Send uart command request
//    snic_core_p->sendUart( command_len, command_array_p );
//
//    // Wait UART response
//    ret = uartCmdMgr_p->wait();
//    if( ret != 0 )
//    {
//        DEBUG_PRINT( "UDP bind failed\r\n" );
//        snic_core_p->freeCmdBuf( payload_buf_p );
//        FUNC_OUT();
//        return -1;
//    }
//
//    if( uartCmdMgr_p->getCommandStatus() != 0 )
//    {
//        DEBUG_PRINT("UDP bind status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
//        snic_core_p->freeCmdBuf( payload_buf_p );
//        FUNC_OUT();
//        return -1;
//    }
//    int id = payload_buf_p->buf[3];
//    //printf("Create UDP socket successfully with socketID = %d\n", id);
//    
//    // Initialize connection information
//    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );
//    if( con_info_p->recvbuf_p == NULL )
//    {
//        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", id);
//        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
//    }
//    con_info_p->is_connected = true;
//    con_info_p->is_received  = false;

    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );
    if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", id);
        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
    }
    C_SNIC_Core::tagSNIC_UDP_START_RECV_REQ_T recv_start_req;
    
    // Make request
    recv_start_req.cmd_sid         = UART_CMD_SID_SNIC_UDP_START_RECV_REQ;
    recv_start_req.seq             = mUartRequestSeq++;
    recv_start_req.socket_id       = id;
    recv_start_req.recv_bufsize[0] = ( (SNIC_UART_RECVBUF_SIZE & 0xFF00) >> 8 );
    recv_start_req.recv_bufsize[1] = (SNIC_UART_RECVBUF_SIZE & 0xFF);

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, recv_start_req.cmd_sid, (unsigned char *)&recv_start_req
                            , sizeof(C_SNIC_Core::tagSNIC_UDP_START_RECV_REQ_T), payload_buf_p->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "UDP recv start failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }

    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("UDP recv start status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    //printf("Start to receive successfully.\n");

    snic_core_p->freeCmdBuf( payload_buf_p );
    FUNC_OUT();
    return 0;
}

// -1 if unsuccessful, else number of bytes written
int C_SNIC_WifiInterface::sendTo(int id, const SocketAddress &remote, const void *packet, unsigned length)
{
 int ret;
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    // Get local ip address.
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("UDP bind payload_buf_p NULL\r\n");
        FUNC_OUT();
        return -1;
    }
    
    C_SNIC_Core::tagSNIC_UDP_SEND_FROM_SOCKET_REQ_T req;
    // Make request
    req.cmd_sid      = UART_CMD_SID_SNIC_UDP_SEND_FROM_SOCKET_REQ;
    req.seq          = mUartRequestSeq++;
    int addr_temp;
    addr_temp = C_SNIC_UartMsgUtil::addrToInteger( remote.get_ip_address() );

    C_SNIC_UartMsgUtil::convertIntToByteAdday( addr_temp, (char *)req.remote_ip );
    req.remote_port[0]  = ( (remote.get_port() & 0xFF00) >> 8 );
    req.remote_port[1]  = (remote.get_port() & 0xFF);
    req.payload_len[0]  = ( (length & 0xFF00) >> 8 );
    req.payload_len[1]  = (length & 0xFF);
    req.socket_id       = id;
    req.connection_mode = 1;
    
    // Initialize connection information
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );
    if( con_info_p != NULL )
    {
        con_info_p->from_ip   = addr_temp;
        con_info_p->from_port = remote.get_port();
    }
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_UDP_SEND_FROM_SOCKET_REQ_T), payload_buf_p->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "UDP send from socket failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != UART_CMD_RES_SNIC_SUCCESS )
    {
        DEBUG_PRINT("UDP send from socket status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    
    snic_core_p->freeCmdBuf( payload_buf_p );
    printf("UDP send from socket successfully.\n");
    return 0;
}

// -1 if unsuccessful, else number of bytes received
int C_SNIC_WifiInterface::receiveFrom(int id, SocketAddress *remote, char *data_p, unsigned length)
{    
    FUNC_IN();
    if( (data_p == NULL) || (length < 1) )
    {
        DEBUG_PRINT("UDPSocket::receiveFrom parameter error\r\n");
        FUNC_OUT();
        return -1;
    }

    C_SNIC_Core                    *snic_core_p  = C_SNIC_Core::getInstance();
    // Initialize connection information
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );

    if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT("UDPSocket::receiveFrom Conncection info error\r\n");
        FUNC_OUT();
        return -1;
    }

    char remote_ip[20] = {'\0'};
    sprintf( remote_ip, "%d.%d.%d.%d"
        , (con_info_p->from_ip >>24) & 0x000000ff
        , (con_info_p->from_ip >>16) & 0x000000ff
        , (con_info_p->from_ip >>8)  & 0x000000ff
        , (con_info_p->from_ip)      & 0x000000ff );
    //remote->set_ip_address( (const char*)remote_ip );
    
    con_info_p->mutex.lock();
    con_info_p->is_receive_complete = true;
    con_info_p->mutex.unlock();
    //if( con_info_p->is_received == false )
//    {
//        // Try receive
//        Thread::yield();
//        
//        if( con_info_p->is_received == false )
//        {
//            // No data received.
//            FUNC_OUT();
//            return 0;
//        }
//    }
    // Get packet data from buffer for receive.
    int i;
    for (i = 0; i < length; i++) 
    {
        if (con_info_p->recvbuf_p->dequeue(&data_p[i]) == false)
        {
            break;
        }
    }
    if( con_info_p->recvbuf_p->isEmpty() )
    {
        con_info_p->mutex.lock();
        con_info_p->is_received = false;
        con_info_p->mutex.unlock();
    }
    FUNC_OUT();
    return i;
}

int C_SNIC_WifiInterface::createSocket( unsigned char bind, unsigned int local_addr, unsigned short port )
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    FUNC_IN();
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf = snic_core_p->allocCmdBuf();
    if( payload_buf == NULL )
    {
        DEBUG_PRINT("createSocket payload_buf NULL\r\n");
        FUNC_OUT();
        return -1;
    }

    C_SNIC_Core::tagSNIC_TCP_CREATE_SOCKET_REQ_T req;
    int req_len = 0;
    
    // Make request
    req.cmd_sid  = UART_CMD_SID_SNIC_TCP_CREATE_SOCKET_REQ;
    req_len++;
    req.seq      = mUartRequestSeq++;
    req_len++;
    req.bind     = bind;
    req_len++;
    if( bind != 0 )
    {
        // set ip addr ( byte order )
        C_SNIC_UartMsgUtil::convertIntToByteAdday( local_addr, (char *)req.local_addr );
        req.local_port[0] = ( (port & 0xFF00) >> 8 );
        req.local_port[1] = (port & 0xFF);

        req_len = sizeof(C_SNIC_Core::tagSNIC_TCP_CREATE_SOCKET_REQ_T);
    }

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , req_len, payload_buf->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "createSocket failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf );
        FUNC_OUT();
        return -1;
    }

    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("createSocket status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf );
        FUNC_OUT();
        return -1;
    }
    int id = payload_buf->buf[3];
    snic_core_p->freeCmdBuf( payload_buf );
    FUNC_OUT();
    return id;
}

int C_SNIC_WifiInterface::resolveHostName( const char *host_p)
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    int ip_addr = 0;

    if( host_p == NULL )
    {
        DEBUG_PRINT("resolveHostName parameter error\r\n");
        return -1;
    }
    
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf = snic_core_p->allocCmdBuf();
    if( payload_buf == NULL )
    {
        DEBUG_PRINT("resolveHostName payload_buf NULL\r\n");
        return -1;
    }
    
    unsigned char *buf_p = (unsigned char *)getSocketSendBuf();
    unsigned int  buf_len = 0;

    memset( buf_p, 0, UART_REQUEST_PAYLOAD_MAX );
    // Make request
    buf_p[0] = UART_CMD_SID_SNIC_RESOLVE_NAME_REQ;
    buf_len++;
    buf_p[1] = mUartRequestSeq++;
    buf_len++;
    // Interface 
    buf_p[2] = 0;
    buf_len++;
    
    // Host name length
    int hostname_len = strlen(host_p);
    buf_p[3] = (unsigned char)hostname_len;
    buf_len++;
    memcpy( &buf_p[4], host_p, hostname_len );
    buf_len += hostname_len;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int   command_len;
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, UART_CMD_SID_SNIC_RESOLVE_NAME_REQ, buf_p
                        , buf_len, payload_buf->buf, command_array_p );
    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "resolveHostName failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf );
        return -1;
    }
    
    // check status
    if( uartCmdMgr_p->getCommandStatus() == 0 )
    {
        ip_addr = ((payload_buf->buf[3] << 24) & 0xFF000000)
                | ((payload_buf->buf[4] << 16) & 0xFF0000)
                | ((payload_buf->buf[5] << 8)  & 0xFF00)
                | (payload_buf->buf[6]);
    }
    
    snic_core_p->freeCmdBuf( payload_buf );
    return ip_addr;
}

int C_SNIC_WifiInterface::connectTCP( int id, const char *host_p, unsigned short port)
{
    int ret;
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
        
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("connect payload_buf_p NULL\r\n");
        FUNC_OUT();
        return -1;
    }
    
    C_SNIC_Core::tagSNIC_TCP_CONNECT_TO_SERVER_REQ_T req;
    // Make request
    req.cmd_sid      = UART_CMD_SID_SNIC_TCP_CONNECT_TO_SERVER_REQ;
    req.seq          = mUartRequestSeq++;
    req.socket_id    = id;
    
    // set ip addr ( byte order )
    int ip_addr = C_SNIC_UartMsgUtil::addrToInteger( host_p );
    C_SNIC_UartMsgUtil::convertIntToByteAdday( ip_addr, (char *)req.remote_addr );
    req.remote_port[0] = ( (port & 0xFF00) >> 8 );
    req.remote_port[1] = (port & 0xFF);
    req.recv_bufsize[0] = ( (SNIC_UART_RECVBUF_SIZE & 0xFF00) >> 8 );
    req.recv_bufsize[1] = (SNIC_UART_RECVBUF_SIZE & 0xFF);
    req.timeout         = 60;
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_TCP_CONNECT_TO_SERVER_REQ_T), payload_buf_p->buf, command_array_p );

    //uartCmdMgr_p->setCommandSID( UART_CMD_SID_SNIC_TCP_CONNECTION_STATUS_IND );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "connect failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        return -1;
    }

    int status = uartCmdMgr_p->getCommandStatus();
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );
    
    if (status)
    {
        if (status == UART_CMD_RES_SNIC_COMMAND_PENDING)
        {
            DEBUG_PRINT("wait for SNIC_TCP_CONNECTION_STATUS_IN event.\n");
            uartCmdMgr_p->setCommandSID( UART_CMD_SID_SNIC_TCP_CONNECTION_STATUS_IND );
            ret = uartCmdMgr_p->wait();
            if( ret != 0 )
            {
                printf( "TCP connect failed.\r\n" );
                snic_core_p->freeCmdBuf( payload_buf_p );
                return -1;
            }
            //while(con_info_p->is_connected == false)
//                Thread::yield();
        }
        else
        {
            DEBUG_PRINT("connect status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
            snic_core_p->freeCmdBuf( payload_buf_p );
            FUNC_OUT();
            return -1;
        }    
    }
    else
        printf("connect to TCP server successfully.\n");

    DEBUG_PRINT("finish TCP connection req.\n");
    snic_core_p->freeCmdBuf( payload_buf_p );

     //Initialize connection information
    
    if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT( "create recv buffer[socket:%d]\r\n", id);
        con_info_p->recvbuf_p = new CircBuffer<char>(SNIC_UART_RECVBUF_SIZE);
    }
    con_info_p->is_connected = true;
    con_info_p->is_received  = false;
    FUNC_OUT();
    return 0;
}

int C_SNIC_WifiInterface::receive(int id, char* data_p, int length)
{
    int i = 0;
    
    FUNC_IN();
    if( (data_p == NULL) || (length < 1) )
    {
        DEBUG_PRINT("TCPSocketConnection::receive parameter error\r\n");
        FUNC_OUT();
        return -1;
    }
    
    C_SNIC_Core                    *snic_core_p  = C_SNIC_Core::getInstance();
    // Initialize connection information
    C_SNIC_Core::tagCONNECT_INFO_T *con_info_p = snic_core_p->getConnectInfo( id );
    
    if( con_info_p->recvbuf_p == NULL )
    {
        DEBUG_PRINT("TCPSocketConnection::receive Conncection info error\r\n");
        FUNC_OUT();
        return -1;
    }

    // Check connection
    if( con_info_p->is_connected == false )
    {
        DEBUG_PRINT(" Socket id \"%d\" is not connected\r\n", id);
        FUNC_OUT();
        return -1;
    }
    
    while( con_info_p->is_received == false )
    {
        // Try receive
        Thread::yield();        
    }
    
    con_info_p->is_receive_complete = true;
    
    // Get packet data from buffer for receive.
    for (i = 0; i < length; i ++) 
    {
        if (con_info_p->recvbuf_p->dequeue(&data_p[i]) == false)
        {
            break;
        }
    }

    if( con_info_p->recvbuf_p->isEmpty() )
    {
        con_info_p->mutex.lock();
        con_info_p->is_received = false;
        con_info_p->mutex.unlock();
    }

    FUNC_OUT();
    return i;
}

int C_SNIC_WifiInterface::send(int id, const void *data_p, unsigned length)
{
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();

    FUNC_IN();
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf_p = snic_core_p->allocCmdBuf();
    if( payload_buf_p == NULL )
    {
        DEBUG_PRINT("connect payload_buf_p NULL\r\n");
        FUNC_OUT();
        return -1;
    }
    
    C_SNIC_Core::tagSNIC_TCP_SEND_FROM_SOCKET_REQ_T req;
    // Make request
    req.cmd_sid       = UART_CMD_SID_SNIC_SEND_FROM_SOCKET_REQ;
    req.seq           = mUartRequestSeq++;
    req.socket_id     = id;
    req.option        = 0;
    req.payload_len[0]= ( (length & 0xFF00) >> 8 );
    req.payload_len[1]= (length & 0xFF);
    
    int req_size     = sizeof(C_SNIC_Core::tagSNIC_TCP_SEND_FROM_SOCKET_REQ_T);
    char *send_buf_p = getSocketSendBuf();
    memcpy( send_buf_p, &req, req_size );
    memcpy( &send_buf_p[req_size], data_p, length );
    
    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int   command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)send_buf_p
                            , req_size + length, payload_buf_p->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    // Wait UART response
    int ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "send failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != UART_CMD_RES_SNIC_SUCCESS )
    {
        DEBUG_PRINT("send status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf_p );
        FUNC_OUT();
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf_p );

    // SNIC_SEND_FROM_SOCKET_REQ
    FUNC_OUT();
    return length;
}

int C_SNIC_WifiInterface::close(int id)
{
    DEBUG_PRINT("close socket : %d\n", id);
    C_SNIC_Core               *snic_core_p  = C_SNIC_Core::getInstance();
    C_SNIC_UartCommandManager *uartCmdMgr_p = snic_core_p->getUartCommand();
    
    FUNC_IN();
    // Get buffer for response payload from MemoryPool
    tagMEMPOOL_BLOCK_T *payload_buf = snic_core_p->allocCmdBuf();
    if( payload_buf == NULL )
    {
        DEBUG_PRINT("socket close payload_buf NULL\r\n");
        FUNC_OUT();
        return -1;
    }

    C_SNIC_Core::tagSNIC_CLOSE_SOCKET_REQ_T req;
    
    // Make request
    req.cmd_sid   = UART_CMD_SID_SNIC_CLOSE_SOCKET_REQ;
    req.seq       = mUartRequestSeq++;
    req.socket_id = id;

    unsigned char *command_array_p = snic_core_p->getCommandBuf();
    unsigned int  command_len;
    // Preparation of command
    command_len = snic_core_p->preparationSendCommand( UART_CMD_ID_SNIC, req.cmd_sid, (unsigned char *)&req
                            , sizeof(C_SNIC_Core::tagSNIC_CLOSE_SOCKET_REQ_T), payload_buf->buf, command_array_p );

    // Send uart command request
    snic_core_p->sendUart( command_len, command_array_p );

    int ret;
    // Wait UART response
    ret = uartCmdMgr_p->wait();
    if( ret != 0 )
    {
        DEBUG_PRINT( "socket close failed\r\n" );
        snic_core_p->freeCmdBuf( payload_buf );
        FUNC_OUT();
        return -1;
    }
    
    if( uartCmdMgr_p->getCommandStatus() != 0 )
    {
        DEBUG_PRINT("socket close status:%02x\r\n", uartCmdMgr_p->getCommandStatus());
        snic_core_p->freeCmdBuf( payload_buf );
        FUNC_OUT();
        return -1;
    }
    snic_core_p->freeCmdBuf( payload_buf );
    FUNC_OUT();
    return 0;
}

char *C_SNIC_WifiInterface::getSocketSendBuf()
{
    return gSOCKET_SEND_BUF;
}