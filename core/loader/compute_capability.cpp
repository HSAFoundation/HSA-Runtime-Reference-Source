#include "compute_capability.hpp"

#include <cstdint>
#include <ostream>

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

using core::loader::ComputeCapability;
using core::loader::ComputeProperties;

//===----------------------------------------------------------------------===//
// CapabilityMapping Initialization.                                          //
//===----------------------------------------------------------------------===//

struct ComputeCapabilityComparison final {
  bool operator()(
    const ComputeCapability &in_lhs, const ComputeCapability &in_rhs
  ) const {
    return in_lhs.version_major() == in_rhs.version_major() &&
             in_lhs.version_minor() == in_rhs.version_minor() &&
               in_lhs.version_stepping() == in_rhs.version_stepping();
  }
}; // struct ComputeCapabilityComparison

struct ComputeCapabilityHash final {
  std::size_t operator()(const ComputeCapability &in_cc) const {
    // FIXME: better hashing function?
    std::ostringstream cc_stream; cc_stream << in_cc;
    return std::hash<std::string>()(cc_stream.str());
  }
}; // struct ComputeCapabilityHash

typedef std::unordered_map<
  ComputeCapability,          // Key.
  ComputeProperties,          // Value.
  ComputeCapabilityHash,      // Hash function.
  ComputeCapabilityComparison // Comparison function.
> CapabilityMap;

struct CapabilityMapping final {
  static CapabilityMap Initialize() {
    CapabilityMap contents;

    // NOTE: All compute capabilities and compute properties must be added below
    // this line.
    // -------------------------------------------------------------------------

    contents[ComputeCapability(7, 0, 0)] = ComputeProperties();
    contents[ComputeCapability(8, 0, 1)] = ComputeProperties();

    return contents;
  }

  static const CapabilityMap contents_;
}; // struct CapabilityMapping

const CapabilityMap CapabilityMapping::contents_ =
  CapabilityMapping::Initialize();

//===----------------------------------------------------------------------===//
// DeviceMapping Initialization.                                              //
//===----------------------------------------------------------------------===//

typedef std::unordered_map<uint64_t, ComputeCapability> DeviceMap;

struct DeviceMapping final {
  static DeviceMap Initialize() {
    DeviceMap contents;

    contents[0] = ComputeCapability(7, 0, 0);

    return contents;
  }

  static const DeviceMap contents_;
}; // struct DeviceMapping

const DeviceMap DeviceMapping::contents_ = DeviceMapping::Initialize();

} // namespace anonymous

namespace core {
namespace loader {

//===----------------------------------------------------------------------===//
// ComputeProperties.                                                         //
//===----------------------------------------------------------------------===//

void ComputeProperties::Initialize() {
  is_initialized_ = true;
} // ComputeProperties::Initialize

void ComputeProperties::Reset() {
  is_initialized_ = false;
} // ComputeProperties::Reset

//===----------------------------------------------------------------------===//
// ComputeCapability.                                                         //
//===----------------------------------------------------------------------===//

void ComputeCapability::Initialize(
  const int32_t &in_version_major,
  const int32_t &in_version_minor,
  const int32_t &in_version_stepping
) {
  version_major_ = in_version_major;
  version_minor_ = in_version_minor;
  version_stepping_ = in_version_stepping;
} // ComputeCapability::Initialize

void ComputeCapability::Initialize(const uint32_t &in_device_id) {
  auto compute_capability = DeviceMapping::contents_.find(0);
  if (compute_capability != DeviceMapping::contents_.end()) {
    version_major_ = compute_capability->second.version_major_;
    version_minor_ = compute_capability->second.version_minor_;
    version_stepping_ = compute_capability->second.version_stepping_;
  }
} // ComputeCapability::Initialize

void ComputeCapability::Reset() {
  version_major_ = COMPUTE_CAPABILITY_VERSION_MAJOR_UNDEFINED;
  version_minor_ = COMPUTE_CAPABILITY_VERSION_MINOR_UNDEFINED;
  version_stepping_ = COMPUTE_CAPABILITY_VERSION_STEPPING_UNDEFINED;
  compute_properties_.Reset();
} // ComputeCapability::Reset

bool ComputeCapability::IsValid() {
  if (compute_properties_.is_initialized()) {
    return true;
  }

  auto compute_properties = CapabilityMapping::contents_.find(*this);
  if (compute_properties != CapabilityMapping::contents_.end()) {
    compute_properties_.Initialize();
    return true;
  }

  return false;
} // ComputeCapability::IsValid

std::ostream& operator<<(
  std::ostream &out_stream,
  const ComputeCapability &in_compute_capability
) {
  return out_stream <<
    in_compute_capability.version_major_ << ":" <<
    in_compute_capability.version_minor_ << ":" <<
    in_compute_capability.version_stepping_;
} // ostream<<ComputeCapability

} // namespace loader
} // namespace core
