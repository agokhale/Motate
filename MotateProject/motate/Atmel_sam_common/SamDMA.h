/*
 utility/SamDMA.h - Library for the Motate system
 http://github.com/synthetos/motate/

 Copyright (c) 2013 - 2016 Robert Giseburt

 This file is part of the Motate Library.

 This file ("the software") is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2 as published by the
 Free Software Foundation. You should have received a copy of the GNU General Public
 License, version 2 along with the software. If not, see <http://www.gnu.org/licenses/>.

 As a special exception, you may use this file as part of a software library without
 restriction. Specifically, if other files instantiate templates or use macros or
 inline functions from this file, or you compile this file and link it with  other
 files to produce an executable, this file does not by itself cause the resulting
 executable to be covered by the GNU General Public License. This exception does not
 however invalidate any other reasons why the executable file might be covered by the
 GNU General Public License.

 THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SAMDMA_H_ONCE
#define SAMDMA_H_ONCE

#include "SamCommon.h" // pull in defines and fix them
#include <type_traits> // for std::alignment_of and std::remove_pointer

namespace Motate {

    // DMA template - MUST be specialized
    template<typename periph_t, uint8_t periph_num>
    struct DMA {
        DMA() = delete; // this prevents accidental direct instantiation
    };


    // So far there are two proimary types of DMA that we support:
    // PDC (Peripheral DMA Controller), which is the DMA built into many
    //   peripherals of the sam3x* and sam4e* lines.
    // XDMAC (eXtensible DMA), which is the global DMA controller of the
    //   SAMS70 (and family).


// PDC peripherals -- if we have a PDC (deduced using PERIPH_PTSR_RXTEN)
#ifdef HAS_PDC

#pragma mark DMA_PDC implementation

    // DMA_PDC_hardware template - - MUST be specialized
    template<typename periph_t, uint8_t periph_num>
    struct DMA_PDC_hardware {
        DMA_PDC_hardware() = delete; // this prevents accidental direct instantiation
    };


    // generic DMA_PDC object.
    template<typename periph_t, uint8_t periph_num>
    struct DMA_PDC : DMA_PDC_hardware<periph_t, periph_num>
    {
        typedef DMA_PDC_hardware<periph_t, periph_num> _hw;
        using _hw::pdc;
        using _hw::startRxDoneInterrupts;
        using _hw::stopRxDoneInterrupts;
        using _hw::startTxDoneInterrupts;
        using _hw::stopTxDoneInterrupts;
        using _hw::inTxBufferEmptyInterrupt;
        using _hw::inRxBufferEmptyInterrupt;

        typedef typename _hw::buffer_t buffer_t;

        void reset() const
        {
            pdc()->PERIPH_PTCR = PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS; // disable all the things
            pdc()->PERIPH_RPR = 0;
            pdc()->PERIPH_RNPR = 0;
            pdc()->PERIPH_RCR = 0;
            pdc()->PERIPH_RNCR = 0;
            pdc()->PERIPH_TPR = 0;
            pdc()->PERIPH_TNPR = 0;
            pdc()->PERIPH_TCR = 0;
            pdc()->PERIPH_TNCR = 0;
        };

        void disableRx() const
        {
            pdc()->PERIPH_PTCR = PERIPH_PTCR_RXTDIS; // disable for setup
        };
        void enableRx() const
        {
            pdc()->PERIPH_PTCR = PERIPH_PTCR_RXTEN;  // enable again
        };
        void setRx(void * const buffer, const uint32_t length) const
        {
            pdc()->PERIPH_RPR = (uint32_t)buffer;
            pdc()->PERIPH_RCR = length;
        };
        void setNextRx(void * const buffer, const uint32_t length) const
        {
            pdc()->PERIPH_RNPR = (uint32_t)buffer;
            pdc()->PERIPH_RNCR = length;
        };
        void flushRead() const {
            pdc()->PERIPH_RNCR = 0;
            pdc()->PERIPH_RCR = 0;
        };
        uint32_t leftToRead(bool include_next = false) const
        {
            if (include_next) {
                return pdc()->PERIPH_RCR + pdc()->PERIPH_RNCR;
            }
            return pdc()->PERIPH_RCR;
        };
        uint32_t leftToReadNext() const
        {
            return pdc()->PERIPH_RNCR;
        };
        bool doneReading(bool include_next = false) const
        {
            return leftToRead(include_next) == 0;
        };
        bool doneReadingNext() const {
            return leftToReadNext() == 0;
        };
        buffer_t getRXTransferPosition() const
        {
            return (buffer_t)pdc()->PERIPH_RPR;
        };

        // Bundle it all up
        bool startRXTransfer(void * const buffer, const uint32_t length, bool handle_interrupts = true, bool include_next = false) const
        {
            if (doneReading()) {
                if (handle_interrupts) { stopRxDoneInterrupts(); }
                setRx(buffer, length);
                if (length != 0) {
                    if (handle_interrupts) { startRxDoneInterrupts(); }
                    enableRx();
                    return true;
                }
                return false;
            }
            else if (include_next && doneReadingNext()) {
                setNextRx(buffer, length);
                return true;
            }
            return false;
        }


        void disableTx() const
        {
            pdc()->PERIPH_PTCR = PERIPH_PTCR_TXTDIS; // disable for setup
        };
        void enableTx() const
        {
            pdc()->PERIPH_PTCR = PERIPH_PTCR_TXTEN;  // enable again
        };
        void setTx(void * const buffer, const uint32_t length) const
        {
            pdc()->PERIPH_TPR = (uint32_t)buffer;
            pdc()->PERIPH_TCR = length;
        };
        void setNextTx(void * const buffer, const uint32_t length) const
        {
            pdc()->PERIPH_TNPR = (uint32_t)buffer;
            pdc()->PERIPH_TNCR = length;
        };
        uint32_t leftToWrite(bool include_next = false) const
        {
            if (include_next) {
                return pdc()->PERIPH_TCR + pdc()->PERIPH_TNCR;
            }
            return pdc()->PERIPH_TCR;
        };
        uint32_t leftToWriteNext() const
        {
            return pdc()->PERIPH_TNCR;
        };
        bool doneWriting(bool include_next = false) const
        {
            return leftToWrite(include_next) == 0;
        };
        bool doneWritingNext() const
        {
            return leftToWriteNext() == 0;
        };
        buffer_t getTXTransferPosition() const
        {
            return (buffer_t)pdc()->PERIPH_TPR;
        };


        // Bundle it all up
        bool startTXTransfer(void * const buffer, const uint32_t length, bool handle_interrupts = true, bool include_next = false) const
        {
            if (doneWriting()) {
                if (handle_interrupts) { stopTxDoneInterrupts(); }
                setTx(buffer, length);
                if (length != 0) {
                    if (handle_interrupts) { startTxDoneInterrupts(); }
                    enableTx();
                    return true;
                }
                return false;
            }
            else if (include_next && doneWritingNext()) {
                setNextTx(buffer, length);
                return true;
            }
            return false;
        }
    };


// We're deducing if there's a USART and it has a PDC
// Notice that this relies on defines set up in SamCommon.h
#ifdef HAS_PDC_USART0

#pragma mark DMA_PDC Usart implementation

    template<uint8_t uartPeripheralNumber>
    struct DMA_PDC_hardware<Usart*, uartPeripheralNumber>
    {
        // this is identical to in SamUART
        static constexpr Usart * const usart()
        {
            return (uartPeripheralNumber == 0) ? USART0 : USART1;
        };

        static constexpr Pdc * const pdc()
        {
            return (uartPeripheralNumber == 0) ? PDC_USART0 : PDC_USART1;
        };

        typedef char* buffer_t ;

        void startRxDoneInterrupts() const { usart()->US_IER = US_IER_RXBUFF; };
        void stopRxDoneInterrupts() const { usart()->US_IDR = US_IDR_RXBUFF; };
        void startTxDoneInterrupts() const { usart()->US_IER = US_IER_TXBUFE; };
        void stopTxDoneInterrupts() const { usart()->US_IDR = US_IDR_TXBUFE; };

        bool inRxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            if (usart()->US_IMR & US_IMR_RXBUFF) {
                // then we read the status register
                return (usart()->US_CSR & US_CSR_RXBUFF);
            }
            return false;
        }

        bool inTxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            if (usart()->US_IMR & US_IMR_TXBUFE) {
                // then we read the status register
                return (usart()->US_CSR & US_CSR_TXBUFE);
            }
            return false;
        }
    };

    // Construct a DMA specialization that uses the PDC
    template<uint8_t periph_num>
    struct DMA<Usart*, periph_num> : DMA_PDC<Usart*, periph_num> {
        // nothing to do here.
    };
#endif // USART + PDC

// We're deducing if there's a UART and it has a PDC
// Notice that this relies on defines set up in SamCommon.h
#ifdef HAS_PDC_UART0

#pragma mark DMA_PDC Usart implementation

    template<uint8_t uartPeripheralNumber>
    struct DMA_PDC_hardware<Uart*, uartPeripheralNumber> {
        static constexpr Uart * const uart()
        {
            return (uartPeripheralNumber == 0) ? UART0 : UART1;
        };

        typedef char* buffer_t ;

        static constexpr Pdc * const pdc()
        {
            return (uartPeripheralNumber == 0) ? PDC_UART0 : PDC_UART1;
        };

        void startRxDoneInterrupts() const { uart()->UART_IER = UART_IER_RXBUFF; };
        void stopRxDoneInterrupts() const { uart()->UART_IDR = UART_IDR_RXBUFF; };
        void startTxDoneInterrupts() const { uart()->UART_IER = UART_IER_TXBUFE; };
        void stopTxDoneInterrupts() const { uart()->UART_IDR = UART_IDR_TXBUFE; };

        bool inRxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            if (uart()->UART_IMR & UART_IMR_RXBUFF) {
                // then we read the status register
                return (uart()->UART_SR & UART_SR_RXBUFF);
            }
            return false;
        }

        bool inTxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            if (uart()->UART_IMR & UART_IMR_TXBUFE) {
                // then we read the status register
                return (uart()->UART_SR & UART_SR_TXBUFE);
            }
            return false;
        }
    };

    // Construct a DMA specialization that uses the PDC
    template<uint8_t periph_num>
    struct DMA<Uart*, periph_num> : DMA_PDC<Uart*, periph_num> {
        // nothing to do here.
    };
#endif // UART + PDC

#endif // if has PDC






// Now if we have an XDMAC, we use it
#if defined(XDMAC)

    // DMA_XDMAC_hardware template - - MUST be specialized
    template<typename periph_t, uint8_t periph_num>
    struct DMA_XDMAC_hardware {
        DMA_XDMAC_hardware() = delete;
    };


    // NOTE, we have 23 channels, and less than 23 peripheral types using this,
    // so we'll assign channels uniquely, but otherwise arbitrarily from lowest
    // to highest. If using XDMAC directly, beware and use the highest channels
    // first.

    // generic DMA_XDMAC object.
    template<typename periph_t, uint8_t periph_num>
    struct DMA_XDMAC : DMA_XDMAC_hardware<periph_t, periph_num> {
        typedef DMA_XDMAC_hardware<periph_t, periph_num> _hw;
        using _hw::xdmaRxPeripheralId;
        using _hw::xdmaTxPeripheralId;
        using _hw::xdmaRxChannelNumber;
        using _hw::xdmaTxChannelNumber;
        using _hw::xdmaPeripheralRxAddress;
        using _hw::xdmaPeripheralTxAddress;

        typedef typename _hw::buffer_t buffer_t;
        static constexpr uint32_t buffer_width = std::alignment_of< std::remove_pointer<buffer_t> >::value;

        static constexpr Xdmac * const xdma() { return XDMAC; };
        static constexpr XdmacChid * const xdmaRxChannel()
        {
            return xdma()->XDMAC_CHID + xdmaRxChannelNumber();
        };
        static constexpr XdmacChid * const xdmaTxChannel()
        {
            return xdma()->XDMAC_CHID + xdmaTxChannelNumber();
        };

        void reset() const
        {
            // disable the channels
            disableRx();
            disableTx();

            // Configure the Rx and Tx
            // ASSUMPTIONS:
            //  * Rx is from peripheral to memory, and Tx is memory to peripheral
            //  * Not doing memory-to-memory or peripheral-to-peripheral (for now)
            //  * Single Block, Single Microblock transfers (for now)
            //  * All peripherals are using a FIFO for Rx and Tx
            //
            // If ANY of those assumptions are wrong, this code must change!!

            // Configure Rx channel
            xdmaRxChannel()->XDMAC_CSA = (uint32_t)xdmaPeripheralRxAddress();
            xdmaRxChannel()->XDMAC_CC =
                XDMAC_CC_TYPE_PER_TRAN | // between memory and a peripheral
                XDMAC_CC_MBSIZE_SINGLE | // burst size of one "unit" at a time
                XDMAC_CC_DSYNC_PER2MEM | // peripheral->memory
                XDMAC_CC_CSIZE_CHK_1   | // chunk size of one "unit" at a time
                XDMAC_CC_DWIDTH( (buffer_width >> 4) ) | // data width (based on alignment size of base type of buffer_t)
                XDMAC_CC_SIF_AHB_IF1   | // source is peripheral (info cryptically extracted from Table 18-3 of the datasheep)
                XDMAC_CC_DIF_AHB_IF0   | // destination is RAM   (info cryptically extracted from Table 18-3 of the datasheep)
                XDMAC_CC_SAM_FIXED_AM  | // the source address doesn't change (FIFO)
                XDMAC_CC_DAM_INCREMENTED_AM | // destination address increments as read
                XDMAC_CC_PERID(xdmaRxPeripheralId()) // and finally, set the peripheral identifier
            ;
            // Datasheep says to clear these out explicitly:
            xdmaRxChannel()->XDMAC_CNDC = 0; // no "next descriptor"
            xdmaRxChannel()->XDMAC_CBC = 0;  // ???
            xdmaRxChannel()->XDMAC_CDS_MSP = 0; // striding is disabled
            xdmaRxChannel()->XDMAC_CSUS = 0;
            xdmaRxChannel()->XDMAC_CDUS = 0;
            xdmaRxChannel()->XDMAC_CUBC = 0;

            // Configure Tx channel
            xdmaTxChannel()->XDMAC_CDA = (uint32_t)xdmaPeripheralTxAddress();
            xdmaTxChannel()->XDMAC_CC =
                XDMAC_CC_TYPE_PER_TRAN | // between memory and a peripheral
                XDMAC_CC_MBSIZE_SINGLE | // burst size of one "unit" at a time
                XDMAC_CC_DSYNC_MEM2PER | // memory->peripheral
                XDMAC_CC_CSIZE_CHK_1   | // chunk size of one "unit" at a time
                XDMAC_CC_DWIDTH( (buffer_width >> 4) ) | // data width (based on alignment size of base type of buffer_t)
                XDMAC_CC_DIF_AHB_IF1   | // destination is peripheral (info cryptically extracted from Table 18-3 of the datasheep)
                XDMAC_CC_SIF_AHB_IF0   | // source is RAM   (info cryptically extracted from Table 18-3 of the datasheep)
                XDMAC_CC_SAM_INCREMENTED_AM  | // the source address increments as written
                XDMAC_CC_DAM_FIXED_AM | // destination address doesn't change (FIFO)
                XDMAC_CC_PERID(xdmaTxPeripheralId()) // and finally, set the peripheral identifier
            ;
            // Datasheep says to clear these out explicitly:
            xdmaTxChannel()->XDMAC_CNDC = 0; // no "next descriptor"
            xdmaTxChannel()->XDMAC_CBC = 0;  // ???
            xdmaTxChannel()->XDMAC_CDS_MSP = 0; // striding is disabled
            xdmaTxChannel()->XDMAC_CSUS = 0;
            xdmaTxChannel()->XDMAC_CDUS = 0;
            xdmaTxChannel()->XDMAC_CUBC = 0;

        };

        void disableRx() const
        {
            xdma()->XDMAC_GD = XDMAC_GID_ID0 << xdmaRxChannelNumber();
        };
        void enableRx() const
        {
            xdma()->XDMAC_GE = XDMAC_GIE_IE0 << xdmaRxChannelNumber();
        };
        void setRx(void * const buffer, const uint32_t length) const
        {
            xdmaRxChannel()->XDMAC_CDA = (uint32_t)buffer;
            xdmaRxChannel()->XDMAC_CUBC = length;
        };
        void setNextRx(void * const buffer, const uint32_t length) const
        {
//            pdc()->PERIPH_RNPR = (uint32_t)buffer;
//            pdc()->PERIPH_RNCR = length;
        };
        void flushRead() const
        {
            xdmaRxChannel()->XDMAC_CUBC = 0;
        };
        uint32_t leftToRead(bool include_next = false) const
        {
//            if (include_next) {
//            }
            return xdmaRxChannel()->XDMAC_CUBC;
        };
        uint32_t leftToReadNext() const
        {
            return 0;
//            return pdc()->PERIPH_RNCR;
        };
        bool doneReading(bool include_next = false) const
        {
            return leftToRead(include_next) == 0;
        };
        bool doneReadingNext() const
        {
            return leftToReadNext() == 0;
        };
        buffer_t getRXTransferPosition() const
        {
            return (buffer_t)xdmaRxChannel()->XDMAC_CDA;
        };

        // Bundle it all up
        bool startRXTransfer(void * const buffer, const uint32_t length, bool handle_interrupts = true, bool include_next = false) const
        {
            if (doneReading()) {
                if (handle_interrupts) { stopRxDoneInterrupts(); }
                setRx(buffer, length);
                if (length != 0) {
                    if (handle_interrupts) { startRxDoneInterrupts(); }
                    enableRx();
                    return true;
                }
                return false;
            }
            else if (include_next && doneReadingNext()) {
                setNextRx(buffer, length);
                return true;
            }
            return false;
        }


        void disableTx() const
        {
            xdma()->XDMAC_GD = XDMAC_GID_ID0 << xdmaTxChannelNumber();
        };
        void enableTx() const
        {
            xdma()->XDMAC_GE = XDMAC_GIE_IE0 << xdmaTxChannelNumber();
        };
        void setTx(void * const buffer, const uint32_t length) const
        {
            xdmaTxChannel()->XDMAC_CSA = (uint32_t)buffer;
            xdmaTxChannel()->XDMAC_CUBC = length;
        };
        void setNextTx(void * const buffer, const uint32_t length) const
        {
//            pdc()->PERIPH_TNPR = (uint32_t)buffer;
//            pdc()->PERIPH_TNCR = length;
        };
        uint32_t leftToWrite(bool include_next = false) const
        {
//            if (include_next) {
//            }
            return xdmaTxChannel()->XDMAC_CUBC;
        };
        uint32_t leftToWriteNext() const
        {
            return 0;
//            return pdc()->PERIPH_TNCR;
        };
        bool doneWriting(bool include_next = false) const
        {
            return leftToWrite(include_next) == 0;
        };
        bool doneWritingNext() const
        {
            return leftToWriteNext() == 0;
        };
        buffer_t getTXTransferPosition() const
        {
            return (buffer_t)xdmaTxChannel()->XDMAC_CDA;
        };


        // Bundle it all up
        bool startTXTransfer(void * const buffer, const uint32_t length, bool handle_interrupts = true, bool include_next = false) const
        {
            if (doneWriting()) {
                if (handle_interrupts) { stopTxDoneInterrupts(); }
                setTx(buffer, length);
                if (length != 0) {
                    if (handle_interrupts) { startTxDoneInterrupts(); }
                    enableTx();
                    return true;
                }
                return false;
            }
            else if (include_next && doneWritingNext()) {
                setNextTx(buffer, length);
                return true;
            }
            return false;
        }


        void startRxDoneInterrupts() const { /*usart()->US_IER = US_IER_RXBUFF;*/ };
        void stopRxDoneInterrupts() const { /*usart()->US_IDR = US_IDR_RXBUFF;*/ };
        void startTxDoneInterrupts() const { /*usart()->US_IER = US_IER_TXBUFE;*/ };
        void stopTxDoneInterrupts() const { /*usart()->US_IDR = US_IDR_TXBUFE;*/ };

        bool inRxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            //            if (uart()->UART_IMR & UART_IMR_RXBUFF) {
            //                // then we read the status register
            //                return (uart()->UART_SR & UART_SR_RXBUFF);
            //            }
            return false;
        }

        bool inTxBufferEmptyInterrupt() const
        {
            // we check if the interupt is enabled
            //            if (uart()->UART_IMR & UART_IMR_TXBUFE) {
            //                // then we read the status register
            //                return (uart()->UART_SR & UART_SR_TXBUFE);
            //            }
            return false;
        }
    };

    // NOTE: If we have an XDMAC peripheral, we don't have PDC, and it's the
    // only want to DMC all of these peripherals.
    // So we don't need the XDMAC deduction -- it's already done.

    // We're deducing if there's a USART and it has a PDC
    // Notice that this relies on defines set up in SamCommon.h
#ifdef HAS_USART0

#pragma mark DMA_XDMAC Usart implementation

    template<uint8_t uartPeripheralNumber>
    struct DMA_XDMAC_hardware<Usart*, uartPeripheralNumber> {
        // this is identical to in SamUART
        static constexpr Usart * const usart()
        {
            switch (uartPeripheralNumber) {
                case (0): return USART0;
                case (1): return USART1;
                case (2): return USART2;
            };
        };

        static constexpr uint8_t const xdmaTxPeripheralId()
        {
            switch (uartPeripheralNumber) {
                case (0): return  7;
                case (1): return  9;
                case (2): return 11;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaTxChannelNumber()
        {
            switch (uartPeripheralNumber) {
                case (0): return  0;
                case (1): return  2;
                case (2): return  4;
            };
            return 0;
        };
        static constexpr void * const xdmaPeripheralTxAddress()
        {
            return &usart()->US_THR;
        };
        static constexpr uint8_t const xdmaRxPeripheralId()
        {
            switch (uartPeripheralNumber) {
                case (0): return  8;
                case (1): return 10;
                case (2): return 12;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaRxChannelNumber()
        {
            switch (uartPeripheralNumber) {
                case (0): return  1;
                case (1): return  3;
                case (2): return  5;
            };
            return 0;
        };
        static constexpr void * const xdmaPeripheralRxAddress()
        {
            return &usart()->US_RHR;
        };

        typedef char* buffer_t;
    };

    // Construct a DMA specialization that uses the PDC
    template<uint8_t periph_num>
    struct DMA<Usart*, periph_num> : DMA_XDMAC<Usart*, periph_num> {
        // nothing to do here.
    };
#endif // USART + XDMAC

    // We're deducing if there's a UART and it has a PDC
    // Notice that this relies on defines set up in SamCommon.h
#ifdef HAS_UART0

#pragma mark DMA_XDMAC Usart implementation

    template<uint8_t uartPeripheralNumber>
    struct DMA_XDMAC_hardware<Uart*, uartPeripheralNumber>
    {
        static constexpr Uart * const uart() {
            switch (uartPeripheralNumber) {
                case (0): return UART0;
                case (1): return UART1;
                case (2): return UART2;
                case (3): return UART3;
                case (4): return UART4;
            };
        };

        static constexpr uint8_t const xdmaTxPeripheralId()
        {
            switch (uartPeripheralNumber) {
                case (0): return 20;
                case (1): return 22;
                case (2): return 24;
                case (3): return 26;
                case (4): return 28;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaTxChannelNumber()
        {
            switch (uartPeripheralNumber) {
                case (0): return  6;
                case (1): return  8;
                case (2): return 10;
                case (3): return 12;
                case (4): return 14;
            };
            return 0;
        };
        static constexpr volatile void * const xdmaPeripheralTxAddress()
        {
            return &uart()->UART_THR;
        };
        static constexpr uint8_t const xdmaRxPeripheralId()
        {
            switch (uartPeripheralNumber) {
                case (0): return 21;
                case (1): return 23;
                case (2): return 25;
                case (3): return 27;
                case (4): return 29;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaRxChannelNumber()
        {
            switch (uartPeripheralNumber) {
                case (0): return  7;
                case (1): return  9;
                case (2): return 11;
                case (3): return 13;
                case (4): return 15;
            };
            return 0;
        };
        static constexpr volatile void * const xdmaPeripheralRxAddress()
        {
            return &uart()->UART_RHR;
        };

        typedef char* buffer_t ;
    };

    // Construct a DMA specialization that uses the PDC
    template<uint8_t periph_num>
    struct DMA<Uart*, periph_num> : DMA_XDMAC<Uart*, periph_num> {
        // nothing to do here.
    };
#endif // UART + XDMAC

#endif // does not have XDMAC

} // namespace Motate

#endif /* end of include guard: SAMDMA_H_ONCE */