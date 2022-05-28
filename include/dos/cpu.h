#ifndef CPU_H
#define CPU_H

#include <string>
#include "dos/types.h"
#include "dos/registers.h"
#include "dos/memory.h"

class Cpu {
public:
    virtual std::string type() const = 0;
    virtual std::string info() const = 0;
    virtual void init(const Address &code, const Address &stack) = 0;
    virtual void step() = 0;
    virtual void run() = 0;
};

class InterruptInterface;

class Cpu_8086 : public Cpu {
    friend class Cpu_8086_Test;
    
private:
    Memory *mem_;
    InterruptInterface *int_;
    Registers regs_;
    const Byte *memBase_, *code_;
    Byte opcode_, modrm_;
    Byte byteOperand1_, byteOperand2_, byteResult_;
    Word wordOperand1_, wordOperand2_, wordResult_;
    bool done_, step_;

public:
    Cpu_8086(Memory *memory, InterruptInterface *inthandler);
    std::string type() const override { return "8086"; };
    std::string info() const override;
    void init(const Address &codeAddr, const Address &stackAddr) override;
    void step() override;
    void run() override;

private:
    // utility
    Register defaultSeg(const Register) const;

    // instruction pointer access and manipulation
    inline Byte ipByte(const Word offset = 0) const { return code_[regs_.bit16(REG_IP) + offset]; }
    inline Word ipWord(const Word offset = 0) const { return *WORD_PTR(code_, regs_.bit16(REG_IP) + offset); }
    inline void ipAdvance(const Word amount) { regs_.bit16(REG_IP) += amount; }

    inline Byte memByte(const Offset offset) const { return memBase_[offset]; }
    inline Word memWord(const Offset offset) const { return *WORD_PTR(memBase_, offset); }

    // ModR/M byte and evaluation
    Register modrmRegister(const RegType type, const Byte value) const;
    inline Register modrmRegRegister(const RegType regType) const;
    inline Register modrmMemRegister(const RegType regType) const;
    Offset modrmMemAddress() const;
    Word modrmInstructionLength() const;

    // instruction execution pipeline
    void pipeline();
    void dispatch();
    void unknown();

    // instructions 
    void instr_mov();
};

#endif // CPU_H