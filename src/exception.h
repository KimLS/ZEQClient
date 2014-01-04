
#ifndef ZEQ_EXCEPTION_H
#define ZEQ_EXCEPTION_H

#include <exception>

class ZEQException : public std::exception
{
public:
	ZEQException(const char* msg) { mMessage = msg; }
	virtual const char* what() const override { return mMessage; }
private:
	const char* mMessage;
};

#endif
