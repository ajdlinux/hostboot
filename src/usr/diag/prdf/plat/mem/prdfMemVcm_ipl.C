/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/diag/prdf/plat/mem/prdfMemVcm_ipl.C $                 */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2016,2017                        */
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

/** @file prdfMemVcm_ipl.C */

// Platform includes
#include <prdfMemVcm.H>

using namespace TARGETING;

namespace PRDF
{

using namespace PlatServices;

//##############################################################################
//
//                          Generic template functions
//
//##############################################################################

template<TARGETING::TYPE T>
uint32_t VcmEvent<T>::falseAlarm( STEP_CODE_DATA_STRUCT & io_sc )
{
    #define PRDF_FUNC "[VcmEvent::falseAlarm] "

    uint32_t o_rc = SUCCESS;

    PRDF_TRAC( PRDF_FUNC "Chip mark false alarm: 0x%08x,0x%02x",
               iv_chip->getHuid(), getKey() );

    io_sc.service_data->setSignature( iv_chip->getHuid(),
                                      PRDFSIG_VcmFalseAlarm );

    do
    {
        // If DRAM repairs are disabled, make the error log predictive.
        if ( areDramRepairsDisabled() )
        {
            io_sc.service_data->setServiceCall();
            break; // Nothing more to do.
        }

        // Remove the chip mark.
        o_rc = MarkStore::clearChipMark<T>( iv_chip, iv_rank );
        if ( SUCCESS != o_rc )
        {
            PRDF_ERR( PRDF_FUNC "clearChipMark(0x%08x,0x%02x) failed",
                      iv_chip->getHuid(), getKey() );
            break;
        }

    } while (0);

    return o_rc;

    #undef PRDF_FUNC
}

//------------------------------------------------------------------------------

// Avoid linker errors with the template.
template class VcmEvent<TYPE_MCA>;
template class VcmEvent<TYPE_MBA>;

//##############################################################################
//
//                          Specializations for MCA
//
//##############################################################################

template<>
uint32_t VcmEvent<TYPE_MCA>::checkEcc( const uint32_t & i_eccAttns,
                                       STEP_CODE_DATA_STRUCT & io_sc,
                                       bool & o_done )
{
    #define PRDF_FUNC "[VcmEvent<TYPE_MCA>::checkEcc] "

    uint32_t o_rc = SUCCESS;

    do
    {
        // IUEs are reported as UEs during read operations. Therefore, we will
        // treat IUEs like UEs for these scrub operations simply to maintain
        // consistency during all of Memory Diagnostics.
        if ( (i_eccAttns & MAINT_UE) || (i_eccAttns & MAINT_IUE) )
        {
            PRDF_TRAC( PRDF_FUNC "UE Detected: 0x%08x,0x%02x",
                       iv_chip->getHuid(), getKey() );

            io_sc.service_data->setSignature( iv_chip->getHuid(),
                                              (i_eccAttns & MAINT_UE)
                                                           ? PRDFSIG_MaintUE
                                                           : PRDFSIG_MaintIUE );

            o_rc = MemEcc::handleMemUe<TYPE_MCA>( iv_chip,
                                                  MemAddr::fromRank(iv_rank),
                                                  UE_TABLE::SCRUB_UE, io_sc );
            if ( SUCCESS != o_rc )
            {
                PRDF_ERR( PRDF_FUNC "handleMemUe(0x%08x,0x%02x) failed",
                          iv_chip->getHuid(), getKey() );
                break;
            }

            // Leave the mark in place and abort this procedure.
            o_done = true; break;
        }

    } while (0);

    return o_rc;

    #undef PRDF_FUNC
}

//##############################################################################
//
//                          Specializations for MBA
//
//##############################################################################

template<>
uint32_t VcmEvent<TYPE_MBA>::checkEcc( const uint32_t & i_eccAttns,
                                       STEP_CODE_DATA_STRUCT & io_sc,
                                       bool & o_done )
{
    #define PRDF_FUNC "[VcmEvent<TYPE_MBA>::checkEcc] "

    uint32_t o_rc = SUCCESS;

    do
    {
        // IUEs (reported via RCE ETE) are reported as UEs during read
        // operations. Therefore, we will treat IUEs like UEs for these scrub
        // operations simply to maintain consistency during all of Memory
        // Diagnostics.
        if ( (i_eccAttns & MAINT_UE) || (i_eccAttns & MAINT_RCE_ETE) )
        {
            PRDF_TRAC( PRDF_FUNC "UE Detected: 0x%08x,0x%02x",
                       iv_chip->getHuid(), getKey() );

            io_sc.service_data->setSignature( iv_chip->getHuid(),
                                              (i_eccAttns & MAINT_UE)
                                                    ? PRDFSIG_MaintUE
                                                    : PRDFSIG_MaintRETRY_CTE );
            // TODO: RTC 157888
            // - Call mssIplUeIsolation() and add all DIMMs with bad bits to the
            //   callout list.
            // - Make the error log predictive.
            // - Might be able to tuck this into MemEcc::handleMemUe().
            PRDF_ERR( PRDF_FUNC "function not implemented yet" );

            // Leave the mark in place and abort this procedure.
            o_done = true;
        }

    } while (0);

    return o_rc;

    #undef PRDF_FUNC
}

//------------------------------------------------------------------------------

} // end namespace PRDF

