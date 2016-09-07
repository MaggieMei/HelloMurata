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
#ifndef _SNIC_CORE_H_
#define _SNIC_CORE_H_

#include "MurataObject.h"
#include "mbed.h"
#include "rtos.h"
#include "RawSerial.h"
#include "CBuffer.h"

#include "SNIC_UartCommandManager.h"

#define UART_REQUEST_PAYLOAD_MAX 2048

#define MEMPOOL_BLOCK_SIZE  2048
#define MEMPOOL_PAYLOAD_NUM 1
#define MAX_SOCKET_ID   5

#define MEMPOOL_UART_RECV_NUM 6
#define SNIC_UART_RECVBUF_SIZE         2048

///** Wi-Fi security
// */
//typedef enum SECURITY {
//    /** Securiry Open */
//    e_SEC_OPEN       = 0x00,
//    /** Securiry WEP */
//    e_SEC_WEP        = 0x01,
//    /** Securiry WPA-PSK(TKIP) */
//    e_SEC_WPA_TKIP   = 0x02,
//    /** Securiry WPA2-PSK(AES) */
//    e_SEC_WPA2_AES   = 0x04,
//    /** Securiry WPA2-PSK(TKIP/AES) */
//    e_SEC_WPA2_MIXED = 0x06,
//    /** Securiry WPA-PSK(AES) */
//    e_SEC_WPA_AES    = 0x07
//}E_SECURITY;

/** Wi-Fi status
 */
typedef enum WIFI_STATUS {
    /** Wi-Fi OFF */
    e_STATUS_OFF = 0,
    /** No network */
    e_NO_NETWORK,
    /** Connected to AP (STA mode) */
    e_STA_JOINED,
    /** Started  on AP mode */
    e_AP_STARTED
}E_WIFI_STATUS;

/** Memorypool
 */
typedef struct
{
    unsigned int  size;
    unsigned int  demand_size;
    unsigned char buf[MEMPOOL_BLOCK_SIZE];
}tagMEMPOOL_BLOCK_T;

/** Internal class used by any other classes. This class is singleton.
 */
class C_SNIC_Core: public C_MurataObject
{
friend class C_SNIC_UartCommandManager;
friend class C_SNIC_WifiInterface;
friend class UDPSocket;
friend class TCPSocketConnection;
friend class Socket;

private:
    /** Wi-Fi Network type
     */
    typedef enum NETWORK_TYPE {
        /** Infrastructure */
        e_INFRA = 0,
        /** Adhoc */
        e_ADHOC = 1
    }E_NETWORK_TYPE;

    /** Connection information
    */
    typedef struct {
        CircBuffer<char>    *recvbuf_p;
        bool                is_connected;
        bool                is_received;
        bool                is_receive_complete;
        int                 parent_socket;
        int                 from_ip;
        short               from_port;
        bool                is_accept;
        Mutex               mutex;
    }tagCONNECT_INFO_T;

    /** UDP Recv information
    */
    typedef struct {
        CircBuffer<char>    *recvbuf_p;
        int                 from_ip;
        short               from_port;
        int                 parent_socket;
        bool                is_received;
        Mutex               mutex;
    }tagUDP_RECVINFO_T;

    /** GEN_FW_VER_GET_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
    }tagGEN_FW_VER_GET_REQ_T;

    /** SNIC_INIT_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char buf_size[2];
    }tagSNIC_INIT_REQ_T;
    
    /** SNIC_IP_CONFIG_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char interface;
        unsigned char dhcp;
    }tagSNIC_IP_CONFIG_REQ_DHCP_T;

    /** SNIC_IP_CONFIG_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char interface;
        unsigned char dhcp;
        unsigned char ip_addr[4];
        unsigned char netmask[4];
        unsigned char gateway[4];
    }tagSNIC_IP_CONFIG_REQ_STATIC_T;

    /** SNIC_TCP_CREATE_SOCKET_REQ */
    typedef struct
    {
        unsigned char  cmd_sid;
        unsigned char  seq;
        unsigned char  bind;
        unsigned char  local_addr[4];
        unsigned char  local_port[2];
    }tagSNIC_TCP_CREATE_SOCKET_REQ_T;

    /** SNIC_CLOSE_SOCKET_REQ */
    typedef struct
    {
        unsigned char  cmd_sid;
        unsigned char  seq;
        unsigned char  socket_id;
    }tagSNIC_CLOSE_SOCKET_REQ_T;
    
    /** SNIC_TCP_SEND_FROM_SOCKET_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char socket_id;
        unsigned char option;
        unsigned char payload_len[2];
    }tagSNIC_TCP_SEND_FROM_SOCKET_REQ_T;
    
    /** SNIC_TCP_CREATE_CONNECTION_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char socket_id;
        unsigned char recv_bufsize[2];
        unsigned char max_client;
    }tagSNIC_TCP_CREATE_CONNECTION_REQ_T;

    /** SNIC_TCP_CONNECT_TO_SERVER_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char socket_id;
        unsigned char remote_addr[4];
        unsigned char remote_port[2];
        unsigned char recv_bufsize[2];
        unsigned char timeout;
    }tagSNIC_TCP_CONNECT_TO_SERVER_REQ_T;
    
    /** SNIC_UDP_SIMPLE_SEND_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char remote_ip[4];
        unsigned char remote_port[2];
        unsigned char payload_len[2];
    }tagSNIC_UDP_SIMPLE_SEND_REQ_T;

    /** SNIC_UDP_CREATE_SOCKET_REQ */
    typedef struct
    {
        unsigned char  cmd_sid;
        unsigned char  seq;
        unsigned char  bind;
        unsigned char  local_addr[4];
        unsigned char  local_port[2];
    }tagSNIC_UDP_CREATE_SOCKET_REQ_T;
    
    /** SNIC_UDP_CREATE_SOCKET_REQ */
    typedef struct
    {
        unsigned char  cmd_sid;
        unsigned char  seq;
        unsigned char  bind;
    }tagSNIC_UDP_CREATE_SOCKET_REQ_CLIENT_T;

    /** SNIC_UDP_SEND_FROM_SOCKET_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char remote_ip[4];
        unsigned char remote_port[2];
        unsigned char socket_id;
        unsigned char connection_mode;
        unsigned char payload_len[2];
    }tagSNIC_UDP_SEND_FROM_SOCKET_REQ_T;

    /** SNIC_UDP_START_RECV_REQ */
    typedef struct
    {
        unsigned char  cmd_sid;
        unsigned char  seq;
        unsigned char  socket_id;
        unsigned char  recv_bufsize[2];
    }tagSNIC_UDP_START_RECV_REQ_T;

    /** SNIC_GET_DHCP_INFO_REQ */
    typedef struct
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char interface;
    }tagSNIC_GET_DHCP_INFO_REQ_T;
    
    /** WIFI_ON_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
        char country[COUNTRYC_CODE_LENTH];
    }tagWIFI_ON_REQ_T;
    
    /** WIFI_OFF_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
    }tagWIFI_OFF_REQ_T;
    
    /** WIFI_DISCONNECT_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
    }tagWIFI_DISCONNECT_REQ_T;
    
    /** WIFI_GET_STA_RSSI_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
    }tagWIFI_GET_STA_RSSI_REQ_T;
    
    /** WIFI_SCAN_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char scan_type;
        unsigned char bss_type;
        unsigned char bssid[BSSID_MAC_LENTH];
        unsigned char chan_list;
        unsigned char ssid[SSID_MAX_LENGTH+1];
    }tagWIFI_SCAN_REQ_T;

    /** WIFI_GET_STATUS_REQ Command */
    typedef struct 
    {
        unsigned char cmd_sid;
        unsigned char seq;
        unsigned char interface;
    }tagWIFI_GET_STATUS_REQ_T;
    
    /** Get buffer for command from memory pool.
        @return Pointer of buffer
     */
    tagMEMPOOL_BLOCK_T *allocCmdBuf();
    
    /** Release buffer to memory pool.
        @param buf_p Pointer of buffer
     */
    void freeCmdBuf( tagMEMPOOL_BLOCK_T *buf_p );

    /** Get buffer for command from memory pool.
        @return Pointer of buffer
     */
    tagMEMPOOL_BLOCK_T *allocUartRcvBuf();
    
    /** Release buffer to memory pool.
        @param buf_p Pointer of buffer
     */
    void freeUartRecvBuf( tagMEMPOOL_BLOCK_T *buf_p );

    /** Module Reset
    */
    int resetModule( PinName reset );

/** Initialize UART
    */
    int initUart( PinName tx, PinName rx, int baud );

    /** Send data to UART
        @param len  Length of send data
        @param data Pointer of send data
        @return 0:success/other:fail
    */
    int sendUart( unsigned int len, unsigned char *data );

    /** Preparation of the UART command
        @param cmd_id           UART Command ID
        @param cmd_sid          UART Command  SubID
        @param req_buf_p        Pointer of UART request buffer
        @param req_buf_len      Length of UART request buffer
        @param response_buf_p   Pointer of UART response buffer
        @param command_p        Pointer of UART command[output]
        @return Length of UART command.
    */
    unsigned int preparationSendCommand( unsigned char cmd_id, unsigned char cmd_sid
                                , unsigned char *req_buf_p,    unsigned int req_buf_len
                                , unsigned char *response_buf_p, unsigned char *command_p );

    /** 
        Get pointer of connection information.
        @param socket_id    Socket ID
        @return The pointer of connection information
    */
    C_SNIC_Core::tagCONNECT_INFO_T   *getConnectInfo( int socket_id );

    /** 
        Get pointer of UDP Recv information.
        @param socket_id    Socket ID
        @return The pointer of UDP Recv information
    */
    C_SNIC_Core::tagUDP_RECVINFO_T   *getUdpRecvInfo( int socket_id );

                                /**
        Get pointer of the instance of C_SNIC_UartCommandManager.
        @return The pointer of the instance of C_SNIC_UartCommandManager.
    */
    C_SNIC_UartCommandManager *getUartCommand();

    unsigned char *getCommandBuf();

    /** Get an instance of the C_SNIC_Core class.
        @return Instance of the C_SNIC_Core class
        @note   Please do not create an instance in the default constructor this class.
                Please use this method when you want to get an instance.
    */
    static C_SNIC_Core *getInstance();

    /** Mutex lock of API calls
    */
    void lockAPI( void );

    /** Mutex unlock of API calls
    */
    void unlockAPI( void );

private:
    static C_SNIC_Core        *mInstance_p;
    Thread                    *mUartRecvThread_p;
    Thread                    *mUartRecvDispatchThread_p;
    RawSerial                 *mUart_p;
    Mutex                      mUartMutex;
    Mutex                      mAPIMutex;
    
//    DigitalInOut             mModuleReset;
    C_SNIC_UartCommandManager *mUartCommand_p;
    
    CircBuffer<char>          *mUartRecvBuf_p;  // UART RecvBuffer
    
    /** Socket buffer */
    tagCONNECT_INFO_T   mConnectInfo[MAX_SOCKET_ID+1];
  
    /** UDP Information */
    tagUDP_RECVINFO_T   mUdpRecvInfo[MAX_SOCKET_ID+1];

    /** Constructor
     */
    C_SNIC_Core();

//    virtual ~C_SNIC_Core();
    
    static void uartRecvCallback( void );
    /** Receiving thread of UART
    */
    static void uartRecvDispatchThread( void const *args_p );
};

#endif
