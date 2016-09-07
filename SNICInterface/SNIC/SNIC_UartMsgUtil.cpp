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
#include "SNIC_UartMsgUtil.h"

C_SNIC_UartMsgUtil::C_SNIC_UartMsgUtil()
{
}

C_SNIC_UartMsgUtil::~C_SNIC_UartMsgUtil()
{
}

unsigned int C_SNIC_UartMsgUtil::makeRequest( unsigned char cmd_id,unsigned char *payload_p
                                , unsigned short payload_len, unsigned char *uart_command_p )
{
    unsigned char check_sum = 0;    // Check Sum
    unsigned int  uart_cmd_len = 0;
    int i;
    
    // set SOM
    *uart_command_p = UART_CMD_SOM;
    uart_command_p++;
    uart_cmd_len++;
    
    // set payload length L0
    *uart_command_p = (0x80 | (payload_len & 0x007f));
    check_sum += *uart_command_p;
    uart_command_p++;
    uart_cmd_len++;

    // set payload length L1
    *uart_command_p = (0x80 | ( (payload_len >> 7) & 0x003f));
    check_sum += *uart_command_p;
    uart_command_p++;
    uart_cmd_len++;
    
    // set Command ID
    *uart_command_p = (0x80 | cmd_id);
    check_sum += *uart_command_p;
    uart_command_p++;
    uart_cmd_len++;

    // set Payload
    for( i = 0; i < payload_len; i++, uart_command_p++, uart_cmd_len++ )
    {
        *uart_command_p = payload_p[i];
    }

    // set Check sum
    *uart_command_p = (0x80 | check_sum);
    uart_command_p++;
    uart_cmd_len++;
    
    // set EOM
    *uart_command_p = UART_CMD_EOM;
    uart_cmd_len++;
    
    return uart_cmd_len;
}

unsigned int C_SNIC_UartMsgUtil::getResponsePayload( unsigned int recvdata_len, unsigned char *recvdata_p
                                            , unsigned char *command_id_p,  unsigned char *payload_p )
{
    unsigned short payload_len  = 0;
    unsigned char *buf_p = NULL;
    int i;
    
    // get payload length
    payload_len = ( ( (recvdata_p[1] & ~0x80) & 0xff) | ( ( (recvdata_p[2] & ~0xC0) << 7) & 0xff80) );

    // get Command ID
    *command_id_p = (recvdata_p[3] & ~0x80);

    buf_p = &recvdata_p[4];

    // get payload data
    for( i = 0; i < payload_len; i++, buf_p++ )
    {
        *payload_p = *buf_p;
        payload_p++;
    }

    return payload_len;
}

int C_SNIC_UartMsgUtil::addrToInteger( const char *addr_p )
{
    if( addr_p == NULL )
    {
        DEBUG_PRINT("addrToInteger parameter error\r\n");
        return 0;
    }
    
    unsigned char ipadr[4];
    unsigned char temp= 0;
    int i,j,k;
    
    /* convert to char[4] */
    k=0;
    for(i=0; i<4; i ++)
    {
        for(j=0; j<4; j ++)
        {
            if((addr_p[k] > 0x2F)&&(addr_p[k] < 0x3A))
            {
                temp = (temp * 10) + addr_p[k]-0x30;
            }
            else if((addr_p[k] == 0x20)&&(temp == 0))
            {
            }
            else
            {
                ipadr[i]=temp;
                temp = 0;
                k++;
                break;
            }
            k++;
        }
    }

    int  addr  = ( (ipadr[0]<<24) | (ipadr[1]<<16) | (ipadr[2]<<8) | ipadr[3] );

    return addr;
}

void C_SNIC_UartMsgUtil::convertIntToByteAdday( int addr, char *addr_array_p )
{
    addr_array_p[0] = ((addr & 0xFF000000) >> 24 );
    addr_array_p[1] = ((addr & 0xFF0000) >> 16 );
    addr_array_p[2] = ((addr & 0xFF00) >> 8 );
    addr_array_p[3] = ( addr & 0xFF);
}
