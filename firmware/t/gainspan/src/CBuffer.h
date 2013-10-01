/* Copyright (C) 2012 mbed.org, MIT License
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

#ifndef CIRCBUFFER_H_
#define CIRCBUFFER_H_

template <class T>
class CircBuffer {
public:
    CircBuffer(int length) {
        write = 0;
        read = 0;
        size = length + 1;
//        buf = new T[size * sizeof(T)];
        buf = (T *)malloc(size * sizeof(T));
    };

    bool isFull() {
        return (((write + 1) % size) == read);
    };

    bool isEmpty() {
        return (read == write);
    };

    bool queue(T k) {
        if (isFull()) {
//            read++;
//            read %= size;
            return false;
        }
        buf[write++] = k;
        write %= size;
        return true;
    }
    
    void flush() {
        read = 0;
        write = 0;
    }
    

    uint32_t available() {
        return (write >= read) ? write - read : size - read + write;
    };

    bool dequeue(T * c) {
        bool empty = isEmpty();
        if (!empty) {
            *c = buf[read++];
            read %= size;
        }
        return(!empty);
    };

private:
    volatile uint32_t write;
    volatile uint32_t read;
    uint32_t size;
    T * buf;
};

#endif
