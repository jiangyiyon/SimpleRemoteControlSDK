// Simple test to verify DisplayDetector DXGI implementation
#include <iostream>
#include "screensdk/capture/display_detector.h"

using namespace screensdk;

int main() {
  DisplayDetector detector;

  // Test 1: GetDisplaySourcesReturnsNonEmpty
  auto sources = detector.getDisplaySources();
  std::cout << "Test 1 - GetDisplaySourcesReturnsNonEmpty: "
            << (sources.empty() ? "FAILED" : "PASSED") << std::endl;
  std::cout << "  Found " << sources.size() << " display sources" << std::endl;

  if (sources.empty()) {
    std::cerr << "ERROR: No display sources found!" << std::endl;
    return 1;
  }

  // Test 2: DisplaySourceIdInRange
  bool id_in_range = true;
  for (const auto& source : sources) {
    if (source.id < 0 || source.id > 3) {
      id_in_range = false;
      std::cerr << "ERROR: Display source ID " << source.id << " out of range!" << std::endl;
    }
  }
  std::cout << "Test 2 - DisplaySourceIdInRange: "
            << (id_in_range ? "PASSED" : "FAILED") << std::endl;

  // Test 3: DisplaySourceHasValidResolution
  bool valid_resolution = true;
  for (const auto& source : sources) {
    if (source.resolution_width <= 0 || source.resolution_height <= 0) {
      valid_resolution = false;
      std::cerr << "ERROR: Invalid resolution: " << source.resolution_width << "x" << source.resolution_height << std::endl;
    }
  }
  std::cout << "Test 3 - DisplaySourceHasValidResolution: "
            << (valid_resolution ? "PASSED" : "FAILED") << std::endl;

  // Test 4: DisplaySourceRefreshRateInRange
  bool valid_refresh_rate = true;
  for (const auto& source : sources) {
    if (source.refresh_rate < 30 || source.refresh_rate > 240) {
      valid_refresh_rate = false;
      std::cerr << "ERROR: Invalid refresh rate: " << source.refresh_rate << " Hz" << std::endl;
    }
  }
  std::cout << "Test 4 - DisplaySourceRefreshRateInRange: "
            << (valid_refresh_rate ? "PASSED" : "FAILED") << std::endl;

  // Test 5: ExactlyOnePrimaryDisplay
  int primary_count = 0;
  for (const auto& source : sources) {
    if (source.is_primary) {
      primary_count++;
      std::cout << "  Primary display: " << source.name << " (ID: " << source.id << ")" << std::endl;
    }
  }
  bool exactly_one_primary = (primary_count == 1);
  std::cout << "Test 5 - ExactlyOnePrimaryDisplay: "
            << (exactly_one_primary ? "PASSED" : "FAILED") << std::endl;

  // Test 6: DisplaySourceNameNotEmpty
  bool name_not_empty = true;
  for (const auto& source : sources) {
    if (source.name.empty()) {
      name_not_empty = false;
      std::cerr << "ERROR: Display source name is empty!" << std::endl;
    }
  }
  std::cout << "Test 6 - DisplaySourceNameNotEmpty: "
            << (name_not_empty ? "PASSED" : "FAILED") << std::endl;

  // Test 7: DisplaySourceAtomicActiveState
  bool atomic_active = true;
  for (const auto& source : sources) {
    bool active = source.is_active;
    if (!active) {
      atomic_active = false;
      std::cerr << "ERROR: Display source is not active!" << std::endl;
    }
  }
  std::cout << "Test 7 - DisplaySourceAtomicActiveState: "
            << (atomic_active ? "PASSED" : "FAILED") << std::endl;

  // Test 8: GetDisplaySourceByIdValid
  int valid_id = sources[0].id;
  auto source_by_id = detector.getDisplaySource(valid_id);
  bool get_by_id_valid = (source_by_id.id == valid_id && !source_by_id.name.empty());
  std::cout << "Test 8 - GetDisplaySourceByIdValid: "
            << (get_by_id_valid ? "PASSED" : "FAILED") << std::endl;

  // Test 9: GetDisplaySourceByIdInvalid
  auto invalid_source = detector.getDisplaySource(999);
  bool get_by_id_invalid = (invalid_source.id == 0 && invalid_source.name.empty());
  std::cout << "Test 9 - GetDisplaySourceByIdInvalid: "
            << (get_by_id_invalid ? "PASSED" : "FAILED") << std::endl;

  // Test 10: DisplaySourceCountMatchesGetSize
  int count = detector.getDisplaySourceCount();
  bool count_matches = (count == static_cast<int>(sources.size()));
  std::cout << "Test 10 - DisplaySourceCountMatchesGetSize: "
            << (count_matches ? "PASSED" : "FAILED") << std::endl;

  // Summary
  std::cout << "\n========================================" << std::endl;
  std::cout << "Test Summary:" << std::endl;
  std::cout << "========================================" << std::endl;

  int passed = 0;
  int total = 10;

  if (!sources.empty()) passed++;
  if (id_in_range) passed++;
  if (valid_resolution) passed++;
  if (valid_refresh_rate) passed++;
  if (exactly_one_primary) passed++;
  if (name_not_empty) passed++;
  if (atomic_active) passed++;
  if (get_by_id_valid) passed++;
  if (get_by_id_invalid) passed++;
  if (count_matches) passed++;

  std::cout << "Passed: " << passed << "/" << total << std::endl;
  std::cout << "Failed: " << (total - passed) << "/" << total << std::endl;

  return (passed == total) ? 0 : 1;
}
