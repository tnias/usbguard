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
#include <cstdint>

namespace usbguard
{

#define BOMS_RESET       0xFF
#define BOMS_GET_MAX_LUN 0xFE

  namespace MassStorage
  {
  /*
   * Command Block Wrapper
   */
  struct CBW
  {
    uint8_t dCBWSignature[4];
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    uint8_t CBWCB[16];
  } __attribute__((packed));

  /*
   * Command Status Wrapper
   */
  struct CSW
  {
    uint8_t dCSWSignature[4];
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
  } __attribute__((packed));

  /*
   * Command Descriptor Block
   */
  struct CDBInquiry
  {
    uint8_t dOpCode; /* 0x12 */
    uint8_t bmFlags;
    uint8_t dPageCode;
    uint16_t dAllocLength;
    uint8_t dControl;
  } __attribute((packed));

  struct CDBReadCapacity
  {
    uint8_t dOpCode;
    uint8_t bmFlags;
    uint32_t dLBA;
    uint16_t dReserved;
    uint8_t bmPMI;
    uint8_t dControl;
  } __attribute((packed));

  struct CDBRead10
  {
    uint8_t dOpCode; /* 0x28 */
    uint8_t bmFlags;
    uint32_t dLBA;
    uint8_t dGroupNumber;
    uint16_t dTransferLength;
    uint8_t dControl;
  } __attribute((packed));

  //void massStorageSendCommand();
  //void massStorageGetStatus();

  } /* namespace MassStorage */
} /* namespace usbguard */
