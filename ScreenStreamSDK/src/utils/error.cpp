#include "screensdk/utils/error.h"
#include <sstream>

namespace screensdk {

std::string ErrorCode::getMessage(ErrorType type, int code) {
  std::ostringstream oss;
  oss << "[" << static_cast<int>(type) << ":" << code << "] ";

  switch (type) {
    case ErrorType::kNetworkError:
      oss << "Network error: ";
      break;
    case ErrorType::kEncodingError:
      oss << "Encoding error: ";
      break;
    case ErrorType::kDecodingError:
      oss << "Decoding error: ";
      break;
    case ErrorType::kInputError:
      oss << "Input error: ";
      break;
    case ErrorType::kCaptureError:
      oss << "Screen capture error: ";
      break;
    case ErrorType::kBrowserIncompatibility:
      oss << "Browser incompatibility: ";
      break;
    case ErrorType::kHardwareUnavailable:
      oss << "Hardware unavailable: ";
      break;
    default:
      oss << "Unknown error: ";
      break;
  }

  // Add code-specific messages
  oss << "Code " << code;
  return oss.str();
}

ErrorType ErrorCode::fromSystemError(HRESULT hr) {
  if (FAILED(hr)) {
    switch (hr) {
      case E_NOTIMPL:
      case E_NOINTERFACE:
      case REGDB_E_CLASSNOTREG:
        return ErrorType::kHardwareUnavailable;
      case E_POINTER:
      case E_INVALIDARG:
        return ErrorType::kInputError;
      case E_FAIL:
      case E_OUTOFMEMORY:
        return ErrorType::kUnknownError;
      default:
        return ErrorType::kCaptureError;
    }
  }
  return ErrorType::kUnknownError;
}

ErrorType ErrorCode::fromNetworkError(int error) {
  switch (error) {
    case WSAECONNREFUSED:
    case WSAECONNABORTED:
    case WSAENETDOWN:
      return ErrorType::kNetworkError;
    case WSAETIMEDOUT:
      return ErrorType::kNetworkError;
    default:
      return ErrorType::kNetworkError;
  }
}

Exception::Exception(ErrorType type, int code,
                 std::string_view message) noexcept
    : type_(type), code_(code), message_(message) {}

Exception::Exception(std::string_view message) noexcept
    : type_(ErrorType::kUnknownError), code_(0), message_(message) {}

const char* Exception::what() const noexcept {
  return message_.c_str();
}

NetworkException::NetworkException(int code, std::string_view message)
    : Exception(ErrorType::kNetworkError, code, message) {}

EncodingException::EncodingException(int code, std::string_view message)
    : Exception(ErrorType::kEncodingError, code, message) {}

DecodingException::DecodingException(int code, std::string_view message)
    : Exception(ErrorType::kDecodingError, code, message) {}

InputException::InputException(int code, std::string_view message)
    : Exception(ErrorType::kInputError, code, message) {}

CaptureException::CaptureException(int code, std::string_view message)
    : Exception(ErrorType::kCaptureError, code, message) {}

HardwareException::HardwareException(int code, std::string_view message)
    : Exception(ErrorType::kHardwareUnavailable, code, message) {}

} // namespace screensdk
