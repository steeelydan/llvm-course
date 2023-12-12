#ifndef Logger_h
#define Logger_h

#include <iostream>
#include <sstream>

class ErrorLogMessage : public std::basic_ostringstream<char>
{
public:
    ~ErrorLogMessage()
    {
        std::cerr << "Fatal Error: " << str().c_str();
        exit(EXIT_FAILURE);
    }
};

#define DIE ErrorLogMessage()

#endif