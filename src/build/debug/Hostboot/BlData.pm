# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/debug/Hostboot/BlData.pm $
#
# OpenPOWER HostBoot Project
#
# Contributors Listed Below - COPYRIGHT 2017
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
# IBM_PROLOG_END_TAG

use strict;

package Hostboot::BlData;
use Exporter;
our @EXPORT_OK = ('main');

# Format data into 4-byte segments and 16-byte lines for display
sub formatData
{
    my $data = shift;
    my $dataRaw = "";

    for (my $i = 0; $i < length($data); $i++)
    {
        my $dataHexStr = sprintf("%02X", ord(substr($data, $i, 1)));
        $dataRaw .= $dataHexStr;

        if ($i % 16 == 15)
        {
            $dataRaw .= "\n";
        }
        elsif ($i % 4 == 3)
        {
            $dataRaw .= "   ";
        }
    }

    return $dataRaw."\n";
}

sub main
{
    ::setBootloader();

    my $btLdrHrmorOffset  = 0x0000000000200000;

    my ($dataSym, $dataSize) = ::findSymbolAddress("Bootloader::g_blData");
    if (not defined $dataSym) { ::userDisplay "Cannot find symbol.\n"; die; }
    my $dataSymStr = sprintf("0x%08X", $dataSym);
    my $dataAddr = ::read64($dataSym|$btLdrHrmorOffset);
    my $dataAddrStr = sprintf("0x%016llX", $dataAddr);

    ::userDisplay "------------Bootloader Data------------";
    ::userDisplay "\nData Symbol Address: ";
    ::userDisplay $dataSymStr;
    ::userDisplay "\nData Address: ";
    ::userDisplay $dataAddrStr;

    my ($scratchSym, $scratchSize) =
        ::findSymbolAddress("Bootloader::g_blScratchSpace");
    if (not defined $scratchSym) { ::userDisplay "Cannot find symbol.\n"; die; }
    my $scratchSymStr = sprintf("0x%08X", $scratchSym);
    my $scratchAddr = ::read64($scratchSym|$btLdrHrmorOffset);
    my $scratchAddrStr = sprintf("0x%016llX", $scratchAddr);

    ::userDisplay "\n\nScratch Space Symbol Address: ";
    ::userDisplay $scratchSymStr;
    ::userDisplay "\nScratch Space Address: ";
    ::userDisplay $scratchAddrStr;
    ::userDisplay "\n--------------------------------------------\n";

    my $traceAddr = $dataAddr;
    my $traceAddrStr = sprintf("0x%08X", $traceAddr);
    my $traceSize = 64;
    my $trace = ::readData($traceAddr,$traceSize);
    my $traceData = formatData($trace);

    ::userDisplay "\nTrace Buffer Address: ";
    ::userDisplay $traceAddrStr;
    ::userDisplay "\n\nTrace Data:\n";
    ::userDisplay $traceData;
    ::userDisplay "\n--------------------------------------------\n";


    my $indexAddr = $dataAddr + 64;
    my $indexAddrStr = sprintf("0x%08X", $indexAddr);
    my $index = ::read8($indexAddr);
    my $indexStr = sprintf("0x%02X", $index);

    ::userDisplay "\nTrace Index Address: ";
    ::userDisplay $indexAddrStr;
    ::userDisplay "\n\nTrace Index (Next Entry): ";
    ::userDisplay $indexStr;
    ::userDisplay "\n\n--------------------------------------------\n";


    my $savedAddr = $dataAddr + 66;
    my $savedAddrStr = sprintf("0x%08X", $savedAddr);
    my $saved = ::read8($savedAddr);
    my $savedStr = sprintf("0x%02X", $saved);

    ::userDisplay "\nSaved Trace Index Address: ";
    ::userDisplay $savedAddrStr;
    ::userDisplay "\n\nSaved Trace Index: ";
    ::userDisplay $savedStr;
    ::userDisplay "\n\n--------------------------------------------\n";


    my $loopCntAddr = $dataAddr + 68;
    my $loopCntAddrStr = sprintf("0x%08X", $loopCntAddr);
    my $loopCnt = ::read32($loopCntAddr);
    my $loopCntStr = sprintf("0x%08X", $loopCnt);

    ::userDisplay "\nPNOR Loop Counter Address: ";
    ::userDisplay $loopCntAddrStr;
    ::userDisplay "\n\nPNOR Loop Counter: ";
    ::userDisplay $loopCntStr;
    ::userDisplay "\n\n--------------------------------------------\n";


    my $pnorMmioAddr = $dataAddr + 72;
    my $pnorMmioAddrStr = sprintf("0x%08X", $pnorMmioAddr);
    my $pnorMmio = ::read64($pnorMmioAddr);
    my $pnorMmioStr = sprintf("0x%016llX", $pnorMmio);

    ::userDisplay "\nFirst PNOR MMIO Address: ";
    ::userDisplay $pnorMmioAddrStr;
    ::userDisplay "\n\nFirst PNOR MMIO: ";
    ::userDisplay $pnorMmioStr;
    ::userDisplay "\n\n--------------------------------------------\n";


    my $tiDataAreaAddr = $dataAddr + 80;
    my $tiDataAreaAddrStr = sprintf("0x%08X", $tiDataAreaAddr);
    my $tiDataAreaSize = 48;
    my $tiDataArea = ::readData($tiDataAreaAddr,$tiDataAreaSize);
    my $tiDataAreaData = formatData($tiDataArea);

    ::userDisplay "\nTI Data Area Address: ";
    ::userDisplay $tiDataAreaAddrStr;
    ::userDisplay "\n\nTI Data Area:\n";
    ::userDisplay $tiDataAreaData;
    ::userDisplay "\n--------------------------------------------\n";


    my $hbbPnorSecAddr = $dataAddr + 128;
    my $hbbPnorSecAddrStr = sprintf("0x%08X", $hbbPnorSecAddr);
    my $hbbPnorSecSize = 32;
    my $hbbPnorSec = ::readData($hbbPnorSecAddr,$hbbPnorSecSize);
    my $hbbPnorSecData = formatData($hbbPnorSec);

    ::userDisplay "\nHBB PNOR Section Data Address: ";
    ::userDisplay $hbbPnorSecAddrStr;
    ::userDisplay "\n\nHBB PNOR Section Data:\n";
    ::userDisplay $hbbPnorSecData;
    ::userDisplay "\n--------------------------------------------\n";


    my $secRomValAddr = $dataAddr + 160;
    my $secRomValAddrStr = sprintf("0x%08X", $secRomValAddr);
    my $secRomVal = ::read8($secRomValAddr);
    my $secRomValStr = sprintf("0x%02X", $secRomVal);

    ::userDisplay "\nSecure ROM Valid Address: ";
    ::userDisplay $secRomValAddrStr;
    ::userDisplay "\n\nSecure ROM Valid: ";
    ::userDisplay $secRomValStr;
    ::userDisplay "\n\n--------------------------------------------\n";


    my $blToHbAddr = $dataAddr + 176;
    my $blToHbAddrStr = sprintf("0x%08X", $blToHbAddr);
    my $blToHbSize = 89;
    my $blToHb = ::readData($blToHbAddr,$blToHbSize);
    my $blToHbData = formatData($blToHb);

    ::userDisplay "\nBL to HB Data Address: ";
    ::userDisplay $blToHbAddrStr;
    ::userDisplay "\n\nBootloader to Hostboot Data:\n";
    ::userDisplay $blToHbData;
    ::userDisplay "\n--------------------------------------------\n";

    ::clearBootloader();
}

sub helpInfo
{
    my %info = (
        name => "BlData",
        intro => ["Displays Bootloader data."],
    );
}
