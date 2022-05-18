#ifndef __ERROR_RESULT_H__
#define __ERROR_RESULT_H__

#include <string>

class ErrorResult {
public:
    ErrorResult();
    ErrorResult(const std::string& message);

    bool HasError() const;
    const std::string& GetMessage() const;
private:
    std::string mMessage;
};

#endif