#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <windows.h>

namespace screensdk {

// Forward declarations
class Exception;

/**
 * @brief Error type enumeration for all error conditions
 *
 * Maps to error_type in FR-006: Error handling and recovery
 */
enum class ErrorType {
  kNetworkError = 1000,
  kEncodingError = 2000,
  kDecodingError = 3000,
  kInputError = 4000,
  kCaptureError = 5000,
  kBrowserIncompatibility = 6000,
  kHardwareUnavailable = 7000,
  kUnknownError = 9999
};

/**
 * @brief Error details
 */
struct ErrorDetail {
  ErrorType type;
  int code;
  std::string message;

  ErrorDetail(ErrorType t, int c, std::string_view msg)
      : type(t), code(c), message(msg) {}
};

/**
 * @brief Result type for operations that can fail
 *
 * Contains either a value of type T or an error detail.
 * Usage:
 *   Result<int> divide(int a, int b) {
 *     if (b == 0) {
 *       return Result<int>::make_error(ErrorType::kUnknownError, 1, "Division by zero");
 *     }
 *     return a / b;
 *   }
 */
template<typename T>
class Result {
public:
  static Result make_ok(T value) {
    return Result(std::move(value));
  }

  static Result make_error(ErrorType type, int code, std::string_view message) {
    return Result(ErrorDetail(type, code, message));
  }

  explicit operator bool() const noexcept { return has_value(); }

  bool has_value() const noexcept { return data_.index() == 0; }

  bool has_error() const noexcept { return data_.index() == 1; }

  const T& value() const& { return std::get<0>(data_); }

  T& value() & { return std::get<0>(data_); }

  T&& value() && { return std::move(std::get<0>(data_)); }

  const ErrorDetail& error() const& { return std::get<1>(data_); }

  ErrorDetail& error() & { return std::get<1>(data_); }

  ErrorDetail&& error() && { return std::move(std::get<1>(data_)); }

private:
  explicit Result(T value) : data_(std::move(value)) {}
  explicit Result(ErrorDetail error) : data_(std::move(error)) {}

  std::variant<T, ErrorDetail> data_;
};

// Specialization for void
template<>
class Result<void> {
public:
  static Result make_ok() { return Result(Ok{}); }

  static Result make_error(ErrorType type, int code, std::string_view message) {
    return Result(ErrorDetail(type, code, message));
  }

  explicit operator bool() const noexcept { return has_value(); }

  bool has_value() const noexcept { return data_.index() == 0; }

  bool has_error() const noexcept { return data_.index() == 1; }

  void value() const {
    if (has_error()) {
      std::get<1>(data_);
    }
  }

  const ErrorDetail& error() const& { return std::get<1>(data_); }

  ErrorDetail& error() & { return std::get<1>(data_); }

  ErrorDetail&& error() && { return std::move(std::get<1>(data_)); }

private:
  struct Ok {};
  explicit Result(Ok) : data_(Ok{}) {}
  explicit Result(ErrorDetail error) : data_(std::move(error)) {}

  std::variant<Ok, ErrorDetail> data_;
};

/**
 * @brief Error code definitions with human-readable messages
 */
class ErrorCode {
public:
  /**
   * @brief Get error message for a specific error code
   */
  static std::string getMessage(ErrorType type, int code);

  /**
   * @brief Get error type from system error code
   */
  static ErrorType fromSystemError(HRESULT hr);

  /**
   * @brief Get error type from Winsock error code
   */
  static ErrorType fromNetworkError(int error);
};

/**
 * @brief Base exception class for all SDK exceptions
 * 
 * Thrown when non-recoverable errors occur.
 * Recoverable errors should be returned via Result<T>.
 */
class Exception : public std::exception {
public:
  explicit Exception(ErrorType type, int code,
                  std::string_view message) noexcept;

  explicit Exception(std::string_view message) noexcept;

  const char* what() const noexcept override;

  ErrorType errorType() const noexcept { return type_; }
  int errorCode() const noexcept { return code_; }

private:
  ErrorType type_;
  int code_;
  std::string message_;
};

/**
 * @brief Network-related exceptions
 */
class NetworkException : public Exception {
public:
  explicit NetworkException(int code, std::string_view message);
};

/**
 * @brief Encoding-related exceptions
 */
class EncodingException : public Exception {
public:
  explicit EncodingException(int code, std::string_view message);
};

/**
 * @brief Decoding-related exceptions
 */
class DecodingException : public Exception {
public:
  explicit DecodingException(int code, std::string_view message);
};

/**
 * @brief Input-related exceptions
 */
class InputException : public Exception {
public:
  explicit InputException(int code, std::string_view message);
};

/**
 * @brief Screen capture-related exceptions
 */
class CaptureException : public Exception {
public:
  explicit CaptureException(int code, std::string_view message);
};

/**
 * @brief Hardware availability exceptions
 */
class HardwareException : public Exception {
public:
  explicit HardwareException(int code, std::string_view message);
};

} // namespace screensdk

// Macro for throwing exceptions with context
#define THROW_ERROR(type, code, msg) \
  throw screensdk::Exception(screensdk::ErrorType::k##type, code, msg)

#define THROW_NETWORK_ERROR(code, msg) \
  throw screensdk::NetworkException(code, msg)

#define THROW_ENCODING_ERROR(code, msg) \
  throw screensdk::EncodingException(code, msg)

#define THROW_CAPTURE_ERROR(code, msg) \
  throw screensdk::CaptureException(code, msg)

#define THROW_HARDWARE_ERROR(code, msg) \
  throw screensdk::HardwareException(code, msg)
