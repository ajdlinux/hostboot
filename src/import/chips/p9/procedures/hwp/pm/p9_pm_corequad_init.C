/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/pm/p9_pm_corequad_init.C $ */
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
///
/// @file  p9_pm_corequad_init.C
/// @brief Establish safe settings for Core and Quad.
///
// *HWP HWP Owner: Greg Still <stillgs@us.ibm.com>
// *HWP FW Owner: Sangeetha T S <sangeet2@in.ibm.com>
// *HWP Team: PM
// *HWP Level: 2
// *HWP Consumed by: HS

/// @verbatim
/// High-level procedure flow:
///
///  Procedure Prereq:
///  - completed istep procedure
///  - completed multicast setup
///
///  if PM_INIT
///  {
///    loop over all valid chiplets (EX, EQ and Core)
///    {
///      Clear the Doorbells to remove latent values
///      Setup static Core PM mode register bits
///      Setup static Quad PM mode register bits
///      Setup CME SCRAM scrub index
///      Clear CME flags and scratch registers
///      Restore CME FIR mask value from HWP attribute
///      Restore PPM Error mask value from HWP attribute
///    }
///  }
///  else if PM_RESET
///  {
///    loop over all valid chiplets (EX, EQ and Core)
///    {
///      Save the CME FIR mask value to HWP attribute and clear the register
///      Save the PPM Error mask value to HWP attribute and clear the register
///      Stop the CME 0 & 1
///      Disable OCC Heartbeat
///      Allow access to the PPM
///      Disable resonant clocks
///      Disable IVRM
///      Clear PPM Errors
///      Clear CME FIRs
///    }
///  }
///
/// @endverbatim

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------
#include <p9_pm_corequad_init.H>
#include <p9_query_cache_access_state.H>

// -----------------------------------------------------------------------------
// Constant definitions
// -----------------------------------------------------------------------------

const uint64_t CME_DOORBELL_CLEAR[4] = {C_CPPM_CMEDB0_CLEAR,
                                        C_CPPM_CMEDB1_CLEAR,
                                        C_CPPM_CMEDB2_CLEAR,
                                        C_CPPM_CMEDB3_CLEAR
                                       };

const uint8_t DOORBELLS_COUNT = 4;

// -----------------------------------------------------------------------------
// Function prototypes
// -----------------------------------------------------------------------------

// Reset function
///
/// @brief Stop the CMEs and clear the CME FIRs, PPM Errors and their masks
//        for all functional and enabled EX chiplets
///
/// @param[in] i_target Proc Chip target
/// @param[in] i_cmeFirMask  Mask value for CME FIR
/// @param[in] i_cppmErrMask Mask value for Core PPM Error
/// @param[in] i_qppmErrMask Mask value for Quad PPM Error
///
/// @return FAPI2_RC_SUCCESS on success or error return code
///
fapi2::ReturnCode pm_corequad_reset(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    const uint32_t i_cmeFirMask,
    const uint32_t i_cppmErrMask,
    const uint32_t i_qppmErrMask);

// Init function
///
/// @brief Setup the CME and PPM Core & Quad for all functional and enabled
///        EX chiplets
///
/// @param[in] i_target Proc Chip target
///
/// @return FAPI2_RC_SUCCESS on success or error return code
///
fapi2::ReturnCode pm_corequad_init(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target);

// ----------------------------------------------------------------------
// Function definitions
// ----------------------------------------------------------------------

fapi2::ReturnCode p9_pm_corequad_init(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    const p9pm::PM_FLOW_MODE i_mode,
    const uint32_t i_cmeFirMask,
    const uint32_t i_cppmErrMask,
    const uint32_t i_qppmErrMask)
{
    FAPI_IMP("Entering p9_pm_corequad_init...");

    if (i_mode == p9pm::PM_INIT)
    {
        FAPI_TRY(pm_corequad_init(i_target),
                 "ERROR: Failed to initialize the core quad");
    }
    else if (i_mode == p9pm::PM_RESET)
    {
        FAPI_TRY(pm_corequad_reset(i_target, i_cmeFirMask, i_cppmErrMask,
                                   i_qppmErrMask),
                 "ERROR: Failed to reset core quad");
    }
    else
    {
        FAPI_ASSERT(false,
                    fapi2::PM_COREQUAD_BAD_MODE().set_BADMODE(i_mode),
                    "Unknown mode 0x%X passed to p9_pm_corequad_init", i_mode);
    }

fapi_try_exit:
    FAPI_IMP("Exiting p9_pm_corequad_init...");
    return fapi2::current_err;
}

fapi2::ReturnCode pm_corequad_init(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{
    FAPI_IMP("Entering pm_corequad_init...");

    fapi2::buffer<uint64_t> l_data64;
    uint8_t l_chpltNumber = 0;
    uint64_t l_address = 0;
    uint32_t l_errMask = 0;

    auto l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>
                        (fapi2::TARGET_STATE_FUNCTIONAL);

    FAPI_DBG("Number of quad chiplets retrieved = %u", l_eqChiplets.size());

    // For each functional EQ chiplet
    for (auto l_quad_chplt : l_eqChiplets)
    {

        // Fetch the position of the EQ target
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_quad_chplt,
                               l_chpltNumber),
                 "ERROR: Failed to get the position of the Quad:0x%08X",
                 l_quad_chplt);
        FAPI_DBG("Quad number = %d", l_chpltNumber);

        // Setup the Quad PPM Mode Register
        // Clear the following bits:
        // 0          : Force FSAFE
        // 1  - 11    : FSAFE
        // 12         : Enable FSAFE on heartbeat loss
        // 13         : Enable DROOP protect upon heartbeat loss
        // 14         : Enable PFETs upon iVRMs dropout
        // 18 - 19    : PCB interrupt
        // 20,22,24,26: InterPPM Ivrm/Aclk/Vdata/Dpll enable
        FAPI_INF("Setup Quad PPM Mode register");
        l_data64.flush<0>().setBit<0, 15>().setBit<18, 3>().
        setBit<22>().setBit<24>().setBit<26>();
        l_address = EQ_QPPM_QPMMR_CLEAR;
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to setup Quad PMM register");

        // Restore Quad PPM Error Mask
        FAPI_INF("Restore QUAD PPM Error Mask");
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_QUAD_PPM_ERRMASK, l_quad_chplt,
                               l_errMask),
                 "ERROR: Failed to get CORE PPM error Mask");
        l_data64.flush<0>().insertFromRight<0, 32>(l_errMask);
        l_address = EQ_QPPM_ERRMSK;
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to restore the QUAD PPM Error Mask");

        auto l_coreChiplets =
            l_quad_chplt.getChildren<fapi2::TARGET_TYPE_CORE>
            (fapi2::TARGET_STATE_FUNCTIONAL);

        // For each core target
        for (auto l_core_chplt : l_coreChiplets)
        {
            // Fetch the position of the Core target
            FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_core_chplt,
                                   l_chpltNumber),
                     "ERROR: Failed to get the position of the CORE:0x%08X",
                     l_core_chplt);
            FAPI_DBG("CORE number = %d", l_chpltNumber);

            // Clear the Core CME DoorBells
            FAPI_INF("Clear the Core PPM CME Doorbells");
            l_data64.flush<1>();

            for (uint8_t l_dbLoop = 0; l_dbLoop < DOORBELLS_COUNT; l_dbLoop++)
            {
                l_address = CME_DOORBELL_CLEAR[l_dbLoop];
                FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                         "ERROR: Failed to clear the doorbells");
            }

            // Setup Core PPM Mode register
            // Clear the following bits:
            // 1      : PPM Write control override
            // 11     : Block interrupts
            // 12     : PPM response for CME error
            // 14     : enable pece
            // 15     : cme spwu done dis
            // Bits Init or Reset in STOP Hcode:
            // 0      : PPM Write control
            // 9      : FUSED_CORE_MODE
            // 10     : STOP_EXIT_TYPE_SEL
            // 13     : WKUP_NOTIFY_SELECT
            FAPI_INF("Setup Core PPM Mode register");
            l_data64.flush<0>().setBit<1>().setBit<11, 2>().setBit<14, 2>();
            l_address =  C_CPPM_CPMMR_CLEAR;
            FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                     "ERROR: Failed to setup Core PMM register");

            // Restore CORE PPM Error Mask
            FAPI_INF("Restore CORE PPM Error Mask");
            FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CORE_PPM_ERRMASK, l_core_chplt,
                                   l_errMask),
                     "ERROR: Failed to get CORE PPM error Mask");
            l_data64.flush<0>().insertFromRight<0, 32>(l_errMask);
            l_address = C_CPPM_ERRMSK;
            FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                     "ERROR: Failed to restore the CORE PPM Error Mask");
        }
    }

fapi_try_exit:
    return fapi2::current_err;
}

fapi2::ReturnCode pm_corequad_reset(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    const uint32_t i_cmeFirMask,
    const uint32_t i_cppmErrMask,
    const uint32_t i_qppmErrMask)
{
    FAPI_IMP("Entering pm_corequad_reset...");

    fapi2::buffer<uint64_t> l_data64;
    fapi2::ReturnCode l_rc;
    uint8_t l_chpltNumber = 0;
    uint64_t l_address = 0;
    uint32_t l_errMask = 0;
    uint32_t l_firMask = 0;
    uint32_t l_pollCount = 20;
    bool l_l2_is_scanable = false;
    bool l_l3_is_scanable = false;
    bool l_l2_is_scomable = false;
    bool l_l3_is_scomable = false;

    auto l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>
                        (fapi2::TARGET_STATE_FUNCTIONAL);

    FAPI_DBG("Number of quad chiplets retrieved = %u", l_eqChiplets.size());

    // For each functional EQ chiplet
    for (auto l_quad_chplt : l_eqChiplets)
    {
        // Fetch the position of the EQ target
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_quad_chplt,
                               l_chpltNumber),
                 "ERROR: Failed to get the position of the Quad:0x%08X",
                 l_quad_chplt);
        FAPI_DBG("Quad number = %d", l_chpltNumber);

        // Store QUAD Error Mask in an attribute
        FAPI_INF("Store QUAD PPM ERROR MASK in HWP attribute");
        l_address = EQ_QPPM_ERRMSK;
        FAPI_TRY(fapi2::getScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to fetch the QUAD PPM Error Mask");
        l_data64.extractToRight<uint32_t>(l_errMask, 0, 32);
        FAPI_TRY(FAPI_ATTR_SET(fapi2::ATTR_QUAD_PPM_ERRMASK, l_quad_chplt,
                               l_errMask),
                 "ERROR: Failed to set QUAD PPM error Mask");

        // Write parameter provided value to Quad Error mask
        FAPI_INF("Write QUAD PPM ERROR MASK");
        l_data64.flush<0>().insertFromRight<0, 32>(i_qppmErrMask);
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to write the QUAD PPM Error Mask");

        // Disable OCC Heartbeat
        // Clear the bit 16 : OCC_HEARTBEAT_ENABLE
        FAPI_INF("Disable OCC HeartBeat register");
        l_address = EQ_QPPM_OCCHB;
        FAPI_TRY(fapi2::getScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to fetch OCC HeartBeat register");
        l_data64.clearBit<16>();
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to disable OCC HeartBeat register");

        // Clear Quad PPM Errors
        FAPI_INF("Clear QUAD PPM ERROR Register");
        l_address = EQ_QPPM_ERR;
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to clear QUAD PPM ERROR");

        // Insert the QPMMR operations to clear the inter-PPM enablement
        // bit to allow for access to those functions for FFDC access.
        // Remember, pm_init (reset) can be run without and subsequent
        // pm_init(init) when left in safe mode.
        FAPI_INF("Clear Quad PPM InterPPM Enablement for FFDC");
        l_data64.flush<0>().setBit<20>().setBit<22>().setBit<24>().setBit<26>();
        l_address = EQ_QPPM_QPMMR_CLEAR;
        FAPI_TRY(fapi2::putScom(l_quad_chplt, l_address, l_data64),
                 "ERROR: Failed to setup Quad PMM register");

        //Cannot always rely on HWAS state, during MPIPL attr are not
        //accurate, must use query_cache_access state prior to scomming
        //ex targets
        FAPI_EXEC_HWP(l_rc, p9_query_cache_access_state, l_quad_chplt,
                      l_l2_is_scomable, l_l2_is_scanable,
                      l_l3_is_scomable, l_l3_is_scanable);
        FAPI_TRY(l_rc, "ERROR: failed to query cache access state for EQ %d",
                 l_chpltNumber);

        if(!l_l3_is_scomable)
        {
            //Skip all of the scoms for this EQ if its not scommable
            continue;
        }

        auto l_exChiplets = l_quad_chplt.getChildren<fapi2::TARGET_TYPE_EX>
                            (fapi2::TARGET_STATE_FUNCTIONAL);

        // For each EX target
        for (auto l_ex_chplt : l_exChiplets)
        {
            // Fetch the position of the EX target
            FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_ex_chplt,
                                   l_chpltNumber),
                     "ERROR: Failed to get the position of the EX:0x%08X",
                     l_ex_chplt);
            FAPI_DBG("EX number = %d", l_chpltNumber);

            // Store CME FIR MASK in an attribute
            FAPI_INF("Store CME FIR MASK HWP attribute");
            l_address = EX_CME_SCOM_LFIRMASK;
            FAPI_TRY(fapi2::getScom(l_ex_chplt, l_address, l_data64),
                     "ERROR: Failed to fetch the QUAD PPM Error Mask");
            l_data64.extractToRight<uint32_t>(l_firMask, 0, 32);
            FAPI_TRY(FAPI_ATTR_SET(fapi2::ATTR_CME_LOCAL_FIRMASK, l_ex_chplt,
                                   l_firMask),
                     "ERROR: Failed to set CME FIR Mask");

            // Write parameter provided value to CME FIR MASK
            FAPI_INF(" Write Local CME FIR MASK ");
            l_data64.flush<0>().insertFromRight<0, 32>(i_cmeFirMask);
            l_address = EX_CME_SCOM_LFIRMASK;
            FAPI_TRY(fapi2::putScom(l_ex_chplt, l_address, l_data64),
                     "ERROR: Failed to clear the Local CME FIR Mask");

            // Program XCR to HALT CME 0 & 1 and verify by polling the XSR
            // Set the value of bits as follows:
            // bits (1,2,3) = 001 : halt state
            //
            // NOTE: This will be removed once the PPE common class that does this
            //       functionality is available.
            //       Ex, ppe<PPE_TYPE_CME> cme;
            //           cme.hard_reset();
            FAPI_INF("Stop CME0 and CME1");
            l_data64.flush<0>().insertFromRight(p9hcd::HALT, 1, 3);
            l_address = EX_PPE_XIXCR;
            FAPI_TRY(fapi2::putScom(l_ex_chplt, l_address, l_data64),
                     "ERROR: Failed to stop the CMEs");

            do
            {
                FAPI_INF("Polling the CME Status");
                l_address = EX_PPE_XIRAMDBG;
                FAPI_TRY(fapi2::getScom(l_ex_chplt, l_address, l_data64),
                         "ERROR: Failed to read CME status");
                fapi2::delay(1000, 0); // poll after a delay of 1us
            }
            while((l_data64.getBit<p9hcd::HALTED_STATE>() != 1) &&
                  (--l_pollCount != 0));

            FAPI_ASSERT((l_pollCount != 0),
                        fapi2::PM_CME_ACTIVE_ERROR()
                        .set_TARGET_CHIPLET(l_ex_chplt),
                        "ERROR: Failed to stop the CME");

            // Clear CME LFIRs
            FAPI_INF("Clear CME FIR Register");
            l_data64.flush<0>();
            l_address = EX_CME_SCOM_LFIR_AND;
            FAPI_TRY(fapi2::putScom(l_ex_chplt, l_address, l_data64),
                     "ERROR: Failed to clear CME FIR");

            auto l_coreChiplets =
                l_ex_chplt.getChildren<fapi2::TARGET_TYPE_CORE>
                (fapi2::TARGET_STATE_FUNCTIONAL);

            // For each core target
            for (auto l_core_chplt : l_coreChiplets)
            {
                // Fetch the position of the Core target
                FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_core_chplt,
                                       l_chpltNumber),
                         "ERROR: Failed to get the position of the CORE:0x%08X",
                         l_core_chplt);
                FAPI_DBG("CORE number = %d", l_chpltNumber);

                // Write CPPM Error mask into attribute
                FAPI_INF("Store CORE PPM ERROR MASK in HWP attribute");
                l_address = C_CPPM_ERRMSK;
                FAPI_TRY(fapi2::getScom(l_core_chplt, l_address, l_data64),
                         "ERROR: Failed to fetch the CORE PPM Error Mask");
                l_data64.extractToRight<uint32_t>(l_errMask, 0, 32);
                FAPI_TRY(FAPI_ATTR_SET(fapi2::ATTR_CORE_PPM_ERRMASK, l_core_chplt,
                                       l_errMask),
                         "ERROR: Failed to set CORE PPM error Mask");

                // Set parameter provided value to Core PPM ERROR MASK
                FAPI_INF(" Write CORE PPM ERROR MASK ");
                l_data64.flush<0>().insertFromRight<0, 32>(i_cppmErrMask);
                FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                         "ERROR: Failed to write to CORE PPM Error Mask");

                // Allow PCB Access
                // Set bit 0 : PPM_WRITE_DISABLE
                FAPI_INF("Allow PCB access to PPM");
                l_data64.flush<0>().setBit<0>();
                l_address =  C_CPPM_CPMMR_CLEAR;
                FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                         "ERROR: Failed to allow PCB access to PPM");

                // Clear Core PPM Errors
                l_data64.flush<0>();
                FAPI_INF("Clear CORE PPM ERROR Register");
                l_address = C_CPPM_ERR;
                FAPI_TRY(fapi2::putScom(l_core_chplt, l_address, l_data64),
                         "ERROR: Failed to clear CORE PPM ERROR");
            }
        }
    }

    //TODO: RTC 149078: Disable Resonant Clocks
    //TODO: RTC 149079: Disable IVRM

fapi_try_exit:
    return fapi2::current_err;
}
