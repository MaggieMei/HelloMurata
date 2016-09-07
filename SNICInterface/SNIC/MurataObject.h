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
#ifndef _MURATA_OBJECT_H_
#define _MURATA_OBJECT_H_
#include "mbed.h"

//#define _DEBUG      /* If this definition is enabled, debug log is output. */
//#define _FUNC_TRACE

#ifdef _DEBUG
extern Serial pc;
#define DEBUG_PRINT(...)    { printf(__VA_ARGS__);}

#ifdef _FUNC_TRACE
#define FUNC_IN()             { pc.printf( "%s[%d]%s[tid:%x] IN\r\n", __FILE__, __LINE__, __FUNCTION__, Thread::gettid());}
#define FUNC_OUT()            { pc.printf( "%s[%d]%s[tid:%x] OUT\r\n", __FILE__, __LINE__, __FUNCTION__, Thread::gettid() );}
#else
#define FUNC_IN()
#define FUNC_OUT()
#endif

#else
#define DEBUG_PRINT(...)
#define FUNC_IN()
#define FUNC_OUT()
#endif

class C_MurataObject{
   
};

#endif
