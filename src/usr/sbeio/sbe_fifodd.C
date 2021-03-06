/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/sbeio/sbe_fifodd.C $                                  */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2012,2017                        */
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
/**
 * @file sbe_fifodd.C
 * @brief SBE FIFO device driver
 */

#include <sys/time.h>
#include <trace/interface.H>
#include <devicefw/driverif.H>
#include <errl/errlentry.H>
#include <errl/errlmanager.H>
#include <errl/errludtarget.H>
#include <targeting/common/target.H>
#include <targeting/common/targetservice.H>
#include <sbeio/sbeioreasoncodes.H>
#include <errl/errlreasoncodes.H>
#include "sbe_fifodd.H"
#include <sbeio/sbe_ffdc_parser.H>
#include <initservice/initserviceif.H>
#include <kernel/pagemgr.H>
#include <fapi2.H>
#include <sbeio/sbe_sp_intf.H>

extern trace_desc_t* g_trac_sbeio;

#define SBE_TRACF(printf_string,args...) \
    TRACFCOMP(g_trac_sbeio,"fifodd: " printf_string,##args)
#define SBE_TRACD(printf_string,args...) \
    TRACDCOMP(g_trac_sbeio,"fifodd: " printf_string,##args)
#define SBE_TRACU(args...)
#define SBE_TRACFBIN(printf_string,args...) \
    TRACFBIN(g_trac_sbeio,"fifodd: " printf_string,##args)
#define SBE_TRACDBIN(printf_string,args...) \
    TRACDBIN(g_trac_sbeio,"fifodd: " printf_string,##args)
/* replace for unit testing
#define SBE_TRACU(printf_string,args...) \
    TRACFCOMP(g_trac_sbeio,"fifodd: " printf_string,##args)
*/
#define READ_BUFFER_SIZE 2048

using namespace ERRORLOG;

namespace SBEIO
{

SbeFifo & SbeFifo::getTheInstance()
{
    return Singleton<SbeFifo>::instance();
}

/**
 * @brief  Constructor
 */
SbeFifo::SbeFifo()
{
    iv_ffdcPackageBuffer = PageManager::allocatePage(ffdcPackageSize, true);
    initFFDCPackageBuffer();
}

/**
 * @brief  Destructor
 */
SbeFifo::~SbeFifo()
{
    if(iv_ffdcPackageBuffer != NULL)
    {
        PageManager::freePage(iv_ffdcPackageBuffer);
    }
}

/**
 * @brief perform SBE FIFO chip-op
 */
errlHndl_t SbeFifo::performFifoChipOp(TARGETING::Target * i_target,
                             uint32_t          * i_pFifoRequest,
                             uint32_t          * i_pFifoResponse,
                             uint32_t            i_responseSize)
{
    errlHndl_t errl = NULL;
    static mutex_t l_fifoOpMux = MUTEX_INITIALIZER;

    SBE_TRACD(ENTER_MRK "performFifoChipOp");

    //Serialize access to PSU
    mutex_lock(&l_fifoOpMux);

    do
    {
        errl = writeRequest(i_target,
                            i_pFifoRequest);
        if (errl) break;  // return with error

        errl = readResponse(i_target,
                            i_pFifoRequest,
                            i_pFifoResponse,
                            i_responseSize);
        if (errl) break;  // return with error

    }
    while (0);

    mutex_unlock(&l_fifoOpMux);

    if( errl && (SBEIO_COMP_ID == errl->moduleId()) )
    {
        SBE_TRACF( "Forcing shutdown for FSP to collect FFDC" );

        //commit the original error after pulling some data out
        uint32_t orig_plid = errl->plid();
        uint32_t orig_rc = errl->reasonCode();
        uint32_t orig_mod = errl->moduleId();
        ERRORLOG::errlCommit( errl, SBEIO_COMP_ID );
        /*@
         * @errortype
         * @moduleid     SBEIO_FIFO
         * @reasoncode   SBEIO_HWSV_COLLECT_SBE_RC
         * @userdata1    PLID of original error log
         * @userdata2[00:31]    Original RC
         * @userdata2[32:63]    Original Module Id
         *
         * @devdesc      SBE error, force HWSV to collect FFDC
         * @custdesc     Firmware error communicating with boot device
         */
        errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                             SBEIO_FIFO,
                             SBEIO_HWSV_COLLECT_SBE_RC,
                             orig_plid,
                             TWO_UINT32_TO_UINT64(orig_rc,orig_mod));
        INITSERVICE::doShutdownWithError( SBEIO_HWSV_COLLECT_SBE_RC,
                                          TARGETING::get_huid(i_target) );
    }

    SBE_TRACD(EXIT_MRK "performFifoChipOp");

    return errl;
}


/**
 * @brief perform SBE FIFO Reset
 */
errlHndl_t SbeFifo::performFifoReset(TARGETING::Target * i_target)
{
    errlHndl_t errl = NULL;
    static mutex_t l_fifoOpMux = MUTEX_INITIALIZER;

    SBE_TRACF(ENTER_MRK "sending FSI SBEFIFO Reset to HUID 0x%x",
              TARGETING::get_huid(i_target));

    //Serialize access to the FIFO
    mutex_lock(&l_fifoOpMux);

    // Perform a write to the DNFIFO Reset to cleanup the fifo
    uint32_t l_dummy = 0xDEAD;
    errl = writeFsi(i_target,SBE_FIFO_DNFIFO_RESET,&l_dummy);

    mutex_unlock(&l_fifoOpMux);

    return errl;
}


/**
 * @brief write FIFO request message
 */
errlHndl_t SbeFifo::writeRequest(TARGETING::Target * i_target,
                        uint32_t * i_pFifoRequest)
{
    errlHndl_t errl = NULL;

    SBE_TRACD(ENTER_MRK "writeRequest");

    do
    {
        // Ensure Downstream Max Transfer Counter is 0 since
        // hostboot has no need for it (non-0 can cause
        // protocol issues)
        uint64_t l_addr       = SBE_FIFO_DNFIFO_MAX_TSFR;
        uint32_t l_data       = 0;
        errl = writeFsi(i_target,l_addr,&l_data);
        if (errl) break;

        //The first uint32_t has the number of uint32_t words in the request
        l_addr                = SBE_FIFO_UPFIFO_DATA_IN;
        uint32_t * l_pSent    = i_pFifoRequest; //advance as words sent
        uint32_t   l_cnt      = *l_pSent;
        SBE_TRACDBIN("Write Request in SBEIO",i_pFifoRequest,
                     l_cnt*sizeof(*l_pSent));
        for (uint32_t i=0;i<l_cnt;i++)
        {
            // Wait for room to write into fifo
            errl = waitUpFifoReady(i_target);
            if (errl) break;

            // Send data into fifo
            errl = writeFsi(i_target,l_addr,l_pSent);
            if (errl) break;

            l_pSent++;
        }
        if (errl) break;

        //notify SBE that last word has been sent
        errl = waitUpFifoReady(i_target);
        if (errl) break;

        l_addr = SBE_FIFO_UPFIFO_SIG_EOT;
        l_data = FSB_UPFIFO_SIG_EOT;
        errl = writeFsi(i_target,l_addr,&l_data);
        if (errl) break;

    }
    while (0);

    SBE_TRACD(EXIT_MRK "writeRequest");

    return errl;
}

/**
 * @brief wait for room in upstream fifo to send data
 */
errlHndl_t SbeFifo::waitUpFifoReady(TARGETING::Target * i_target)
{
    errlHndl_t errl = NULL;

    SBE_TRACD(ENTER_MRK "waitUpFifoReady");

    uint64_t l_elapsed_time_ns = 0;
    uint64_t l_addr = SBE_FIFO_UPFIFO_STATUS;
    uint32_t l_data = 0;

    do
    {
        // read upstream status to see if room for more data
        errl = readFsi(i_target,l_addr,&l_data);
        if (errl) break;

        if ( !(l_data & UPFIFO_STATUS_FIFO_FULL) )
        {
            break;
        }

        // time out if wait too long
        if (l_elapsed_time_ns >= MAX_UP_FIFO_TIMEOUT_NS )
        {
            SBE_TRACF(ERR_MRK "waitUpFifoReady: "
                      "timeout waiting for upstream FIFO to be not full");

            /*@
             * @errortype
             * @moduleid     SBEIO_FIFO
             * @reasoncode   SBEIO_FIFO_UPSTREAM_TIMEOUT
             * @userdata1    Timeout in NS
             * @devdesc      Timeout waiting for upstream FIFO to have
             *               room to write
             */
            errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                                 SBEIO_FIFO,
                                 SBEIO_FIFO_UPSTREAM_TIMEOUT,
                                 MAX_UP_FIFO_TIMEOUT_NS,
                                 0);

            errl->addProcedureCallout(HWAS::EPUB_PRC_HB_CODE,
                                      HWAS::SRCI_PRIORITY_HIGH);
            errl->addHwCallout(  i_target,
                                 HWAS::SRCI_PRIORITY_LOW,
                                 HWAS::NO_DECONFIG,
                                 HWAS::GARD_NULL );
            errl->collectTrace(SBEIO_COMP_NAME);
            break;
        }

        // try later
        nanosleep( 0, 10000 ); //sleep for 10,000 ns
        l_elapsed_time_ns += 10000;
    }
    while (1);

    SBE_TRACD(EXIT_MRK "waitUpFifoReady");

    return errl;
}

/**
 * @brief Read FIFO response messages
 */
errlHndl_t SbeFifo::readResponse(TARGETING::Target * i_target,
                        uint32_t * i_pFifoRequest,
                        uint32_t * o_pFifoResponse,
                        uint32_t   i_responseSize)
{
    errlHndl_t errl = NULL;
    uint32_t l_readBuffer[READ_BUFFER_SIZE];

    SBE_TRACD(ENTER_MRK "readResponse");

    do
    {
        // EOT is expected before the response buffer is full. Room for
        // the PCBPIB status or FFDC is included, but is only returned
        // if there is an error. The last received word has the distance
        // to the status, which is placed at the end of the returned data
        // in order to reflect errors during transfer.

        uint32_t * l_pReceived = o_pFifoResponse; //advance as words received
        uint32_t   l_maxWords  =  i_responseSize / sizeof(uint32_t);
                                    //Number of words in response buffer.
        uint32_t   l_recWords  = 0; //Number of words received.
                                    //Used validate "distance" to status
                                    //and pointer to status.
        bool       l_EOT      = false;
        uint32_t   l_last     = 0; // last word received. The "current" word.
                                   // Final read is "distance" in words to
                                   // status header including the final
                                   // distance word.
        // receive words until EOT, but do not exceed response buffer size
        bool       l_overRun  = false;

        do
        {
            // Wait for data to be ready to receive (download) or if the EOT
            // has been sent. If not EOT, then data ready to receive.
            uint32_t l_status = 0;
            errl = waitDnFifoReady(i_target,l_status);

            if (l_status & DNFIFO_STATUS_DEQUEUED_EOT_FLAG)
            {
                l_EOT = true;
                // ignore EOT dummy word
                if (l_recWords >= (sizeof(statusHeader)/sizeof(uint32_t)) )
                {
                    if(l_overRun == false) {
                        l_pReceived--;
                        l_recWords--;
                        l_last = o_pFifoResponse[l_recWords-1];
                    } else {
                        l_last = (uint32_t) l_readBuffer[l_recWords-2];
                    }
                }
                break;
            }

            // When error occurs, SBE will write more than l_maxWords
            // we have to keep reading 1 word at a time until we get EOT
            // or more than READ_BUFFER_SIZE. Save what we read in the buffer
            if (l_recWords >= l_maxWords)
            {
                l_overRun = true;
            }

            // read next word
            errl = readFsi(i_target,SBE_FIFO_DNFIFO_DATA_OUT,&l_last);
            if (errl) break;

            l_readBuffer[l_recWords] = l_last;

            if(l_overRun == false) {
                *l_pReceived = l_last; //copy to returned output buffer
                l_pReceived++; //advance to next position
            }
            l_recWords++;  //count word received
            SBE_TRACD("Read a byte from data reg: 0x%.8X",l_last);
            if(l_recWords > READ_BUFFER_SIZE) {
                SBE_TRACF(ERR_MRK "readResponse: data overflow without EOT");
                break;
            }
        }
        while (1); // exit check in middle of loop
        if (errl) break;

        // At this point,
        // l_recWords of words received.
        // l_pReceived points to 1 word past last word received.
        // l_last has last word received, which is "distance" to status
        // EOT is expected before running out of response buffer
        if (!l_EOT)
        {
            SBE_TRACF(ERR_MRK "readResponse: no EOT cmd=0x%08x size=%d",
                      i_pFifoRequest[1],i_responseSize);

            /*@
             * @errortype
             * @moduleid     SBEIO_FIFO
             * @reasoncode   SBEIO_FIFO_NO_DOWNSTREAM_EOT
             * @userdata1    FIFO command class and command
             * @userdata2    Response buffer size
             * @devdesc      EOT not received before downstream buffer full.
             */
            errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                                 SBEIO_FIFO,
                                 SBEIO_FIFO_NO_DOWNSTREAM_EOT,
                                 i_pFifoRequest[1],
                                 i_responseSize);


            errl->addProcedureCallout(HWAS::EPUB_PRC_HB_CODE,
                                      HWAS::SRCI_PRIORITY_HIGH);
            errl->addHwCallout(  i_target,
                                 HWAS::SRCI_PRIORITY_LOW,
                                 HWAS::NO_DECONFIG,
                                 HWAS::GARD_NULL );
            errl->collectTrace(SBEIO_COMP_NAME);
            break;
        }

        //notify that EOT has been received
        uint32_t l_eotSig = FSB_UPFIFO_SIG_EOT;
        errl = writeFsi(i_target,SBE_FIFO_DNFIFO_ACK_EOT,&l_eotSig);
        if (errl) break;

        //Determine if successful.
        //Last word received has distance to status in words including itself.
        //l_recWords has number of words received.
        //Need to have received at least status header and distance word.
        if ( (l_last      < (sizeof(statusHeader)/sizeof(uint32_t) + 1)) ||
             (l_recWords  < (sizeof(statusHeader)/sizeof(uint32_t) + 1)) ||
             (l_last      > (l_recWords)) )
        {
            SBE_TRACF(ERR_MRK "readResponse: invalid status distance "
                      "cmd=0x%08x distance=%d allocated response size=%d "
                      "received word size=%d" ,
                      i_pFifoRequest[1],
                      l_last,
                      i_responseSize,
                      l_recWords);

            SBE_TRACFBIN("Invalid Response from SBE",
                         o_pFifoResponse,l_recWords*sizeof(l_last));

            /*@
             * @errortype
             * @moduleid     SBEIO_FIFO
             * @reasoncode   SBEIO_FIFO_INVALID_STATUS_DISTANCE
             * @userdata1    FIFO command class and command
             * @userdata2[0:15]   Distance to status in words
             * @userdata2[16:31]  Bytes received
             * @userdata2[32:63]  Response buffer size
             * @devdesc      The distance to the status header is not
             *               within the response buffer.
             */
            errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                                 SBEIO_FIFO,
                                 SBEIO_FIFO_INVALID_STATUS_DISTANCE,
                                 i_pFifoRequest[1],
                                 TWO_UINT32_TO_UINT64(
                                    TWO_UINT16_TO_UINT32(l_last,
                                         l_recWords*sizeof(uint32_t)),
                                    i_responseSize));

            errl->addProcedureCallout(HWAS::EPUB_PRC_HB_CODE,
                                      HWAS::SRCI_PRIORITY_HIGH);
            errl->addHwCallout(  i_target,
                                 HWAS::SRCI_PRIORITY_LOW,
                                 HWAS::NO_DECONFIG,
                                 HWAS::GARD_NULL );
            errl->collectTrace(SBEIO_COMP_NAME);
            break;
        }

        // Check status for success.
        // l_pReceived points one word past last word received.
        // l_last has number of words to status header including self.

        uint32_t * l_pStatusTmp = (l_overRun == false) ?
            l_pReceived - l_last : //do word ptr math
            &l_readBuffer[l_recWords - 1];
        statusHeader * l_pStatusHeader = (statusHeader *)l_pStatusTmp;
        if ((FIFO_STATUS_MAGIC != l_pStatusHeader->magic) ||
            (SBE_PRI_OPERATION_SUCCESSFUL != l_pStatusHeader->primaryStatus) ||
            (SBE_SEC_OPERATION_SUCCESSFUL != l_pStatusHeader->secondaryStatus))
        {
            SBE_TRACF(ERR_MRK "readResponse: failing downstream status "
                      " cmd=0x%08x magic=0x%08x prim=0x%08x secondary=0x%08x",
                      i_pFifoRequest[1],
                      l_pStatusHeader->magic,
                      l_pStatusHeader->primaryStatus,
                      l_pStatusHeader->secondaryStatus);

            /*@
             * @errortype
             * @moduleid     SBEIO_FIFO
             * @reasoncode   SBEIO_FIFO_RESPONSE_ERROR
             * @userdata1    FIFO command class and command
             * @userdata2[0:31]   Should be magic value 0xC0DE
             * @userdata1[32:47]  Primary Status
             * @userdata1[48:63]  Secondary Status
             *
             * @devdesc  Status header does not start with magic number or
             *           non-zero primary or secondary status
             */

            errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                                 SBEIO_FIFO,
                                 SBEIO_FIFO_RESPONSE_ERROR,
                                 i_pFifoRequest[1],
                                 FOUR_UINT16_TO_UINT64(
                                         0,
                                         l_pStatusHeader->magic,
                                         l_pStatusHeader->primaryStatus,
                                         l_pStatusHeader->secondaryStatus));

            /*
             * The FFDC package should start at l_pReceived.
             * Size of the FFDC package should be l_maxWords - l_recWords
             */

            if(l_overRun == false) {
                writeFFDCBuffer(l_pReceived,
                            sizeof(uint32_t) * (l_maxWords - l_recWords - 1));
            } else {
                // If Overrun, FFDC should be
                // l_recWords (words read) - l_last (distance to status) + 1
                // in l_readBuffer
                writeFFDCBuffer(&l_readBuffer[l_recWords - l_last + 1],
                            sizeof(uint32_t) * (l_last + 1));
            }
            SbeFFDCParser * l_ffdc_parser = new SbeFFDCParser();
            l_ffdc_parser->parseFFDCData(iv_ffdcPackageBuffer);

            // Go through the buffer, get the RC
            uint8_t l_pkgs = l_ffdc_parser->getTotalPackages();
            for(uint8_t i = 0; i < l_pkgs; i++)
            {
                 uint32_t l_rc = l_ffdc_parser->getPackageRC(i);
                 // If fapiRC, add data to errorlog
                 if(l_rc ==  fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA)
                 {
                     errl->addFFDC( SBEIO_COMP_ID,
                                    l_ffdc_parser->getFFDCPackage(i),
                                    l_ffdc_parser->getPackageLength(i),
                                    0,
                                    SBEIO_UDT_PARAMETERS,
                                    false );
                 }
                 else
                 {
                     using namespace fapi2;

                     fapi2::ReturnCode l_fapiRC;

                     /*
                      * Put FFDC into sbeFfdc_t struct and
                      * call FAPI_SET_SBE_ERROR
                      */
                     sbeFfdc_t * l_ffdcBuf = new sbeFfdc_t();
                     l_ffdcBuf->size = l_ffdc_parser->getPackageLength(i);
                     l_ffdcBuf->data = reinterpret_cast<uint64_t>(
                                           l_ffdc_parser->getFFDCPackage(i));

                     FAPI_SET_SBE_ERROR(l_fapiRC,
                                l_rc,
                                l_ffdcBuf,
                                i_target->getAttr<TARGETING::ATTR_FAPI_POS>());

                     errlHndl_t sbe_errl = fapi2::rcToErrl(l_fapiRC);
                     if( sbe_errl )
                     {
                         ERRORLOG::errlCommit( sbe_errl, SBEIO_COMP_ID );
                     }
                 }
            }

            errl->addProcedureCallout(HWAS::EPUB_PRC_HB_CODE,
                                      HWAS::SRCI_PRIORITY_HIGH);
            errl->addHwCallout(  i_target,
                                 HWAS::SRCI_PRIORITY_LOW,
                                 HWAS::NO_DECONFIG,
                                 HWAS::GARD_NULL );
            errl->collectTrace(SBEIO_COMP_NAME);

            delete  l_ffdc_parser;
            break;
        }
    }
    while (0);

    SBE_TRACD(EXIT_MRK "readResponse");

    return errl;
}

/**
 * @brief wait for data in downstream fifo to receive
 *        or hit EOT.
 *
 *        On return, either a valid word is ready to read,
 *        or the EOT will be set in the returned doorbell status.
 */
errlHndl_t SbeFifo::waitDnFifoReady(TARGETING::Target * i_target,
                           uint32_t          & o_status)
{
    errlHndl_t errl = NULL;

    SBE_TRACD(ENTER_MRK "waitDnFifoReady");

    uint64_t l_elapsed_time_ns = 0;
    uint64_t l_addr = SBE_FIFO_DNFIFO_STATUS;

    do
    {
        // read dnstream status to see if data ready to be read
        // or if has hit the EOT
        errl = readFsi(i_target,l_addr,&o_status);
        if (errl) break;

        if (  (!(o_status & DNFIFO_STATUS_FIFO_EMPTY)) ||
              (o_status & DNFIFO_STATUS_DEQUEUED_EOT_FLAG) )
        {
            SBE_TRACD("Read a word from status register: 0x%.8X",o_status);
            break;
        }
        else
        {
            SBE_TRACD("SBE status reg returned fifo empty or dequeued eot flag 0x%.8X",
                      o_status);
        }

        // time out if wait too long
        if (l_elapsed_time_ns >= MAX_UP_FIFO_TIMEOUT_NS )
        {
            SBE_TRACF(ERR_MRK "waitDnFifoReady: "
                      "timeout waiting for downstream FIFO to be not full");

            /*@
             * @errortype
             * @moduleid     SBEIO_FIFO
             * @reasoncode   SBEIO_FIFO_DOWNSTREAM_TIMEOUT
             * @userdata1    Timeout in NS
             * @devdesc      Timeout waiting for downstream FIFO to have
             *               data to receive
             */

            errl = new ErrlEntry(ERRL_SEV_UNRECOVERABLE,
                                SBEIO_FIFO,
                                SBEIO_FIFO_DOWNSTREAM_TIMEOUT,
                                MAX_UP_FIFO_TIMEOUT_NS,
                                0);

            errl->addProcedureCallout(HWAS::EPUB_PRC_HB_CODE,
                                      HWAS::SRCI_PRIORITY_HIGH);

            errl->addHwCallout(  i_target,
                                 HWAS::SRCI_PRIORITY_HIGH,
                                 HWAS::NO_DECONFIG,
                                 HWAS::GARD_NULL );

            errl->collectTrace(SBEIO_COMP_NAME);
            break;
        }

        // try later
        nanosleep( 0, 10000 ); //sleep for 10,000 ns
        l_elapsed_time_ns += 10000;
    }
    while (1);

    SBE_TRACD(EXIT_MRK "waitDnFifoReady");

    return errl;
}

/**
 * @brief read FSI
 */
errlHndl_t SbeFifo::readFsi(TARGETING::Target * i_target,
                     uint64_t   i_addr,
                     uint32_t * o_pData)
{
    errlHndl_t errl = NULL;

    size_t l_32bitSize = sizeof(uint32_t);
    errl = deviceOp(DeviceFW::READ,
                    i_target,
                    o_pData,
                    l_32bitSize,
                    DEVICE_FSI_ADDRESS(i_addr));

    SBE_TRACU("  readFsi addr=0x%08lx data=0x%08x",
                         i_addr,*o_pData);

    return errl;
}

/**
 * @brief write FSI
 */
errlHndl_t SbeFifo::writeFsi(TARGETING::Target * i_target,
                     uint64_t   i_addr,
                     uint32_t * i_pData)
{
    errlHndl_t errl = NULL;

    SBE_TRACU("  writeFsi addr=0x%08lx data=0x%08x",
                         i_addr,*i_pData);
    size_t l_32bitSize = sizeof(uint32_t);
    errl = deviceOp(DeviceFW::WRITE,
                    i_target,
                    i_pData,
                    l_32bitSize,
                    DEVICE_FSI_ADDRESS(i_addr));

    return errl;
}

/**
 * @brief zero out FFDC Package Buffer
 */

void SbeFifo::initFFDCPackageBuffer()
{
    memset(iv_ffdcPackageBuffer, 0x00, PAGESIZE * ffdcPackageSize);
}

/**
 * @brief populate FFDC package buffer
 * @param[in]  i_data        FFDC error data
 * @param[in]  i_len         data buffer len to copy
 */
void SbeFifo::writeFFDCBuffer( void * i_data, uint32_t i_len) {
    if(i_len <= PAGESIZE * ffdcPackageSize)
    {
        initFFDCPackageBuffer();
        memcpy(iv_ffdcPackageBuffer, i_data, i_len);
    }
    else
    {
        SBE_TRACF(ERR_MRK"writeFFDCBuffer: Buffer size too large: %d",
                      i_len);
    }
}

} //end of namespace SBEIO
