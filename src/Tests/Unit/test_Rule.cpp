//
// Copyright (C) 2016 Red Hat, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors: Daniel Kopecek <dkopecek@redhat.com>
//
#include <catch.hpp>
#include <Rule.hpp>

using namespace usbguard;

TEST_CASE("Default constructed rule", "[Rule]") {
  Rule rule;

  SECTION("has default id") {
    REQUIRE(rule.getID() == Rule::DefaultID);
  }

  SECTION("is implicit") {
    REQUIRE(rule.isImplicit());
  }

  SECTION("has invalid target") {
    REQUIRE(rule.getTarget() == Rule::Target::Invalid);
  }

  SECTION("has empty attributes") {
    REQUIRE(rule.getVendorID().empty());
    REQUIRE(rule.getProductID().empty());
    REQUIRE(rule.getSerialNumber().empty());
    REQUIRE(rule.getDeviceName().empty());
    REQUIRE(rule.getDeviceHash().empty());
    REQUIRE(rule.getDevicePorts().size() == 0);
    REQUIRE(rule.getInterfaceTypes().size() == 0);
    REQUIRE(rule.getAction().empty());
    REQUIRE(rule.getTimeoutSeconds() == 0);
  }

  SECTION("is not valid") {
    REQUIRE_FALSE(rule);
  }

  SECTION("cannot be converted to a string form unless forced to") {
    REQUIRE_THROWS(rule.toString());
    REQUIRE_NOTHROW(rule.toString(/*invalid=*/true));
  }
}

TEST_CASE("Target-only rule", "[RuleParser]") {
  Rule rule;

  SECTION("from/to string: allow") {
    REQUIRE_NOTHROW(rule = Rule::fromString("allow"));
    REQUIRE(rule.getTarget() == Rule::Target::Allow);
    REQUIRE_NOTHROW(rule.toString() == "allow");
  }

  SECTION("from/to string: block") {
    REQUIRE_NOTHROW(rule = Rule::fromString("block"));
    REQUIRE(rule.getTarget() == Rule::Target::Block);
    REQUIRE_NOTHROW(rule.toString() == "block");
  }

  SECTION("from/to string: reject") {
    REQUIRE_NOTHROW(rule = Rule::fromString("reject"));
    REQUIRE(rule.getTarget() == Rule::Target::Reject);
    REQUIRE_NOTHROW(rule.toString() == "reject");
  }
}
