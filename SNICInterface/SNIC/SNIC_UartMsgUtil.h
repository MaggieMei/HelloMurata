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
#ifndef _SNIC_WIFI_UART_MSG_UTIL_H_
#define _SNIC_WIFI_UART_MSG_UTILH_
#include "MurataObject.h"
#include "mbed.h"
#include "rtos.h"
#include "RawSerial.h"

#define UART_CMD_SOM        0x02
#define UART_CMD_EOM        0x04
#define UART_CMD_ESC        0x10

/* SNIC UART Command ID */
#define UART_CMD_ID_GEN     0x01    //General command
#define UART_CMD_ID_SNIC    0x70    //SNIC command
#define UART_CMD_ID_WIFI    0x50    //Wi-Fi command

/* SNIC UART Subcommand ID */
#define UART_CMD_SID_GEN_PWR_UP_IND          0x00   //Power up indication
#define UART_CMD_SID_GEN_FW_VER_GET_REQ      0x08   //Get firmware version string

#define UART_CMD_SID_SNIC_INIT_REQ                      0x00    // SNIC API initialization
#define UART_CMD_SID_SNIC_CLEANUP_REQ                   0x01    // SNIC API cleanup
#define UART_CMD_SID_SNIC_SEND_FROM_SOCKET_REQ          0x02    // Send from socket
#define UART_CMD_SID_SNIC_CLOSE_SOCKET_REQ              0x03    // Close socket
#define UART_CMD_SID_SNIC_ SOCKET _PARTIAL_CLOSE_ REQ   0x04    // Socket partial close
#define UART_CMD_SID_SNIC_GETSOCKOPT_REQ                0x05    // Get socket option
#define UART_CMD_SID_SNIC_SETSOCKOPT_REQ                0x06    // Set socket option
#define UART_CMD_SID_SNIC_SOCKET_GETNAME_REQ            0x07    // Get name or peer name
#define UART_CMD_SID_SNIC_SEND_ARP_REQ                  0x08    // Send ARP request
#define UART_CMD_SID_SNIC_GET_DHCP_INFO_REQ             0x09    // Get DHCP info
#define UART_CMD_SID_SNIC_RESOLVE_NAME_REQ              0x0A    // Resolve a host name to IP address
#define UART_CMD_SID_SNIC_IP_CONFIG_REQ                 0x0B    // Configure DHCP or static IP
#define UART_CMD_SID_SNIC_DATA_IND_ACK_CONFIG_REQ       0x0C    // ACK configuration for data indications
#define UART_CMD_SID_SNIC_TCP_CREATE_SOCKET_REQ         0x10    // Create TCP socket
#define UART_CMD_SID_SNIC_TCP_CREATE_CONNECTION_REQ     0x11    // Create TCP connection server
#define UART_CMD_SID_SNIC_TCP_CONNECT_TO_SERVER_REQ     0x12    // Connect to TCP server
#define UART_CMD_SID_SNIC_UDP_CREATE_SOCKET_REQ         0x13    // Create UDP socket
#define UART_CMD_SID_SNIC_UDP_START_RECV_REQ            0x14    // Start UDP receive on socket
#define UART_CMD_SID_SNIC_UDP_SIMPLE_SEND_REQ           0x15    // Send UDP packet
#define UART_CMD_SID_SNIC_UDP_SEND_FROM_SOCKET_REQ      0x16    // Send UDP packet from socket
#define UART_CMD_SID_SNIC_HTTP_REQ                      0x17    // Send HTTP request
#define UART_CMD_SID_SNIC_HTTP_MORE_REQ                 0x18    // Send HTTP more data request
#define UART_CMD_SID_SNIC_HTTPS_REQ                     0x19    // Send HTTPS request
#define UART_CMD_SID_SNIC_TCP_CREATE_ADV_TLS_SOCKET_REQ 0x1A    // Create advanced TLS TCP socket
#define UART_CMD_SID_SNIC_TCP_CREAET_SIMPLE_TLS_SOCKET_REQ  0x1B    // Create simple TLS TCP socket
#define UART_CMD_SID_SNIC_TCP_CONNECTION_STATUS_IND     0x20    // Connection status indication
#define UART_CMD_SID_SNIC_TCP_CLIENT_SOCKET_IND         0x21    // TCP client socket indication
#define UART_CMD_SID_SNIC_CONNECTION_RECV_IND           0x22    // TCP or connected UDP packet received indication
#define UART_CMD_SID_SNIC_UDP_RECV_IND                  0x23    // UCP packet received indication
#define UART_CMD_SID_SNIC_ARP_REPLY_IND                 0x24    // ARP reply indication
#define UART_CMD_SID_SNIC_HTTP_RSP_IND                  0x25    // HTTP response indication

#define UART_CMD_SID_WIFI_ON_REQ             0x00   // Turn on Wifi
#define UART_CMD_SID_WIFI_OFF_REQ            0x01   // Turn off Wifi
#define UART_CMD_SID_WIFI_JOIN_REQ           0x02   // Associate to a network
#define UART_CMD_SID_WIFI_DISCONNECT_REQ     0x03   // Disconnect from a network
#define UART_CMD_SID_WIFI_GET_STATUS_REQ     0x04   // Get WiFi status
#define UART_CMD_SID_WIFI_SCAN_REQ           0x05   // Scan WiFi networks
#define UART_CMD_SID_WIFI_GET_STA_RSSI_REQ   0x06   // Get STA signal strength (RSSI)
#define UART_CMD_SID_WIFI_AP_CTRL_REQ        0x07   // Soft AP on-off control
#define UART_CMD_SID_WIFI_WPS_REQ            0x08   // Start WPS process
#define UART_CMD_SID_WIFI_AP_GET_CLIENT_REQ  0x0A   // Get clients that are associated to the soft AP.
#define UART_CMD_SID_WIFI_NETWORK_STATUS_IND 0x10   // Network status indication
#define UART_CMD_SID_WIFI_SCAN_RESULT_IND    0x11   // Scan result indication

/* SNIC UART Command response status code */
#define UART_CMD_RES_SNIC_SUCCESS                   0x00
#define UART_CMD_RES_SNIC_FAIL                      0x01
#define UART_CMD_RES_SNIC_INIT_FAIL                 0x02
#define UART_CMD_RES_SNIC_CLEANUP_FAIL              0x03
#define UART_CMD_RES_SNIC_GETADDRINFO_FAIL          0x04
#define UART_CMD_RES_SNIC_CREATE_SOCKET_FAIL        0x05
#define UART_CMD_RES_SNIC_BIND_SOCKET_FAIL          0x06
#define UART_CMD_RES_SNIC_LISTEN_SOCKET_FAIL        0x07
#define UART_CMD_RES_SNIC_ACCEPT_SOCKET_FAIL        0x08
#define UART_CMD_RES_SNIC_PARTIAL_CLOSE_FAIL        0x09
#define UART_CMD_RES_SNIC_SOCKET_PARTIALLY_CLOSED   0x0A
#define UART_CMD_RES_SNIC_SOCKET_CLOSED             0x0B
#define UART_CMD_RES_SNIC_CLOSE_SOCKET_FAIL         0x0C
#define UART_CMD_RES_SNIC_PACKET_TOO_LARGE          0x0D
#define UART_CMD_RES_SNIC_SEND_FAIL                 0x0E
#define UART_CMD_RES_SNIC_CONNECT_TO_SERVER_FAIL    0x0F
#define UART_CMD_RES_SNIC_NOT_ENOUGH_MEMORY         0x10
#define UART_CMD_RES_SNIC_TIMEOUT                   0x11
#define UART_CMD_RES_SNIC_CONNECTION_UP             0x12
#define UART_CMD_RES_SNIC_GETSOCKOPT_FAIL           0x13
#define UART_CMD_RES_SNIC_SETSOCKOPT_FAIL           0x14
#define UART_CMD_RES_SNIC_INVALID_ARGUMENT          0x15
#define UART_CMD_RES_SNIC_SEND_ARP_FAIL             0x16
#define UART_CMD_RES_SNIC_INVALID_SOCKET            0x17
#define UART_CMD_RES_SNIC_COMMAND_PENDING           0x18
#define UART_CMD_RES_SNIC_SOCKET_NOT_BOUND          0x19
#define UART_CMD_RES_SNIC_SOCKET_NOT_CONNECTED      0x1A
#define UART_CMD_RES_SNIC_NO_NETWORK                0x20
#define UART_CMD_RES_SNIC_INIT_NOT_DONE             0x21
#define UART_CMD_RES_SNIC_NET_IF_FAIL               0x22
#define UART_CMD_RES_SNIC_NET_IF_NOT_UP             0x23
#define UART_CMD_RES_SNIC_DHCP_START_FAIL           0x24

#define UART_CMD_RES_WIFI_SUCCESS               0x00
#define UART_CMD_RES_WIFI_ERR_UNKNOWN_COUNTRY   0x01
#define UART_CMD_RES_WIFI_ERR_INIT_FAIL         0x02
#define UART_CMD_RES_WIFI_ERR_ALREADY_JOINED    0x03
#define UART_CMD_RES_WIFI_ERR_AUTH_TYPE         0x04
#define UART_CMD_RES_WIFI_ERR_JOIN_FAIL         0x05
#define UART_CMD_RES_WIFI_ERR_NOT_JOINED        0x06
#define UART_CMD_RES_WIFI_ERR_LEAVE_FAILED      0x07
#define UART_CMD_RES_WIFI_COMMAND_PENDING       0x08
#define UART_CMD_RES_WIFI_WPS_NO_CONFIG         0x09
#define UART_CMD_RES_WIFI_NETWORK_UP            0x10
#define UART_CMD_RES_WIFI_NETWORK_DOWN          0x11
#define UART_CMD_RES_WIFI_FAIL                  0xFF

/** UART Command sequence number
*/
static unsigned char mUartRequestSeq;  

/** Internal utility class used by any other classes.
 */
class C_SNIC_UartMsgUtil: public C_MurataObject
{
friend class C_SNIC_Core;
friend class C_SNIC_WifiInterface;
friend class UDPSocket;
friend class TCPSocketConnection;
friend class Socket;
    
private:
    C_SNIC_UartMsgUtil();
    virtual ~C_SNIC_UartMsgUtil();
    
    /** Make SNIC UART command.
            @param cmd_id         Command ID
            @param payload_p      Payload pointer
            @param uart_command_p UART Command pointer [output]
            @return UART Command length    
    */
    static unsigned int makeRequest( unsigned char cmd_id, unsigned char *payload_p, unsigned short payload_len, unsigned char *uart_command_p );


    /** Get uart command from receive data.
            @param recvdata_len   Receive data length
            @param recvdata_p     Pointer of received data from UART
            @param command_id_p   Pointer of command ID[output]
            @param payload_p      Pointer of payload[output]
            @return Payload length    
    */
    static unsigned int getResponsePayload( unsigned int recvdata_len, unsigned char *recvdata_p
                                    , unsigned char *command_id_p,  unsigned char *payload_p );

    /** Convert a string to number of ip address.
            @param recvdata_p     Pointer of string of IP address
            @return Number of ip address    
    */
    static int addrToInteger( const char *addr_p );

    /** Convert a integer to byte array of ip address.
            @param recvdata_p     Pointer of string of IP address
            @return Number of ip address    
    */
    static void convertIntToByteAdday( int addr, char *addr_array_p );

protected:

};

#endif /* _YD_WIFI_UART_MSG_H_ */
