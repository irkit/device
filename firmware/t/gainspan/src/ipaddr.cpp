
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
 * code from http://mbed.org/cookbook/NetServices
 */

#include "ipaddr.h"

//#include "netCfg.h"
#if NET_LWIP_STACK
#include "lwip/ip_addr.h"
#endif


#if NET_LWIP_STACK
IpAddr::IpAddr(ip_addr_t* pIp)
{
  *((uint32_t*)m_ip) = pIp->addr;
}
#endif

///Initializes IP address with provided values
IpAddr::IpAddr(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
{
  //We are in LE
  m_ip[0] = ip0;
  m_ip[1] = ip1;
  m_ip[2] = ip2;
  m_ip[3] = ip3;
}

///Initializes IP address with null values 
IpAddr::IpAddr()
{
  m_ip[0] = 0;
  m_ip[1] = 0;
  m_ip[2] = 0;
  m_ip[3] = 0;
}


#if NET_LWIP_STACK
ip_addr_t IpAddr::getStruct() const
{
  ip_addr_t ip_struct;
  ip_struct.addr = *((uint32_t*)m_ip);
  return ip_struct;
}
#endif

uint8_t IpAddr::operator[](unsigned int i) const
{
  uint8_t null = 0;
  if( i > 3 )
    return null;
  return m_ip[i];
}

bool IpAddr::isEq(const IpAddr& b) const
{
  return (*((uint32_t*)m_ip) == *((uint32_t*)(b.m_ip)));
}

bool IpAddr::operator==(const IpAddr& b) const
{
  return isEq(b);
}
 
bool IpAddr::operator!=(const IpAddr& b) const
{
  return !(operator==(b));
}

bool IpAddr::isNull() const
{
  return (*((uint32_t*)m_ip) == 0);
}

bool IpAddr::isBroadcast() const
{
  return (*((uint32_t*)m_ip) == 0xFFFFFFFF);
}

bool IpAddr::isMulticast() const
{
  return ((m_ip[0] & 0xF0) == 0xE0);
}
