#include "screensdk/platform/gpu_detector.h"

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <memory>
#include <string>

namespace screensdk {

class GpuDetectorImpl : public IGpuDetector {
public:
  GpuDetectorImpl();
  ~GpuDetectorImpl() override;

  GpuDetectorImpl(const GpuDetectorImpl&) = delete;
  GpuDetectorImpl& operator=(const GpuDetectorImpl&) = delete;
  GpuDetectorImpl(GpuDetectorImpl&&) = delete;
  GpuDetectorImpl& operator=(GpuDetectorImpl&&) = delete;

  bool isNvenconline() override;
  bool isQuickSyncAvailable() override;
  bool hasHardwareEncoder() override;
  std::string getGpuInfo() override;
  EncoderType getBestEncoderType() override;
  std::string getEncoderTypeName(EncoderType type) override;

private:
  bool detectGpuInfo();
  
  bool nvenc_available_;
  bool qsv_available_;
  std::string gpu_info_;
  bool info_cached_;
};

GpuDetectorImpl::GpuDetectorImpl()
    : nvenc_available_(false),
      qsv_available_(false),
      info_cached_(false) {
  detectGpuInfo();
}

GpuDetectorImpl::~GpuDetectorImpl() = default;

bool GpuDetectorImpl::detectGpuInfo() {
  if (info_cached_) {
    return true;
  }

  HRESULT hr;
  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;

  hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), &factory);
  if (FAILED(hr)) {
    gpu_info_ = "Failed to create DXGI factory";
    info_cached_ = true;
    return false;
  }

  hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                            __uuidof(IDXGIAdapter4), &adapter);
  if (FAILED(hr)) {
    gpu_info_ = "Failed to enumerate GPU adapter";
    info_cached_ = true;
    return false;
  }

  DXGI_ADAPTER_DESC3 desc;
  hr = adapter->GetDesc3(&desc);
  if (FAILED(hr)) {
    gpu_info_ = "Failed to get GPU description";
    info_cached_ = true;
    return false;
  }

  // Convert description to narrow string
  int length = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                                    nullptr, 0, nullptr, nullptr);
  if (length > 0) {
    std::string desc_str(length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                         &desc_str[0], length, nullptr, nullptr);
    gpu_info_ = desc_str;
  } else {
    gpu_info_ = "Unknown GPU";
  }

  // Check for NVIDIA GPU (Vendor ID: 0x10DE)
  if (desc.VendorId == 0x10DE ||
      wcsstr(desc.Description, L"NVIDIA") != nullptr) {
    nvenc_available_ = true;
    gpu_info_ += " (NVIDIA)";
  }

  // Check for Intel GPU (Vendor ID: 0x8086)
  if (desc.VendorId == 0x8086 ||
      wcsstr(desc.Description, L"Intel") != nullptr) {
    qsv_available_ = true;
    gpu_info_ += " (Intel)";
  }

  // Check for AMD GPU (Vendor ID: 0x1002)
  if (desc.VendorId == 0x1002 ||
      wcsstr(desc.Description, L"AMD") != nullptr ||
      wcsstr(desc.Description, L"ATI") != nullptr) {
    gpu_info_ += " (AMD)";
  }

  info_cached_ = true;
  return true;
}

bool GpuDetectorImpl::isNvenconline() {
  detectGpuInfo();
  return nvenc_available_;
}

bool GpuDetectorImpl::isQuickSyncAvailable() {
  detectGpuInfo();
  return qsv_available_;
}

bool GpuDetectorImpl::hasHardwareEncoder() {
  detectGpuInfo();
  return nvenc_available_ || qsv_available_;
}

std::string GpuDetectorImpl::getGpuInfo() {
  detectGpuInfo();
  return gpu_info_;
}

EncoderType GpuDetectorImpl::getBestEncoderType() {
  detectGpuInfo();

  // Prefer NVENC over QuickSync over software
  if (nvenc_available_) {
    return EncoderType::kHardwareNVENC;
  } else if (qsv_available_) {
    return EncoderType::kHardwareQuickSync;
  } else {
    return EncoderType::kSoftwareX264;
  }
}

std::string GpuDetectorImpl::getEncoderTypeName(EncoderType type) {
  switch (type) {
    case EncoderType::kHardwareNVENC:
      return "NVENC (NVIDIA)";
    case EncoderType::kHardwareQuickSync:
      return "QuickSync (Intel)";
    case EncoderType::kSoftwareX264:
      return "x264 Software";
    default:
      return "Unknown";
  }
}

extern "C" SCREEN_STREAM_SDK_EXPORT IGpuDetector* CreateGpuDetector() {
  return new GpuDetectorImpl();
}

extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyGpuDetector(IGpuDetector* detector) {
  if (detector != nullptr) {
    delete detector;
  }
}

} // namespace screensdk
