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
#ifndef _SNIC_H_
#define _SNIC_H_

#include "SNIC_Core.h"
#include "MurataObject.h"
#include "WiFiInterface.h"

/** Wi-Fi status used by getWifiStatus(). */
typedef struct
{
    /** status */
    E_WIFI_STATUS status;
    /** Mac address */
    char mac_address[BSSID_MAC_LENTH];
    /** SSID */
    char ssid[SSID_MAX_LENGTH+1];
}tagWIFI_STATUS_T;

/** Interface class for using SNIC UART.
 */
class C_SNIC_WifiInterface : public C_MurataObject {

public:    
    /** Constructor
        @param tx mbed pin to use for tx line of Serial interface
        @param rx mbed pin to use for rx line of Serial interface
        @param cts mbed pin to use for cts line of Serial interface
        @param rts mbed pin to use for rts line of Serial interface
        @param reset reset pin of the wifi module
        @param alarm alarm pin of the wifi module (default: NC)
        @param baud baud rate of Serial interface (default: 115200)
    */
    C_SNIC_WifiInterface(PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm = NC, int baud = 115200);

     /** Initialize the interface.
        @return 0 on success, a negative number on failure
    */
    int init();

    /** Get Firmware version string.
        @param  version_p Pointer of FW version string.(null terminated)[output]
        @return 0:success/other:fail
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int getFWVersion( unsigned char *version_p );

    /** Connect to AP
        @param ssid_p       Wi-Fi SSID(null terminated)
        @param ssid_len     Wi-Fi SSID length
        @param sec_type     Wi-Fi security type.
        @param sec_key_len  Wi-Fi passphrase or security key length
        @param sec_key_p    Wi-Fi passphrase or security key
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int connect(const char *ssid_p, unsigned char ssid_len, E_SECURITY sec_type, const char *sec_key_p, unsigned char sec_key_len);
  
    /** Disconnect from AP
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int disconnect();

    /** Scan AP
        @param  ssid_p  Wi-Fi SSID(null terminated)
                        If do not specify SSID, set to NULL.
        @param  bssid_p Wi-Fi BSSID(null terminated)
                        If do not specify SSID, set to NULL.
        @param  result_handler_p Pointer of scan result callback function.
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
                Scan results will be notified by asynchronous callback function.
                If there is no continuity data, scan_result　will be set NULL..
    */
    int scan( const char *ssid_p, unsigned char *bssid_p
            ,void (*result_handler_p)(tagSCAN_RESULT_T *scan_result) );

    /** Wi-Fi Turn on
        @param country_p Pointer of country code.
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int wifi_on( const char *country_p );

    /** Wi-Fi Turn off
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int wifi_off();

    /** Get Wi-Fi RSSI
        @param rssi_p Pointer of RSSI.[output]
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int getRssi( signed char *rssi_p );

    /** Get Wi-Fi status
        @param status_p Pointer of status structure.[output]
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int getWifiStatus( tagWIFI_STATUS_T *status_p);

    /** Set IP configuration
        @param is_DHCP   true:DHCP false:static IP.
        @param ip_p      Pointer of strings of IP address.(null terminate).
        @param mask_p    Pointer of strings of Netmask.(null terminate).
        @param gateway_p Pointer of strings of gateway address.(null terminate).
        @return 0 on success, a negative number on failure
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int setIPConfig( bool is_DHCP, const char *ip_p=NULL, const char *mask_p=NULL, const char *gateway_p=NULL );

    /** Get the IP address of your SNIC interface
     * \return a pointer to a string containing the IP address
     */
    static char* getIPAddress();
    
    /** Get the MAC address of your SNIC interface
     * \return a pointer to a string containing the MAC address
     */
    static char* getMACAddress();
    
    /** Bind a UDP Server Socket to a specific port
    \param port The port to listen for incoming connections on
    \return 0 on success, -1 on failure.
    */
    int bind(int id, const char *ip_addr, short port);
    
    
    /** Send a packet to a remote endpoint
    \param remote   The remote endpoint
    \param packet   The packet to be sent
    \param length   The length of the packet to be sent
    \return the number of written bytes on success (>=0) or -1 on failure
    */
    int sendTo(int id, const SocketAddress &remote, const void *packet, unsigned length);
    
    /** Receive a packet from a remote endpoint
    \param remote   The remote endpoint
    \param buffer   The buffer for storing the incoming packet data. If a packet
           is too long to fit in the supplied buffer, excess bytes are discarded
    \param length   The length of the buffer
    \return the number of received bytes on success (>=0) or -1 on failure
    */
    int receiveFrom(int id, SocketAddress *remote, char *buffer, unsigned length);
    
       /** Connects this TCP socket to the server
        @param host The strings of ip address.
        @param port The host's port to connect to.
        @return 0 on success, -1 on failure.
        @note   This function is blocked until a returns.
                When you use it by UI thread, be careful. 
    */
    int connectTCP( int id, const char *ip_addr_p, unsigned short port );
    
    /** Create UDP socket
    \return socket id on success, -1 on fail    
    */
    int createUDPSocket();
    
    /** Resolve host name
     @param host_p point to host name
    */
    int resolveHostName( const char *host_p );
    
    /** Create TCP socket
    \return socket id on success, -1 on fail    
    */
    int createSocket( unsigned char bind = 0, unsigned int local_addr = 0, unsigned short port = 0 );
    
    char* getSocketSendBuf();
    
    /** Send data to the remote host.
        @param data The buffer to send to the host.
        @param length The length of the buffer to send.
        @return the number of written bytes on success (>=0) or -1 on failure
     */
    int send(int id, const void *data_p, unsigned length);
    
    /** Receive data from the remote host.
        @param data The buffer in which to store the data received from the host.
        @param length The maximum length of the buffer.
        @return the number of received bytes on success (>=0) or -1 on failure
     */
    int receive(int id, char *data_p, int length);
    
    /** Close UDP/TCP socket
     @param id socket id to be closed
    */
    int close(int id);

private:
    PinName mUART_tx;
    PinName mUART_rx;
    PinName mUART_cts;
    PinName mUART_rts;
    int     mUART_baud;
    PinName mModuleReset;
    
protected:
};
#endif  /* _YD_WIFIINTERFACE_H_ */