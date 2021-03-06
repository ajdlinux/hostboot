/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/nest/p9_build_smp.H $      */
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
/// @file p9_build_smp.H
/// @brief Perform fabric configuration (FAPI2)
///
/// Perform fabric SMP build/reconfiguration operations.
///
/// Platform Notes:
///     This HWP has multiple IPL use cases.  In all all cases the HWP input
///     is expected to contain an entry for each chip within the scope of
///     the new SMP to be constructed (with valid attribute state repesenting all
///     active links that are fully contained within the new SMP).
///
///     The p9_build_smp_operation HWP input defines the desired
///     reconfiguration option to be performed:
///
///         SMP_ACTIVATE_PHASE1 (HBI):
///             o program FBC configuration dependent registers (switch C/D)
///             o join all single chip 'island' fabrics into drawer level
///               SMP (switch A/B)
///
///         SMP_ACTIVATE_PHASE2 (FSP):
///             o join collection of drawer level SMPs into full system SMP
///               (switch A/B)
///
/// *HWP HWP Owner: Joe McGill <jmcgill@us.ibm.com>
/// *HWP FW Owner: Thi Tran <thi@us.ibm.com>
/// *HWP Team: Nest
/// *HWP Level: 3
/// *HWP Consumed by: HB,FSP
///

#ifndef _P9_BUILD_SMP_H_
#define _P9_BUILD_SMP_H_


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <fapi2.H>
#include <map>


//------------------------------------------------------------------------------
// Constant definitions
//------------------------------------------------------------------------------
const uint8_t P9_BUILD_SMP_MAX_SIZE = 16;

//------------------------------------------------------------------------------
// Structure definitions
//------------------------------------------------------------------------------

// HWP argument, define supported execution modes
enum p9_build_smp_operation
{
    // used to initialize scope of HBI drawer
    // call from HB (switch C/D + A/B),
    SMP_ACTIVATE_PHASE1 = 1,
    // used to stitch drawers
    // call from FSP (only switch A/B)
    SMP_ACTIVATE_PHASE2 = 2
};

// Structure to represent fabric connectivty & properites for a single chip
// in the SMP topology
struct p9_build_smp_chip
{
    // associated target handle from HWP input vector
    fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>* target;

    // fabric chip/node ID
    uint8_t chip_id;
    uint8_t group_id;

    // node/system master designation (curr)
    bool master_chip_group_curr;
    bool master_chip_sys_curr;

    // node/system master designation (next)
    bool master_chip_group_next;
    bool master_chip_sys_next;
    bool issue_quiesce_next;
    bool quiesced_next;
};

// Structure to represent properties for a single group in the SMP topology
struct p9_build_smp_group
{
    // chips which reside in this node
    std::map<uint8_t, p9_build_smp_chip> chips;

    // node properties/attributes:
    // fabric node ID
    uint8_t group_id;
};

// Structure to represent collection of groups in SMP topology
struct p9_build_smp_system
{
    // groups which reside in this SMP
    std::map<uint8_t, p9_build_smp_group> groups;
};

/// function pointer typedef definition for HWP call support
typedef fapi2::ReturnCode (*p9_build_smp_FP_t)
(std::vector<fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>>&,
 const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>&,
 const p9_build_smp_operation);


//------------------------------------------------------------------------------
// Function prototypes
//------------------------------------------------------------------------------

extern "C"
{

///
/// @brief Perform fabric SMP reconfiguration operation
///
/// @param[in] i_chips                  Vector of processor chip targets to be included in SMP
/// @param[in] i_master_chip_sys_next   Target designating chip which should be designated fabric
///                                     system master post-reconfiguration
///                                     NOTE: this chip must currently be designated a
///                                       master in its enclosing fabric
///                                       PHASE1/HB: any chip
///                                       PHASE2/FSP: any current drawer master
/// @param[in] i_op                     Enumerated type representing SMP build phase (HB or FSP)
///
/// @return fapi2:ReturnCode. FAPI2_RC_SUCCESS if success, else error code.
///
    fapi2::ReturnCode p9_build_smp(std::vector<fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>>& i_chips,
                                   const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_master_chip_sys_next,
                                   const p9_build_smp_operation i_op);

} // extern "C"

#endif // _P9_BUILD_SMP_H_
