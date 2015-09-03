#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <queue>

#include "main.h"

void decode(uint32_t instr, uint32_t pc, uint32_t valid, decout_type &decout){
	uint32_t rd = (instr >> 7) & 0x1F;
        uint32_t rs1 = (instr >> 15) & 0x1F;
        uint32_t rs2 = (instr >> 20) & 0x1F;
	uint32_t opcode, instr_type, imm,s_foo;
	int32_t sinstr = instr;
        
        switch( (instr >> 2) & 0x1F) {
            case 0b00000: opcode = opcode_load; break;
            case 0b00100: opcode = opcode_op_imm; break;
            case 0b00101: opcode = opcode_auipc; break;
            case 0b01000: opcode = opcode_store; break;
            case 0b01100: opcode = opcode_op; break;
            case 0b01101: opcode = opcode_lui; break;
            case 0b11000: opcode = opcode_branch; break;
            case 0b11001: opcode = opcode_jalr; break;
            case 0b11011: opcode = opcode_jal; break;
            case 0b11100: opcode = opcode_sys; break;
            default: opcode = opcode_invalid;
        }
        switch(opcode){ 
            case opcode_load: instr_type = i_type; break;
            case opcode_op_imm: instr_type = i_type; break;
            case opcode_auipc: instr_type = u_type; break;
            case opcode_store: instr_type = s_type; break;
            case opcode_op: instr_type = r_type; break;
            case opcode_lui: instr_type = u_type; break;
            case opcode_branch: instr_type = sb_type; break;
            case opcode_jalr: instr_type = i_type; break;
            case opcode_jal: instr_type = uj_type; break;
            case opcode_sys: instr_type = no_type; break;
            default: instr_type = no_type;
        }

        if(instr & 0x3 != 0b11)
            instr_type = no_type;
        
        imm = 0;
        s_foo = ((sinstr>>20) & ~0x1F) | ((instr>>7)&0x1F);
        switch(instr_type){
            case i_type: imm = sinstr >> 20; break;
            case s_type: imm = s_foo; break;
            case sb_type: imm = sign_ex((downto(instr,31,31) << 12)  | (downto(instr,7,7) << 11) | (downto(instr,30,25) << 5) | (downto(instr,11,8) << 1),13);
            case u_type: imm = instr & ~((1<<12) - 1); break;
            case uj_type: imm = sign_ex((downto(instr,31,31) << 20) | (downto(instr,19,12) << 12) | (downto(instr,20,20) << 11) | (downto(instr,30,21) << 1),21);
            default: imm = 0; 
        }
        
        //Signal that branch predictor is going to have to make a choice
            decout.isjump = opcode == opcode_jalr || opcode == opcode_jal || opcode == opcode_branch || opcode == opcode_sys;
            decout.jump_reg = opcode == opcode_jalr;
        
        //Signals that renamer must allocate/retire/locate these registers
           decout.uses_rd = rd != 0 && instr_type != s_type && instr_type != sb_type && instr_type != no_type;
           decout.uses_rs1 = rs1 != 0 && instr_type != u_type && instr_type != uj_type && instr_type != no_type;
           decout.uses_rs2 = rs2 != 0 && instr_type != u_type && instr_type != uj_type && instr_type != i_type && instr_type != no_type;
        
           decout.invalid_op = opcode == opcode_invalid && valid == 1;
        
           decout.pc <= pc;
           decout.rd <= rd;
           decout.rs1 <= rs1;
           decout.rs2 <= rs2;
           decout.imm <= imm;
           decout.valid <= opcode != opcode_invalid && valid == 1;
}

static uint32_t target_mem[4][16];
void jump_target(uint32_t pc, decout_type &decout, int i){
	decout.target = target_mem[i][(pc >> 4) & 0xF];
}

static uint8_t predict[8192];
void jump_predict(uint32_t pc, decout_type &decout){
	decout.predict = predict[(pc >> 2) &  0x1FFF];
}

void decode_tick(icout_type in, decout_type (&out)[4]){
	PIPELINE(in,1);

	for(int i=0; i < 4; i++){
		decode(in.instr[i],in.pc[i],(in.valid>>i)&1,out[i]);
		jump_target(in.pc[i],out[i],i);

	}
}

