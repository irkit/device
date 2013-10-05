
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

#ifndef HOST_H
#define HOST_H

#include "ipaddr.h"
#include <string.h>

///Host information container
/**
This class is a container for data relative to a connection:
- IP Address
- Port number
- Host Name
*/
class Host 
{
public:
  ///Initiliazes host with null values
  Host() : m_ip(0,0,0,0), m_port(0), m_name(NULL)
  {
    
  }
  
  ///Initializes host
  Host(const IpAddr& ip, const int& port, const char* name="" ) : m_ip(ip), m_port(port), m_name(NULL)
  {
    setName(name); 
  }
  
  ~Host()
  {
    if(m_name)
    {
      delete[] m_name;
    }
  }
  
  ///Returns IP address
  const IpAddr& getIp() const
  {
    return m_ip;
  }
  
  ///Returns port number
  const int& getPort() const
  {
    return m_port;
  }
  
  ///Returns host name
  const char* getName() const
  {
    return m_name;
  }
  
  ///Sets IP address
  void setIp(const IpAddr& ip)
  {
    m_ip = ip;
  }
  
  ///Sets port number
  void setPort(int port)
  {
    m_port = port;
  }
  
  ///Sets host name
  void setName(const char* name)
  {
    if(m_name)
      delete[] m_name;
    int len = strlen(name);
    if(len)
    {
      m_name = new char[len+1];
      strcpy(m_name, name);
    }
  }
  
private:
  IpAddr m_ip;
  int m_port;
  char* m_name;
};

#endif
