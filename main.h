#pragma once

//This allows black box modules to easily specify a pipeline latency by intializing their queues
template <typename T> 
constexpr std::deque<T>  func(int n)
{
    std::deque<T> ret;
    for(int i=0; i < n; i++)
	ret.push_back(T());
    return ret;
}

#define PIPELINE(T,x) \
	static std::deque<__typeof__(T)> deq_##T = func<__typeof__(T)>(x); \
	deq_##T.push_back(T); \
	T = deq_##T.front(); \
	deq_##T.pop_front();

enum opcode_type {opcode_load, opcode_op_imm, opcode_auipc, opcode_store, opcode_op, opcode_lui, opcode_branch, opcode_jalr, opcode_jal, opcode_sys, opcode_invalid};
enum instr_type_type {i_type,s_type,sb_type,u_type,uj_type,r_type,no_type};

#define downto(x,u,d) (   (((x) << (31-(u))) >> (31 - (u)))>>(d) )
#define sign_ex(x,b)  ( ((int32_t) ((x)<<(32-(b))))>>(32-(b))  )

struct icin_type {
	icin_type(){valid=0;}
	uint32_t pc,valid;
};

struct icout_type {
	icout_type(){valid=0;}
	uint32_t instr[4],pc[4],valid;
};

struct decout_type {
	decout_type(){valid=0;}
	uint32_t valid,pc,rd,rs1,rs2,imm;
	uint32_t invalid_op,uses_rd,uses_rs1,uses_rs2;
	uint32_t isjump,jump_reg;
	uint32_t target,predict;
};

struct ratout_type {
	ratout_type(){decout.valid=0;}
	decout_type decout;
	uint32_t rdold;
};


void decode_tick(icout_type in, decout_type (&out)[4]);
