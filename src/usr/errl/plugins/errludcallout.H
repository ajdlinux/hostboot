/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/errl/plugins/errludcallout.H $                        */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2013,2017                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef ERRL_UDCALLOU_H
#define ERRL_UDCALLOU_H

/**
 *  @file errludcallout.H
 *
 *  Defines the ErrlUserDetailsParserCallout class that parses
 *  callout user detail in an error log
 */

#include "errluserdetails.H"
#include "errludtarget.H" // for errlud_parse_entity_path()
#include "../hwas/common/hwasCallout.H"

namespace ERRORLOG
{

/**
 * @class ErrlUserDetailsCallout
 *
 * Parses Callout user detail in an error log
 */
class ErrlUserDetailsParserCallout : public ErrlUserDetailsParser
{
public:
    /**
     *  @brief Constructor
     */
    ErrlUserDetailsParserCallout() {}

    /**
     *  @brief Destructor
     */
    virtual ~ErrlUserDetailsParserCallout() {}

    /**
     *  @brief Parses callout user detail data from an error log
     *
     *  @param  i_version Version of the data
     *  @param  i_parse   ErrlUsrParser object for outputting information
     *  @param  i_pBuffer Pointer to buffer containing detail data
     *  @param  i_buflen  Length of the buffer
     */
    virtual void parse(errlver_t i_version,
                       ErrlUsrParser & i_parser,
                       void * i_pBuffer,
                       const uint32_t i_buflen) const
    {
        HWAS::callout_ud_t *pData;
        pData = static_cast<HWAS::callout_ud_t *>(i_pBuffer);

        switch (pData->type)
        {
            case HWAS::CLOCK_CALLOUT:
            {
                switch (ntohl(pData->clockType))
                {
#define case_CLOCK_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "Clock Type", #_type); break;
                    case_CLOCK_TYPE(TODCLK_TYPE)
                    case_CLOCK_TYPE(MEMCLK_TYPE)
                    case_CLOCK_TYPE(OSCREFCLK_TYPE)
                    case_CLOCK_TYPE(OSCPCICLK_TYPE)
                    default:
                        i_parser.PrintNumber( "Clock Type", "UNKNOWN: 0x%X",
                                        ntohl(pData->clockType) );
                } // switch clockType
#undef case_CLOCK_TYPE
                switch (ntohl(pData->clkDeconfigState))
                {
#define case_DECONFIG_STATE(_type) \
case HWAS::_type: i_parser.PrintString( "Deconfig State", #_type); break;
                    case_DECONFIG_STATE(NO_DECONFIG)
                    case_DECONFIG_STATE(DECONFIG)
                    case_DECONFIG_STATE(DELAYED_DECONFIG)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->clkDeconfigState) );
                        break;
                } // switch clkDeconfigState
#undef case_DECONFIG_STATE
                switch (ntohl(pData->clkGardErrorType))
                {
#define case_GARD_ERROR_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "GARD Error Type", #_type); break;
                    case_GARD_ERROR_TYPE(GARD_NULL)
                    case_GARD_ERROR_TYPE(GARD_User_Manual)
                    case_GARD_ERROR_TYPE(GARD_Unrecoverable)
                    case_GARD_ERROR_TYPE(GARD_Fatal)
                    case_GARD_ERROR_TYPE(GARD_Predictive)
                    case_GARD_ERROR_TYPE(GARD_Power)
                    case_GARD_ERROR_TYPE(GARD_PHYP)
                    case_GARD_ERROR_TYPE(GARD_Reconfig)
                    case_GARD_ERROR_TYPE(GARD_Void)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->clkGardErrorType) );
                        break;
                } // switch clkGardErrorType
#undef case_GARD_ERROR_TYPE
                // what follows the pData structure is one entity path
                // print it out
                uint8_t *l_ptr = reinterpret_cast<uint8_t *>(pData+1);
                printEntityPath(l_ptr, i_parser, "Target");

                break; // CLOCK_CALLOUT
            }
            case HWAS::PART_CALLOUT:
            {
                switch (ntohl(pData->partType))
                {
#define case_PART_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "Part Type", #_type); break;
                    case_PART_TYPE(FLASH_CONTROLLER_PART_TYPE)
                    case_PART_TYPE(PNOR_PART_TYPE)
                    case_PART_TYPE(SBE_SEEPROM_PART_TYPE)
                    case_PART_TYPE(VPD_PART_TYPE)
                    case_PART_TYPE(LPC_SLAVE_PART_TYPE)
                    case_PART_TYPE(GPIO_EXPANDER_PART_TYPE)
                    case_PART_TYPE(SPIVID_SLAVE_PART_TYPE)

                    default:
                        i_parser.PrintNumber( "Part Type", "UNKNOWN: 0x%X",
                                        ntohl(pData->partType) );
                } // switch partType
#undef case_PART_TYPE
                switch (ntohl(pData->partDeconfigState))
                {
#define case_DECONFIG_STATE(_type) \
case HWAS::_type: i_parser.PrintString( "Deconfig State", #_type); break;
                    case_DECONFIG_STATE(NO_DECONFIG)
                    case_DECONFIG_STATE(DECONFIG)
                    case_DECONFIG_STATE(DELAYED_DECONFIG)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->partDeconfigState) );
                        break;
                } // switch partDeconfigState
#undef case_DECONFIG_STATE
                switch (ntohl(pData->partGardErrorType))
                {
#define case_GARD_ERROR_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "GARD Error Type", #_type); break;
                    case_GARD_ERROR_TYPE(GARD_NULL)
                    case_GARD_ERROR_TYPE(GARD_User_Manual)
                    case_GARD_ERROR_TYPE(GARD_Unrecoverable)
                    case_GARD_ERROR_TYPE(GARD_Fatal)
                    case_GARD_ERROR_TYPE(GARD_Predictive)
                    case_GARD_ERROR_TYPE(GARD_Power)
                    case_GARD_ERROR_TYPE(GARD_PHYP)
                    case_GARD_ERROR_TYPE(GARD_Reconfig)
                    case_GARD_ERROR_TYPE(GARD_Void)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->partGardErrorType) );
                        break;
                } // switch partGardErrorType
#undef case_GARD_ERROR_TYPE
                // what follows the pData structure is one entity path
                // print it out
                uint8_t *l_ptr = reinterpret_cast<uint8_t *>(pData+1);
                printEntityPath(l_ptr, i_parser, "Target");

                break; // PART_CALLOUT
            }
            case HWAS::BUS_CALLOUT:
            {
                switch (ntohl(pData->busType))
                {
#define case_BUS_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "Bus Type", #_type); break;
                    case_BUS_TYPE(FSI_BUS_TYPE)
                    case_BUS_TYPE(DMI_BUS_TYPE)
                    case_BUS_TYPE(A_BUS_TYPE)
                    case_BUS_TYPE(X_BUS_TYPE)
                    case_BUS_TYPE(I2C_BUS_TYPE)
                    case_BUS_TYPE(PSI_BUS_TYPE)
                    case_BUS_TYPE(O_BUS_TYPE)
                    default:
                        i_parser.PrintNumber( "Bus Type", "UNKNOWN: 0x%X",
                                        ntohl(pData->busType) );
                } // switch busType
#undef case_BUS_TYPE
                // what follows the pData structure is Two entity paths
                // print them out
                uint8_t *l_ptr = reinterpret_cast<uint8_t *>(pData+1);
                printEntityPath(l_ptr, i_parser, "Target1");
                printEntityPath(l_ptr, i_parser, "Target2");

                break; // BUS_CALLOUT
            }
            case HWAS::HW_CALLOUT:
            {
                i_parser.PrintString( "Callout type", "Hardware Callout");

                // what follows the pData structure is an entity path
                // print it first
                uint8_t *l_ptr = reinterpret_cast<uint8_t *>(pData+1);
                printEntityPath(l_ptr, i_parser, "Target");

                i_parser.PrintNumber( "CPU id", "0x%X", ntohl(pData->cpuid) );

                switch (ntohl(pData->deconfigState))
                {
#define case_DECONFIG_STATE(_type) \
case HWAS::_type: i_parser.PrintString( "Deconfig State", #_type); break;
                    case_DECONFIG_STATE(NO_DECONFIG)
                    case_DECONFIG_STATE(DECONFIG)
                    case_DECONFIG_STATE(DELAYED_DECONFIG)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->deconfigState) );
                        break;
                } // switch deconfigState
#undef case_DECONFIG_STATE
                switch (ntohl(pData->gardErrorType))
                {
#define case_GARD_ERROR_TYPE(_type) \
case HWAS::_type: i_parser.PrintString( "GARD Error Type", #_type); break;
                    case_GARD_ERROR_TYPE(GARD_NULL)
                    case_GARD_ERROR_TYPE(GARD_User_Manual)
                    case_GARD_ERROR_TYPE(GARD_Unrecoverable)
                    case_GARD_ERROR_TYPE(GARD_Fatal)
                    case_GARD_ERROR_TYPE(GARD_Predictive)
                    case_GARD_ERROR_TYPE(GARD_Power)
                    case_GARD_ERROR_TYPE(GARD_PHYP)
                    case_GARD_ERROR_TYPE(GARD_Reconfig)
                    case_GARD_ERROR_TYPE(GARD_Void)
                    default:
                        i_parser.PrintNumber( "Deconfig State", "UNKNOWN: 0x%X",
                                        ntohl(pData->gardErrorType) );
                        break;
                } // switch deconfigState
#undef case_GARD_ERROR_TYPE
                break; // HW_CALLOUT
            }
            case HWAS::PROCEDURE_CALLOUT:
            {
                i_parser.PrintString( "Callout type", "Procedure Callout");
                switch (ntohl(pData->procedure))
                {
#define case_PROCEDURE(_type) \
case HWAS::_type: i_parser.PrintString( "Procedure", #_type); break;
                    case_PROCEDURE(EPUB_PRC_NONE)
                    case_PROCEDURE(EPUB_PRC_FIND_DECONFIGURED_PART)
                    case_PROCEDURE(EPUB_PRC_SP_CODE)
                    case_PROCEDURE(EPUB_PRC_PHYP_CODE)
                    case_PROCEDURE(EPUB_PRC_ALL_PROCS)
                    case_PROCEDURE(EPUB_PRC_ALL_MEMCRDS)
                    case_PROCEDURE(EPUB_PRC_INVALID_PART)
                    case_PROCEDURE(EPUB_PRC_LVL_SUPP)
                    case_PROCEDURE(EPUB_PRC_PROCPATH)
                    case_PROCEDURE(EPUB_PRC_NO_VPD_FOR_FRU)
                    case_PROCEDURE(EPUB_PRC_MEMORY_PLUGGING_ERROR)
                    case_PROCEDURE(EPUB_PRC_FSI_PATH)
                    case_PROCEDURE(EPUB_PRC_PROC_AB_BUS)
                    case_PROCEDURE(EPUB_PRC_PROC_XYZ_BUS)
                    case_PROCEDURE(EPUB_PRC_MEMBUS_ERROR)
                    case_PROCEDURE(EPUB_PRC_EIBUS_ERROR)
                    case_PROCEDURE(EPUB_PRC_POWER_ERROR)
                    case_PROCEDURE(EPUB_PRC_MEMORY_UE)
                    case_PROCEDURE(EPUB_PRC_PERFORMANCE_DEGRADED)
                    case_PROCEDURE(EPUB_PRC_HB_CODE)
                    case_PROCEDURE(EPUB_PRC_TOD_CLOCK_ERR)
                    case_PROCEDURE(EPUB_PRC_COOLING_SYSTEM_ERR)
                    case_PROCEDURE(EPUB_PRC_FW_VERIFICATION_ERR)
                    case_PROCEDURE(EPUB_PRC_GPU_ISOLATION_PROCEDURE)
                    default:
                        i_parser.PrintNumber( "Procedure", "UNKNOWN: 0x%X",
                                        ntohl(pData->procedure) );
                        break;
                } // switch procedure
#undef case_PROCEDURE
                break; // PROCEDURE_CALLOUT
            }
            default:
                i_parser.PrintNumber( "Callout type", "UNKNOWN: 0x%X",
                                            ntohl(pData->type) );
                break;
        } // switch type

        switch (ntohl(pData->priority))
        {
#define case_PRIORITY(_type) \
case HWAS::_type: i_parser.PrintString( "Priority", #_type); break;
            case_PRIORITY(SRCI_PRIORITY_LOW)
            case_PRIORITY(SRCI_PRIORITY_MEDC)
            case_PRIORITY(SRCI_PRIORITY_MEDB)
            case_PRIORITY(SRCI_PRIORITY_MEDA)
            case_PRIORITY(SRCI_PRIORITY_MED)
            case_PRIORITY(SRCI_PRIORITY_HIGH)
        default:
            i_parser.PrintNumber( "Priority", "UNKNOWN: 0x%X",
                                    ntohl(pData->priority) );
            break;
        } // switch priority
#undef case_PRIORITY
    } // parse

private:
    // Disabled
    ErrlUserDetailsParserCallout(const ErrlUserDetailsParserCallout &);
    ErrlUserDetailsParserCallout & operator=(
        const ErrlUserDetailsParserCallout &);

    void printEntityPath(uint8_t * & io_ptr,
                         ErrlUsrParser & i_parser,
                         const char * i_label) const
    {
        if (*io_ptr == HWAS::TARGET_IS_SENTINEL)
        {
            i_parser.PrintString( i_label,
                        "MASTER_PROCESSOR_CHIP_TARGET_SENTINEL");
            ++io_ptr;
        }
        else
        {
            char outString[128];
            io_ptr = errlud_parse_entity_path(io_ptr, outString);
            i_parser.PrintString(i_label, outString);
        }
    }

};

}

#endif

