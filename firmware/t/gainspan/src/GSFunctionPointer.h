/* mbed Microcontroller Library
 * Copyright (c) 2006-2012 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * Modifyed for the GSwifi library, by 2013 gsfan
 */
#ifndef GS_FUNCTIONPOINTER_H
#define GS_FUNCTIONPOINTER_H

#include <string.h>

/** A class for storing and calling a pointer to a static or member void function
 */
class GSFunctionPointer {
public:

    /** Create a FunctionPointer, attaching a static function
     *
     *  @param function The void static function to attach (default is none)
     */
    GSFunctionPointer(void (*function)(int, int) = 0) {
        attach(function);
    }

    /** Create a FunctionPointer, attaching a member function
     *
     *  @param object The object pointer to invoke the member function on (i.e. the this pointer)
     *  @param function The address of the void member function to attach
     */
    template<typename T>
    GSFunctionPointer(T *object, void (T::*member)(int, int)) {
        attach(object, member);
    }

    /** Attach a static function
     *
     *  @param function The void static function to attach (default is none)
     */
    void attach(void (*function)(int, int) = 0) {
        _function = function;
        _object = 0;
    }

    /** Attach a member function
     *
     *  @param object The object pointer to invoke the member function on (i.e. the this pointer)
     *  @param function The address of the void member function to attach
     */
    template<typename T>
    void attach(T *object, void (T::*member)(int, int)) {
        _object = static_cast<void*>(object);
        memcpy(_member, (char*)&member, sizeof(member));
        _membercaller = &GSFunctionPointer::membercaller<T>;
        _function = NULL;
    }

    /** Call the attached static or member function
     */
    int call(int a, int b) {
        if (_function) {
            _function(a, b);
            return 0;
        } else
        if (_object) {
            _membercaller(_object, _member, a, b);
            return 0;
        }
        return -1;
    }

    void detach() {
        _function = NULL;
        _object = 0;
    }

    GSFunctionPointer &operator= (GSFunctionPointer &gsfp) {
        _function = gsfp._function;
        _object = gsfp._object;
        memcpy(_member, gsfp._member, sizeof(_member));
        _membercaller = gsfp._membercaller;
        return *this;
    }

private:
    template<typename T>
    static void membercaller(void *object, char *member, int a, int b) {
        T* o = static_cast<T*>(object);
        void (T::*m)(int, int);
        memcpy((char*)&m, member, sizeof(m));
        (o->*m)(a, b);
    }

    void (*_function)(int, int);                // static function pointer - 0 if none attached
    void *_object;                            // object this pointer - 0 if none attached
    char _member[16];                        // raw member function pointer storage - converted back by registered _membercaller
    void (*_membercaller)(void*, char*, int, int);    // registered membercaller function to convert back and call _member on _object
};

#endif
