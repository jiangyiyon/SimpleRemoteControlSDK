#include <gtest/gtest.h>
#include "screensdk/utils/error.h"

using namespace screensdk;

class ErrorTest : public ::testing::Test {
};

TEST(ErrorTest, ExceptionConstruction) {
  Exception ex(ErrorType::kNetworkError, 1001, "Test error");

  EXPECT_STREQ(ex.what(), "Test error");
  EXPECT_EQ(ex.errorType(), ErrorType::kNetworkError);
  EXPECT_EQ(ex.errorCode(), 1001);
}

TEST(ErrorTest, ExceptionConstructionSimple) {
  Exception ex("Simple error");

  EXPECT_STREQ(ex.what(), "Simple error");
  EXPECT_EQ(ex.errorType(), ErrorType::kUnknownError);
  EXPECT_EQ(ex.errorCode(), 0);
}

TEST(ErrorTest, NetworkException) {
  NetworkException ex(1002, "Network failed");

  EXPECT_EQ(ex.errorType(), ErrorType::kNetworkError);
  EXPECT_EQ(ex.errorCode(), 1002);
  EXPECT_STREQ(ex.what(), "Network failed");
}

TEST(ErrorTest, EncodingException) {
  EncodingException ex(2001, "Encoding failed");

  EXPECT_EQ(ex.errorType(), ErrorType::kEncodingError);
  EXPECT_EQ(ex.errorCode(), 2001);
}

TEST(ErrorTest, CaptureException) {
  CaptureException ex(5001, "Capture failed");

  EXPECT_EQ(ex.errorType(), ErrorType::kCaptureError);
  EXPECT_EQ(ex.errorCode(), 5001);
}

TEST(ErrorTest, HardwareException) {
  HardwareException ex(7001, "Hardware unavailable");

  EXPECT_EQ(ex.errorType(), ErrorType::kHardwareUnavailable);
  EXPECT_EQ(ex.errorCode(), 7001);
}

TEST(ErrorTest, ErrorCodeGetMessage) {
  std::string msg = ErrorCode::getMessage(ErrorType::kNetworkError, 1001);

  EXPECT_FALSE(msg.empty());
  EXPECT_NE(msg.find("Network"), std::string::npos);
  EXPECT_NE(msg.find("1001"), std::string::npos);
}

TEST(ErrorTest, ThrowAndCatchException) {
  EXPECT_THROW({
    throw Exception(ErrorType::kEncodingError, 1, "Test");
  }, Exception);

  try {
    throw Exception(ErrorType::kEncodingError, 1, "Test");
  } catch (const Exception& ex) {
    EXPECT_STREQ(ex.what(), "Test");
  }
}

TEST(ErrorTest, ThrowAndCatchNetworkException) {
  EXPECT_THROW({
    throw NetworkException(1001, "Network error");
  }, NetworkException);

  try {
    throw NetworkException(1001, "Network error");
  } catch (const NetworkException& ex) {
    EXPECT_STREQ(ex.what(), "Network error");
    EXPECT_EQ(ex.errorType(), ErrorType::kNetworkError);
  }
}
