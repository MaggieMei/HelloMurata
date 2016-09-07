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
#include "MurataInterface.h"
#include "SNIC_UartMsgUtil.h"

// MurataInterface implementation
MurataInterface::MurataInterface( PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm, int baud)
            : _sni(tx, rx, cts, rts, reset, alarm, baud)
{
}

int MurataInterface::connect(
    const char *ssid,
    const char *pass,
    E_SECURITY security)
{
    int ret = 0;

    if (_sni.init()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }

    tagWIFI_STATUS_T p_st;
    ret = _sni.getWifiStatus(&p_st);
    if (!ret)
    {
        int status = p_st.status;
        if (status == e_STATUS_OFF)
        {
            printf("Wifi OFF! Start to turn it on.\n");
            char country_p[2] = {'N', 'C'}; // Country code

            if (_sni.wifi_on( country_p ))
            {
                printf("turn on wifi fail!\n");
                return -1;
            }
            else
            {
                printf("turn on Wifi successfully!\n");
            }
        }
        else if (status == e_NO_NETWORK)
        {
            printf("Wifi is open, but no network. start to join AP\n");
        }
        else if (status == e_STA_JOINED)
        {
            printf("The module has been connected to AP. Start to disconnect before connect again.\n");
            if(_sni.disconnect()) {
                printf("Wifi disconnect failed!\n");
                return -1;
            }
            else
                printf("wifi disconnect successfully\n");
        }
        else if (status == e_AP_STARTED)
        {
            printf("Wifi is on, module in AP mode.\n");
        }
        else
        {
            printf("Wifi status unknown, please check it!\n");
            return -1;
        }
    }
    else
    { 
        printf("getWifi status failed!\n"); 
        return -1;
    }
    
    int ssid_len = 6;
    int pass_len = 12;
    
    if (_sni.connect(ssid,ssid_len, security, pass, pass_len)) 
    {
        return NSAPI_ERROR_NO_CONNECTION;
     }
                 
    if (_sni.setIPConfig(true)) {
        return NSAPI_ERROR_DHCP_FAILURE;
    }
    //else
//        printf("wifi setIPConfig successfully\n");

    return 0;
}

int MurataInterface::disconnect()
{
    if (_sni.disconnect()) {
        printf("disconnect failed \n");
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    else
        printf("disconnect successful\n");

    return 0;
}

const char *MurataInterface::get_ip_address()
{
    return _sni.getIPAddress();
}

const char *MurataInterface::get_mac_address()
{
    return _sni.getMACAddress();
}

struct murata_socket {
    int id;
    nsapi_protocol_t proto;
    bool connected;
};

int MurataInterface::socket_open(void **handle, nsapi_protocol_t proto)
{
    struct murata_socket *socket = new struct murata_socket;
    if (!socket) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    int id = 0;
   
    if (proto == NSAPI_TCP)
    {
        id = _sni.createSocket();
        //printf("Create TCP socket : %d.\n", id);
    }
    else
    {
        id = _sni.createUDPSocket();
        //printf("Create UDP socket : %d.\n", id);
    }
    
    socket->id = id;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return 0;    
}

int MurataInterface::socket_close(void *handle)
{
    struct murata_socket *socket = (struct murata_socket *)handle;
    int err = 0;
      
    if (_sni.close(socket->id)) {
        err = NSAPI_ERROR_DEVICE_ERROR;
    }

    delete socket;
    return err;
}

int MurataInterface::socket_bind(void *handle, const SocketAddress &address)
{
//    return NSAPI_ERROR_UNSUPPORTED;
    return 0;
}

int MurataInterface::socket_listen(void *handle, int backlog)
{
    return 0;
//    return NSAPI_ERROR_UNSUPPORTED;
}

int MurataInterface::socket_connect(void *handle, const SocketAddress &addr)
{
    struct murata_socket *socket = (struct murata_socket *)handle;
    int ret = 0;

    if (socket->proto == NSAPI_UDP)
    {
        ret = _sni.bind(socket->id, addr.get_ip_address(), addr.get_port());
    
        if (ret)
        {
            printf("UDP connect failed, return %d\n", ret);
            return -1;
        }
    }
    else if (socket->proto == NSAPI_TCP)
    {
        //printf("TCP socket id is : %d.\n", socket->id);
        ret = _sni.connectTCP(socket->id, addr.get_ip_address(), addr.get_port());
    
        if (ret)
        {
            printf("TCP connect failed, return %d\n", ret);
            return -1;
        }
    }
    else
    {
        printf("Uknown socket proto\n");
        return -1;
    }
 
    socket->connected = true;
    return 0;
}
    
int MurataInterface::socket_accept(void **handle, void *server)
{
    return 0;
//    return NSAPI_ERROR_UNSUPPORTED;
}

int MurataInterface::socket_send(void *handle, const void *data, unsigned size)
{
    struct murata_socket *socket = (struct murata_socket *)handle;
    printf("start to send data from TCP socket : %d.\n", socket->id);
    return _sni.send(socket->id, data, size);
}

int MurataInterface::socket_recv(void *handle, void *data, unsigned size)
{
    struct murata_socket *socket = (struct murata_socket *)handle;
    
    int32_t recv = _sni.receive(socket->id, (char*)data, size);
    if (recv < 0) {
        return NSAPI_ERROR_WOULD_BLOCK;
    }
 
    return recv;
}

int MurataInterface::socket_sendto(void *handle, const SocketAddress &addr, const void *data, unsigned size)
{
    struct murata_socket *socket = (struct murata_socket *)handle;
    if (!socket->connected) {
        int err = socket_connect(socket, addr);
        if (err < 0) {
            return err;
        }
    }
    
    return _sni.sendTo(socket->id, addr, data, size);
}

int MurataInterface::socket_recvfrom(void *handle, SocketAddress *addr, void *data, unsigned size)
{
    struct murata_socket *socket = (struct murata_socket *)handle;    
    return _sni.receiveFrom(socket->id, addr, (char*)data, size);
}

void MurataInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
}

int MurataInterface::socket_resolveHostName( const char *host_p )
{
    return _sni.resolveHostName(host_p);
}