/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/Format.h>
#include <folly/IPAddress.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/Thrift.h>

#include <openr/common/Constants.h>
#include <openr/if/gen-cpp2/Fib_types.h>
#include <openr/if/gen-cpp2/Lsdb_types.h>
#include <openr/if/gen-cpp2/Network_types.h>

namespace std {

/**
 * Make IpPrefix hashable
 */
template <>
struct hash<openr::thrift::IpPrefix> {
  size_t operator()(openr::thrift::IpPrefix const&) const;
};

/**
 * Make BinaryAddress hashable
 */
template <>
struct hash<openr::thrift::BinaryAddress> {
  size_t operator()(openr::thrift::BinaryAddress const&) const;
};

/**
 * Make MplsAction hashable
 */
template <>
struct hash<openr::thrift::MplsAction> {
  size_t operator()(openr::thrift::MplsAction const&) const;
};

/**
 * Make NextHopThrift hashable
 */
template <>
struct hash<openr::thrift::NextHopThrift> {
  size_t operator()(openr::thrift::NextHopThrift const&) const;
};

/**
 * Make UnicastRoute hashable
 */
template <>
struct hash<openr::thrift::UnicastRoute> {
  size_t operator()(openr::thrift::UnicastRoute const&) const;
};

} // namespace std

namespace openr {

struct LinkEntry {
  LinkEntry(
      const std::string& ifName,
      const int64_t ifIndex,
      bool isUp,
      std::vector<folly::CIDRNetwork> networks,
      const int64_t weight = 1)
      : ifName(ifName),
        ifIndex(ifIndex),
        isUp(isUp),
        networks(networks),
        weight(weight) {}
  // interface name
  const std::string ifName{""};

  // interface index
  const int64_t ifIndex{0};

  // boolean flag indicating link UP/DOWN
  bool isUp{false};

  // list of networks associated with this interface
  std::vector<folly::CIDRNetwork> networks{};

  // weight for UCMP
  int64_t weight{1};
};

template <class IPAddressVx>
thrift::BinaryAddress
toBinaryAddressImpl(const IPAddressVx& addr) {
  thrift::BinaryAddress result;
  result.addr.append(
      reinterpret_cast<const char*>(addr.bytes()), IPAddressVx::byteCount());
  return result;
}

inline thrift::BinaryAddress
toBinaryAddress(const folly::IPAddress& addr) {
  return addr.isV4() ? toBinaryAddressImpl(addr.asV4())
                     : addr.isV6() ? toBinaryAddressImpl(addr.asV6())
                                   : thrift::BinaryAddress();
}

inline thrift::BinaryAddress
toBinaryAddress(const std::string& addr) {
  return toBinaryAddress(folly::IPAddress(addr));
}

template <typename T>
inline folly::IPAddress
toIPAddress(const T& input) {
  return input.type != decltype(input.type)::VUNSPEC
      ? folly::IPAddress(input.addr)
      : folly::IPAddress();
}

inline folly::IPAddress
toIPAddress(const std::string& binAddr) {
  return folly::IPAddress::fromBinary(folly::ByteRange(
      reinterpret_cast<const uint8_t*>(binAddr.data()), binAddr.size()));
}

inline folly::IPAddress
toIPAddress(const thrift::BinaryAddress& addr) {
  return folly::IPAddress::fromBinary(folly::ByteRange(
      reinterpret_cast<const unsigned char*>(addr.addr.data()),
      addr.addr.size()));
}

inline folly::CIDRNetwork
toIPNetwork(const thrift::IpPrefix& prefix, bool applyMask = true) {
  return folly::IPAddress::createNetwork(
      toIPAddress(*prefix.prefixAddress_ref()).str(),
      *prefix.prefixLength_ref(),
      applyMask);
}

inline thrift::IpPrefix
toIpPrefix(const folly::CIDRNetwork& network) {
  return thrift::IpPrefix(
      apache::thrift::FRAGILE, toBinaryAddress(network.first), network.second);
}

inline thrift::IpPrefix
toIpPrefix(const std::string& prefix) {
  return toIpPrefix(folly::IPAddress::createNetwork(prefix));
}

inline std::string
toString(const thrift::BinaryAddress& addr) {
  return addr.addr.empty() ? "" : toIPAddress(addr).str();
}

inline std::string
toString(const thrift::IpPrefix& ipPrefix) {
  return folly::sformat(
      "{}/{}",
      toString(*ipPrefix.prefixAddress_ref()),
      *ipPrefix.prefixLength_ref());
}

inline std::string
toString(const thrift::MplsAction& mplsAction) {
  return folly::sformat(
      "mpls {} {}{}",
      apache::thrift::util::enumNameSafe(*mplsAction.action_ref()),
      mplsAction.swapLabel_ref() ? std::to_string(*mplsAction.swapLabel_ref())
                                 : "",
      mplsAction.pushLabels_ref()
          ? folly::join("/", *mplsAction.pushLabels_ref())
          : "");
}

inline std::string
toString(const thrift::NextHopThrift& nextHop) {
  return folly::sformat(
      "via {} dev {} weight {} metric {} area {} {}",
      toIPAddress(*nextHop.address_ref()).str(),
      nextHop.address_ref()->ifName_ref().value_or("N/A"),
      *nextHop.weight_ref(),
      *nextHop.metric_ref(),
      nextHop.area_ref().value_or("N/A"),
      nextHop.mplsAction_ref().has_value()
          ? toString(nextHop.mplsAction_ref().value())
          : "");
}

inline std::string
toString(const thrift::UnicastRoute& route) {
  std::vector<std::string> lines;
  lines.emplace_back(folly::sformat("> Prefix: {}", toString(route.dest)));
  for (const auto& nh : *route.nextHops_ref()) {
    lines.emplace_back("  " + toString(nh));
  }
  return folly::join("\n", lines);
}

inline std::string
toString(const thrift::MplsRoute& route) {
  std::vector<std::string> lines;
  lines.emplace_back(folly::sformat("> Label: {}", route.topLabel));
  for (const auto& nh : *route.nextHops_ref()) {
    lines.emplace_back("  " + toString(nh));
  }
  return folly::join("\n", lines);
}

} // namespace openr
