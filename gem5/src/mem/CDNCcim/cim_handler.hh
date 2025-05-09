/**
 * CDNCcim:
 *
 * @file
 * CimHandler declaration
 *      base class for CIM operation handler
 */

#ifdef CDNCcimFlag

#ifndef __CIM_HANDLER_HH__
#define __CIM_HANDLER_HH__

#include "debug/CIMDBG.hh"
#include "mem/abstract_mem.hh"
#include "mem/mem_ctrl.hh"
#include "params/CimHandler.hh"
#include "sim/sim_object.hh"

namespace gem5
{
namespace memory
{

class CimHandler : public SimObject
{
  private:
    enum class OperationType : uint8_t
    {
        AND = 0,
        OR,
        XOR,
        COPY,
        NOT_COND,
        //
        short_AND = 0x80,
        short_OR,
        short_XOR,
        short_COPY,
        short_NOT_COND,
    };
    struct CommandDecode
    {
        uint8_t operation_type { 0xff };
        uint8_t operation_flag_mask { 0 };
        //
        uint8_t byte_mask { 0xffu };
        uint32_t bank_mask { 0xfffffffu };
        uint64_t column_mask { 0xfffffffffffffffful };
        //
        uint16_t row_number[8] { 0xffffu, 0xffffu, 0xffffu, 0xffffu,
                                 0xffffu, 0xffffu, 0xffffu, 0xffffu };
        //
        uint16_t dest { 0 };
        //
        void print();
    };

    //
    const uint64_t CommandSize { 3 * 8 };
    const uint8_t numOperationTypes { 5 };
    const uint8_t byteBits { 3 };
    //
    Addr readWriteAddress;
    Addr resultTemporaryBufferAddress;
    Addr commandWriteAddress;
    //
    uint8_t numColumnBits;
    uint8_t numBankBits;
    uint8_t numRowBits;
    //
    std::vector<Tick> operationsInitLatency;
    std::vector<Tick> operationsOnWordLatency;
    //
    Tick *unitReleaseTime;
    //
    void cimExecuteCommand(
        AbstractMemory *abstract_mem, CommandDecode &command);
    void cimUpdateLatencyTable(bool init, uint8_t operation, size_t bank);

    uint8_t *addressTranslator(
        AbstractMemory *abstract_mem, Addr startAddress, uint16_t row,
        uint8_t bank, uint8_t column);

  public:
    CimOperationInterface *cimOperationHandler;
    inline bool isCimAddressRenge(const Addr &addr) const
    {
        return (addr >= readWriteAddress)
               && (addr < (commandWriteAddress + CommandSize));
    }

    inline bool isCimReadWriteRegion(const Addr &addr) const
    {
        return (addr >= readWriteAddress)
               && (addr
                   < (readWriteAddress
                      + (1ul << (numRowBits + numBankBits + numColumnBits))));
    }
    inline bool isCimBufferRegion(const Addr &addr) const
    {
        return (addr >= resultTemporaryBufferAddress)
               && (addr
                   < (resultTemporaryBufferAddress
                      + (1ul << (numRowBits + numBankBits + numColumnBits))));
    }
    inline bool isCimCommandRegion(const Addr &addr) const
    {
        return (addr >= commandWriteAddress)
               && (addr < (commandWriteAddress + CommandSize));
    }

    CimHandler(const CimHandlerParams &_p);
    ~CimHandler();

    void cimFetchCommand(
        AbstractMemory *abstract_mem, PacketPtr pkt, uint8_t *host_addr);

    Tick getCimLatency(const Addr &addr);
};
} // namespace memory
} // namespace gem5

#endif //__CIM_HANDLER_HH__
#endif // CDNCcimFlag