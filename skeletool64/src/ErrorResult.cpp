#include "ErrorResult.h"

ErrorResult::ErrorResult() : mMessage("") {}

ErrorResult::ErrorResult(const std::string& message) : mMessage(message) {}

bool ErrorResult::HasError() const {
    return mMessage.length() > 0;
}

const std::string& ErrorResult::GetMessage() const {
    return mMessage;
}