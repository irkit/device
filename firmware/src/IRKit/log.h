/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __LOG_H__
#define __LOG_H__

#include "Arduino.h"

#define MAINLOG
#define GSLOG
#define HTTPLOG
#define IRLOG
// #define KEYLOG
// #define MOLOG

#ifdef MAINLOG
# define MAINLOG_PRINTLN(a)    Serial.println(a)
# define MAINLOG_PRINTLN2(a,b) Serial.println(a,b)
# define MAINLOG_PRINT(a)      Serial.print(a)
# define MAINLOG_PRINT2(a,b)   Serial.print(a,b)
# define MAINLOG_WRITE(a)      Serial.write(a)
#else
# define MAINLOG_PRINTLN(a)    
# define MAINLOG_PRINTLN2(a,b) 
# define MAINLOG_PRINT(a)      
# define MAINLOG_PRINT2(a,b)   
# define MAINLOG_WRITE(a)      
#endif

#ifdef GSLOG
# define GSLOG_PRINTLN(a)    Serial.println(a)
# define GSLOG_PRINTLN2(a,b) Serial.println(a,b)
# define GSLOG_PRINT(a)      Serial.print(a)
# define GSLOG_PRINT2(a,b)   Serial.print(a,b)
# define GSLOG_WRITE(a)      Serial.write(a)
#else
# define GSLOG_PRINTLN(a)    
# define GSLOG_PRINTLN2(a,b) 
# define GSLOG_PRINT(a)      
# define GSLOG_PRINT2(a,b)   
# define GSLOG_WRITE(a)      
#endif

#ifdef HTTPLOG
# define HTTPLOG_PRINTLN(a)    Serial.println(a)
# define HTTPLOG_PRINTLN2(a,b) Serial.println(a,b)
# define HTTPLOG_PRINT(a)      Serial.print(a)
# define HTTPLOG_PRINT2(a,b)   Serial.print(a,b)
# define HTTPLOG_WRITE(a)      Serial.write(a)
#else
# define HTTPLOG_PRINTLN(a)    
# define HTTPLOG_PRINTLN2(a,b) 
# define HTTPLOG_PRINT(a)      
# define HTTPLOG_PRINT2(a,b)   
# define HTTPLOG_WRITE(a)      
#endif

#ifdef IRLOG
# define IRLOG_PRINTLN(a)    Serial.println(a)
# define IRLOG_PRINTLN2(a,b) Serial.println(a,b)
# define IRLOG_PRINT(a)      Serial.print(a)
# define IRLOG_PRINT2(a,b)   Serial.print(a,b)
# define IRLOG_WRITE(a)      Serial.write(a)
#else
# define IRLOG_PRINTLN(a)    
# define IRLOG_PRINTLN2(a,b) 
# define IRLOG_PRINT(a)      
# define IRLOG_PRINT2(a,b)   
# define IRLOG_WRITE(a)      
#endif

#ifdef KEYLOG
# define KEYLOG_PRINTLN(a)    Serial.println(a)
# define KEYLOG_PRINTLN2(a,b) Serial.println(a,b)
# define KEYLOG_PRINT(a)      Serial.print(a)
# define KEYLOG_PRINT2(a,b)   Serial.print(a,b)
# define KEYLOG_WRITE(a)      Serial.write(a)
#else
# define KEYLOG_PRINTLN(a)    
# define KEYLOG_PRINTLN2(a,b) 
# define KEYLOG_PRINT(a)      
# define KEYLOG_PRINT2(a,b)   
# define KEYLOG_WRITE(a)      
#endif

#ifdef MOLOG
# define MOLOG_PRINTLN(a)    Serial.println(a)
# define MOLOG_PRINTLN2(a,b) Serial.println(a,b)
# define MOLOG_PRINT(a)      Serial.print(a)
# define MOLOG_PRINT2(a,b)   Serial.print(a,b)
# define MOLOG_WRITE(a)      Serial.write(a)
#else
# define MOLOG_PRINTLN(a)    
# define MOLOG_PRINTLN2(a,b) 
# define MOLOG_PRINT(a)      
# define MOLOG_PRINT2(a,b)   
# define MOLOG_WRITE(a)      
#endif

#endif // __LOG_H__
