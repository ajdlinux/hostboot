/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/memory/p9_mss_draminit.C $ */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2017                        */
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
/// @file p9_mss_draminit.C
/// @brief Initialize dram
///
// *HWP HWP Owner: Brian Silver <bsilver@us.ibm.com>
// *HWP HWP Backup: Andre Marin <aamarin@us.ibm.com>
// *HWP Team: Memory
// *HWP Level: 2
// *HWP Consumed by: FSP:HB

#include <fapi2.H>
#include <mss.H>

#include <p9_mss_draminit.H>
#include <lib/utils/count_dimm.H>
#include <lib/utils/conversions.H>
#include <lib/dimm/bcw_load.H>
#include <lib/workarounds/dqs_align_workarounds.H>

using fapi2::TARGET_TYPE_MCBIST;
using fapi2::TARGET_TYPE_MCA;
using fapi2::TARGET_TYPE_DIMM;
using fapi2::FAPI2_RC_SUCCESS;

extern "C"
{
    ///
    /// @brief Initialize dram
    /// @param[in] i_target, the McBIST of the ports of the dram you're initializing
    /// @return FAPI2_RC_SUCCESS iff ok
    ///
    fapi2::ReturnCode p9_mss_draminit( const fapi2::Target<TARGET_TYPE_MCBIST>& i_target )
    {
        fapi2::buffer<uint64_t> l_data;

        mss::ccs::instruction_t<TARGET_TYPE_MCBIST> l_inst;
        mss::ccs::instruction_t<TARGET_TYPE_MCBIST> l_des = mss::ccs::des_command<TARGET_TYPE_MCBIST>();

        mss::ccs::program<TARGET_TYPE_MCBIST> l_program;

        // Up, down P down, up N. Somewhat magic numbers - came from Centaur and proven to be the
        // same on Nimbus. Why these are what they are might be lost to time ...
        constexpr uint64_t PCLK_INITIAL_VALUE = 0b10;
        constexpr uint64_t NCLK_INITIAL_VALUE = 0b01;

        const auto l_mca = mss::find_targets<TARGET_TYPE_MCA>(i_target);

        FAPI_INF("Start draminit: %s", mss::c_str(i_target));

        // If we don't have any ports, lets go.
        if (l_mca.size() == 0)
        {
            FAPI_INF("++++ No ports? %s ++++", mss::c_str(i_target));
            return fapi2::FAPI2_RC_SUCCESS;
        }

        // If we don't have any DIMM, lets go.
        if (mss::count_dimm(i_target) == 0)
        {
            FAPI_INF("++++ NO DIMM on %s ++++", mss::c_str(i_target));
            return fapi2::FAPI2_RC_SUCCESS;
        }

        // Configure the CCS engine. Since this is a chunk of MCBIST logic, we don't want
        // to do it for every port. If we ever break this code out so f/w can call draminit
        // per-port (separate threads) we'll need to provide them a way to set this up before
        // sapwning per-port threads.
        {
            fapi2::buffer<uint64_t> l_ccs_config;

            FAPI_TRY( mss::ccs::read_mode(i_target, l_ccs_config) );

            // It's unclear if we want to run with this true or false. Right now (10/15) this
            // has to be false. Shelton was unclear if this should be on or off in general BRS
            mss::ccs::stop_on_err(i_target, l_ccs_config, mss::LOW);
            mss::ccs::ue_disable(i_target, l_ccs_config, mss::LOW);
            mss::ccs::copy_cke_to_spare_cke(i_target, l_ccs_config, mss::HIGH);
            mss::ccs::parity_after_cmd(i_target, l_ccs_config, mss::HIGH);

            FAPI_TRY( mss::ccs::write_mode(i_target, l_ccs_config) );
        }

        // We initialize dram by iterating over the (ungarded) ports. We could allow the caller
        // to initialize each port's dram on a separate thread if we could synchronize access
        // to the MCBIST (CCS engine.) Right now we can't, so we'll do it this way.

        //
        // We expect to come in to draminit with the following setup:
        // 1. ENABLE_RESET_N (FARB5Q(6)) 0
        // 2. RESET_N (FARB5Q(4)) 0 - driving reset
        // 3. CCS_ADDR_MUX_SEL (FARB5Q(5)) - 1
        // 4. CKE out of high impedence
        //
        for (const auto& p : l_mca)
        {
            FAPI_TRY( mss::draminit_entry_invariant(p) );

            // Begin driving mem clks, and wait 10ns (we'll do this outside the loop)
            // From the RCD Spec, before the DRST_n (resetn) input is pulled HIGH the
            // clock input signal must be stable.
            FAPI_TRY( mss::drive_mem_clks(p, PCLK_INITIAL_VALUE, NCLK_INITIAL_VALUE) );

            // After RESET_n is de-asserted, wait for another 500us until CKE becomes active.
            // During this time, the DRAM will start internal initialization; this will be done
            // independently of external clocks.
            FAPI_TRY( mss::ddr_resetn(p, mss::HIGH) );
        }

        // From the DDR4 JEDEC Spec (79-A): Power-up Initialization Sequence
        {
            // Clocks (CK_t,CK_c) need to be started and stable for 10ns or 5tCK
            // (whichever is greater) before CKE goes active.
            // Doing this once here than twice in drive_mem_clks
            constexpr uint64_t DELAY_5TCK = 5;
            const uint64_t l_delay_in_ns = std::max( static_cast<uint64_t>(mss::DELAY_10NS),
                                           mss::cycles_to_ns(i_target, DELAY_5TCK) );

            const uint64_t l_delay_in_cycles = mss::ns_to_cycles(i_target, l_delay_in_ns);

            // Set our delay (for HW and SIM)
            FAPI_TRY( fapi2::delay(l_delay_in_ns, mss::cycles_to_simcycles(l_delay_in_cycles)) );
        }

        // Also a Deselect command must be registered as required from the Spec.
        // Register DES instruction, which pulls CKE high. Idle 400 cycles, and then begin RCD loading
        // Note: This only is sent to one of the MCA as we still have the mux_addr_sel bit set, meaning
        // we'll PDE/DES all DIMM at the same time.
        l_des.arr1.insertFromRight<MCBIST_CCS_INST_ARR1_00_IDLES, MCBIST_CCS_INST_ARR1_00_IDLES_LEN>(400);
        l_program.iv_instructions.push_back(l_des);
        FAPI_TRY( mss::ccs::execute(i_target, l_program, l_mca[0]) );

        // Per conversation with Shelton and Steve 10/9/15, turn off addr_mux_sel after the CKE CCS but
        // before the RCD/MRS CCSs
        for (const auto& p : l_mca)
        {
            FAPI_TRY( change_addr_mux_sel(p, mss::LOW) );
        }

        // Load RCD control words
        FAPI_TRY( mss::rcd_load(i_target) );

        // Load data buffer control words (BCW)
        FAPI_TRY( mss::bcw_load(i_target) );

        // Load MRS
        FAPI_TRY( mss::mrs_load(i_target) );

    fapi_try_exit:
        FAPI_INF("End draminit: %s (0x%lx)", mss::c_str(i_target), uint64_t(fapi2::current_err));
        return fapi2::current_err;
    }
}
