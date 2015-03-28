#include "isa.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include "compute_capability.hpp"
#include "core/inc/hsa_internal.h"

#include <cassert>
#include <cstring>
#include <list>
#include <new>
#include <sstream>
#include <stdexcept>
#include "inc/hsa_ext_amd.h"

namespace {

//===----------------------------------------------------------------------===//
// Isa List.                                                                  //
//===----------------------------------------------------------------------===//

std::list<core::loader::Isa> isa_list;

//===----------------------------------------------------------------------===//
// Utilities.                                                                 //
//===----------------------------------------------------------------------===//

// FIXME: move to common area (string utilities).
bool Tokenize(
  const std::string &in_str,
  const char &in_del,
  std::list<std::string> &out_tokens
) {
  try {
    std::size_t start = 0;
    std::size_t end = 0;
    while ((end = in_str.find(in_del, start)) != std::string::npos) {
      out_tokens.push_back(in_str.substr(start, end - start));
      start = end + 1;
    }
    out_tokens.push_back(in_str.substr(start));
  } catch (const std::bad_alloc) {
    return false;
  } catch (const std::out_of_range) {
    return false;
  }

  return true;
} // Tokenize

} // namespace anonymous

namespace core {
namespace loader {

//===----------------------------------------------------------------------===//
// Isa.                                                                       //
//===----------------------------------------------------------------------===//

hsa_status_t Isa::Create(
  const hsa_agent_t &in_agent,
  hsa_isa_t *out_isa_handle
) {
  assert(out_isa_handle);

  try {
    isa_list.push_back(Isa());
  } catch (const std::bad_alloc) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  hsa_status_t hsa_status_code = isa_list.back().Initialize(in_agent);
  if (HSA_STATUS_SUCCESS != hsa_status_code) {
    isa_list.pop_back();
    return hsa_status_code;
  }

  *out_isa_handle = Isa::Handle(&isa_list.back());
  return HSA_STATUS_SUCCESS;
} // Isa::Create

hsa_status_t Isa::Create(
  const char *in_isa_name,
  hsa_isa_t *out_isa_handle
) {
  assert(in_isa_name);
  assert(out_isa_handle);

  try {
    isa_list.push_back(Isa());
  } catch (const std::bad_alloc) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  hsa_status_t hsa_status_code = isa_list.back().Initialize(in_isa_name);
  if (HSA_STATUS_SUCCESS != hsa_status_code) {
    isa_list.pop_back();
    return hsa_status_code;
  }

  *out_isa_handle = Isa::Handle(&isa_list.back());
  return HSA_STATUS_SUCCESS;
} // Isa::Create

hsa_isa_t Isa::Handle(const Isa *in_isa_object) {
  assert(in_isa_object);

  hsa_isa_t out_isa_handle;
  out_isa_handle.handle = reinterpret_cast<uint64_t>(in_isa_object);
  return out_isa_handle;
} // Isa::Handle

Isa* Isa::Object(const hsa_isa_t &in_isa_handle) {
  return reinterpret_cast<Isa*>(in_isa_handle.handle);
} // Isa::Object

hsa_status_t Isa::Initialize(const hsa_agent_t &in_agent) {
  uint32_t agent_device_id = 0;
  hsa_status_t hsa_status_code = HSA::hsa_agent_get_info(
    in_agent,
    static_cast<hsa_agent_info_t>(HSA_EXT_AMD_AGENT_INFO_DEVICE_ID),
    &agent_device_id
  );
  if (HSA_STATUS_SUCCESS != hsa_status_code) {
    return hsa_status_code;
  }

  vendor_ = ISA_NAME_AMD_VENDOR;
  device_ = ISA_NAME_AMD_DEVICE;

  compute_capability_.Initialize(agent_device_id);
  if (!compute_capability_.IsValid())  {
    // TODO: new error code?
    return HSA_STATUS_ERROR;
  }

  std::ostringstream full_name_stream; full_name_stream << *this;
  if (!full_name_stream.good()) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }
  full_name_ = full_name_stream.str();

  return HSA_STATUS_SUCCESS;
} // Isa::Initialize

hsa_status_t Isa::Initialize(const char *in_isa_name) {
  assert(in_isa_name);

  std::list<std::string> isa_name_tokens;
  if (!Tokenize(in_isa_name, ':', isa_name_tokens)) {
    return HSA_STATUS_ERROR_INVALID_ISA_NAME;
  }

  if (ISA_NAME_AMD_TOKEN_COUNT != isa_name_tokens.size()) {
    return HSA_STATUS_ERROR_INVALID_ISA_NAME;
  }

  full_name_ = in_isa_name;

  vendor_ = isa_name_tokens.front();
  isa_name_tokens.pop_front();

  device_ = isa_name_tokens.front();
  isa_name_tokens.pop_front();

  compute_capability_.set_version_major(std::stoi(isa_name_tokens.front()));
  isa_name_tokens.pop_front();

  compute_capability_.set_version_minor(std::stoi(isa_name_tokens.front()));
  isa_name_tokens.pop_front();

  compute_capability_.set_version_stepping(std::stoi(isa_name_tokens.front()));
  isa_name_tokens.pop_front();

  assert(0 == isa_name_tokens.size());

  if (!IsValid()) {
    return HSA_STATUS_ERROR_INVALID_ISA_NAME;
  }

  return HSA_STATUS_SUCCESS;
} // Isa::Initialize

void Isa::Reset() {
  full_name_ = "";
  vendor_ = "";
  device_ = "";
  compute_capability_.Reset();
} // Isa::Reset

hsa_status_t Isa::GetInfo(
  const hsa_isa_info_t &in_isa_attribute,
  const uint32_t &in_call_convention_index,
  void *out_value
) const {
  assert(out_value);

  switch (in_isa_attribute) {
    case HSA_ISA_INFO_NAME_LENGTH: {
      *((uint32_t*)out_value) = static_cast<uint32_t>(full_name_.size());
      return HSA_STATUS_SUCCESS;
    }
    case HSA_ISA_INFO_NAME: {
      memcpy(out_value, full_name_.c_str(), full_name_.size());
      return HSA_STATUS_SUCCESS;
    }
    case HSA_ISA_INFO_CALL_CONVENTION_COUNT: {
      // TODO: not implemented.
      assert(false && "not implemented");
      return HSA_STATUS_ERROR;
    }
    case HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONT_SIZE: {
      // TODO: not implemented.
      assert(false && "not implemented");
      return HSA_STATUS_ERROR;
    }
    case HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONTS_PER_COMPUTE_UNIT: {
      // TODO: not implemented.
      assert(false && "not implemented");
      return HSA_STATUS_ERROR;
    }
    default: {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }
} // Isa::GetInfo

hsa_status_t Isa::IsCompatible(
  const Isa &in_isa_object,
  bool *out_result
) const {
  assert(out_result);

  *out_result =
    0 == full_name_.compare(in_isa_object.full_name()) ? true : false;
  return HSA_STATUS_SUCCESS;
} // Isa::IsCompatible

bool Isa::IsValid() {
  if (0 != vendor_.compare(ISA_NAME_AMD_VENDOR)) { return false; }
  if (0 != device_.compare(ISA_NAME_AMD_DEVICE)) { return false; }
  return compute_capability_.IsValid();
} // Isa::IsValid

std::ostream& operator<<(
  std::ostream &out_stream,
  const Isa &in_isa
) {
  return out_stream <<
    in_isa.vendor_ << ":" <<
    in_isa.device_ << ":" <<
    in_isa.compute_capability_;
} // ostream<<Isa

} // namespace loader
} // namespace core
