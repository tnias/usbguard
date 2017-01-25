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
// ////
//
// Based on xusb.c (https://github.com/libusb/libusb/blob/master/examples/xusb.c)
//
// xusb: Generic USB test program
// Copyright © 2009-2012 Pete Batard <pete@akeo.ie>
// Contributions to Mass Storage by Alan Stern.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//

#include "usbguard-mass-storage-read.hpp"
#include <iostream>
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <cstring>
#include <Logger.hpp>
#include <libusb.h>
#include <getopt.h>
#include <endian.h>

namespace usbguard
{
  const uint8_t MassStorageCDBLength[256] = {
    6,  6,  6,  6,   6,  6,  6,  6,   6,  6,  6,  6,   6,  6,  6,  6,
    6,  6,  6,  6,   6,  6,  6,  6,   6,  6,  6,  6,   6,  6,  6,  6,
    10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,
    10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,
    10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,
    10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,  10, 10, 10, 10,
    0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
    0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
    16, 16, 16, 16,  16, 16, 16, 16,  16, 16, 16, 16,  16, 16, 16, 16,
    16, 16, 16, 16,  16, 16, 16, 16,  16, 16, 16, 16,  16, 16, 16, 16,
    12, 12, 12, 12,  12, 12, 12, 12,  12, 12, 12, 12,  12, 12, 12, 12,
    12, 12, 12, 12,  12, 12, 12, 12,  12, 12, 12, 12,  12, 12, 12, 12,
    0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
    0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0
  };

  const uint8_t MassStorageCDBMaxLength = 16;

  /*
   * 
   */
  static int massStorageSendCommand(libusb_device_handle* const handle, uint8_t endpoint, uint8_t lun,
      uint8_t *cdb, uint8_t direction, int size, uint32_t *return_tag)
  {
    static uint32_t cbw_tag = 1;
    struct MassStorage::CBW cbw = { 0 };

    if (endpoint & LIBUSB_ENDPOINT_IN) {
      USBGUARD_LOG(Error) << "wrong endpoint";
      return -1;
    }

    const uint8_t cdb_len = MassStorageCDBLength[cdb[0]];

    if ((cdb_len == 0) || (cdb_len > sizeof cbw.CBWCB)) {
      USBGUARD_LOG(Error) << "wrong CDB length";
      return -1;
    }

    *return_tag = cbw_tag;

    memcpy(cbw.dCBWSignature, "USBC", 4);
    memcpy(cbw.CBWCB, cdb, cdb_len);

    cbw.dCBWTag = cbw_tag++;
    cbw.dCBWDataTransferLength = size;
    cbw.bmCBWFlags = direction;
    cbw.bCBWLUN = lun;
    cbw.bCBWCBLength = cdb_len;

    int result = -1;
    int result_size = 0;

    for (int retry = 0; retry <= 3 && result != LIBUSB_SUCCESS; ++retry) {
      USBGUARD_LOG(Debug) << "retry=" << retry;
      /*
       * Questions:
       *  1) Why 31?
       *  2) Why 1000?
       */
      result = libusb_bulk_transfer(handle, endpoint, (unsigned char*)&cbw, 31, &result_size, 1000);

      if (result == LIBUSB_ERROR_PIPE) {
        USBGUARD_LOG(Debug) << "error_pipe => retry";
        libusb_clear_halt(handle, endpoint);
      }
    }

    if (result != LIBUSB_SUCCESS) {
      USBGUARD_LOG(Error) << "result=" << result;
      return -1;
    }

    return 0;
  }

  static void massStorageReadToStream(libusb_device_handle* const handle, uint8_t *endpoint, std::ostream& output_stream, size_t max_size)
  {
    int result = -1;
    std::cout << "reading!" << std::endl;

    uint8_t max_lun = 0;

    result = libusb_control_transfer(handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
        BOMS_GET_MAX_LUN, 0, 0,
        &max_lun, sizeof(uint8_t), 1000);

    /*
     * Some devices send a STALL instead of the actual value.
     * In such cases we should set lun to 0.
     */
    if (result == 0) {
      max_lun = 0;
    } else if (result < 0) {
      USBGUARD_LOG(Error) << "max_lun error";
      max_lun = 0;
    }

    printf("   Max LUN = %d\n", max_lun);

    struct MassStorage::CDBInquiry cdb = { 0 };

    cdb.dOpCode = 0x12;
    cdb.dAllocLength = htobe16(36); /*  why 36 ? */
    
    uint32_t expected_tag = 0;
    massStorageSendCommand(handle, endpoint[0], max_lun, (uint8_t*)&cdb,
        LIBUSB_ENDPOINT_IN, 36, &expected_tag);

  }

  static int massStorageReadToStream(int busnum, int devnum, uint8_t *endpoint, std::ostream& output_stream, size_t max_size)
  {
    USBGUARD_LOG(Trace) << "busnum=" << busnum << " devnum=" << devnum << " max_size=" << max_size;

    if (libusb_init(nullptr) != 0) {
      USBGUARD_LOG(Error) << "libusb: library initialization failed";
      return EXIT_FAILURE;
    }

    int retval = EXIT_SUCCESS;
    libusb_device **list = nullptr;
    libusb_device *found = nullptr;

    const ssize_t count = libusb_get_device_list(nullptr, &list);

    if (count < 0) {
      USBGUARD_LOG(Error) << "libusb: " << libusb_error_name(count);
      throw std::runtime_error("libusb error");
    }

    for (size_t i = 0; i < (size_t)count && found == nullptr; ++i) {
      libusb_device * const device = list[i];

      const int device_busnum = libusb_get_bus_number(device);
      const int device_devnum = libusb_get_device_address(device);

      USBGUARD_LOG(Debug) << "Inspecting device: busnum=" << device_busnum
        << " devnum=" << device_devnum;

      if (device_busnum == busnum && device_devnum == devnum) {
        found = device;
      }
    }

    if (found == nullptr) {
      USBGUARD_LOG(Error) << "Device not found";
      return EXIT_FAILURE;
    }

    try {
      libusb_device_handle *handle = nullptr;

      const int rc = libusb_open(found, &handle);

      if (rc != 0) {
        USBGUARD_LOG(Error) << "libusb: " << libusb_error_name(rc);
        throw std::runtime_error("libusb error");
      }

      if (libusb_claim_interface(handle, 0) != LIBUSB_SUCCESS) {
        USBGUARD_LOG(Warning) << "claim interface error";
//        throw std::runtime_error("libusb claim error");
      }

      massStorageReadToStream(handle, endpoint, output_stream, max_size);
    }
    catch(...) {
      USBGUARD_LOG(Error) << "read failed";
      retval = EXIT_FAILURE;
    }

    libusb_free_device_list(list, 1);

    return retval;
  }
} /* namespace usbguard */

static const char *options_short = "ho:";

static const struct ::option options_long[] = {
  { "help", no_argument, nullptr, 'h' },
  { "output", required_argument, nullptr, 'o' },
  { "size", required_argument, nullptr, 's' },
  { nullptr, 0, nullptr, 0 }
};

static void showHelp(std::ostream& stream, const char *usbguard_arg0)
{
  stream << " Usage: " << ::basename(usbguard_arg0) << " [OPTIONS] <syspath>" << std::endl;
  stream << "        " << ::basename(usbguard_arg0) << " [OPTIONS] <busnum> <devnum> <e_out> <e_in>" << std::endl;
  stream << std::endl;
  stream << " Options:" << std::endl;
  stream << "  -s, --size <bytes>    Maximum number of bytes to read from the device. Default is 4096." << std::endl;
  stream << "  -o, --output <path>   Output file. Standard output will be used if not specified." << std::endl;
  stream << "  -h, --help            Show this help." << std::endl;
  stream << std::endl;
}

using namespace usbguard;

int main(int argc, char **argv)
{
  const char *usbguard_arg0 = argv[0];
  std::string output_path;
  size_t read_max_size = 4096;
  int opt = 0;

  while ((opt = getopt_long(argc, argv, options_short, options_long, nullptr)) != -1) {
    switch(opt) {
      case 'h':
        showHelp(std::cout, usbguard_arg0);
        return EXIT_SUCCESS;
      case 'o':
        output_path = optarg;
        break;
      case 's':
        read_max_size = std::strtoull(optarg, nullptr, 10);

        if (errno == ERANGE) {
          std::cerr << "Invalid --size value.";
          return EXIT_FAILURE;
        }
        break;
    }
  }

  argc -= optind;
  argv += optind;

  if (!(argc == 4 || argc == 3)) {
    showHelp(std::cerr, usbguard_arg0);
    return EXIT_FAILURE;
  }

  int busnum = -1;
  int devnum = -1;
  uint8_t endpoint[2];

  if (argc == 4) {
    busnum = strtol(argv[0], nullptr, 10);
    devnum = strtol(argv[1], nullptr, 10);
    endpoint[0] = strtoul(argv[2], nullptr, 10);
    endpoint[1] = strtoul(argv[3], nullptr, 10);
  }

  std::ofstream output_stream;

  if (!output_path.empty()) {
    output_stream.open(output_path, std::ofstream::trunc);

    if (output_stream.good()) {
      return massStorageReadToStream(busnum, devnum, endpoint, output_stream, read_max_size);
    }
    else {
      USBGUARD_LOG(Error) << output_path << ": errno=" << errno;
      return EXIT_FAILURE;
    }
  }
  else {
    return massStorageReadToStream(busnum, devnum, endpoint, std::cout, read_max_size);
  }

  return EXIT_SUCCESS;
}


