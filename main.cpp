#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <queue>

#include "main.h"

/*************************************************************
	Cycle "accurate" simulator for the Oxygen processor
	CPU does not actually exist yet so how accurate can it be. 
	This is to explore implementation realities. Some
	of the black box modules have been implemented so there
	hardware resources and latencies have been defined. 
*************************************************************/

//For now we are simulating zero i cache misses so black box here is trivial
static uint32_t imem[8192];
void icache_tick(icin_type in, icout_type &out){
	PIPELINE(in,3);

	out.valid=0;
	if(in.valid)
		out.valid = 0xF & (0xF << ((in.pc / 4) % 4));

	in.pc = in.pc & 0x7FFF;
	for(int i=0; i < 4; i++){
		out.pc[i] = (((in.pc >> 2) & ~0x3)+i)<<2;
		out.instr[i] = imem[out.pc[i]>>2];
	}
}

void jump_tick(decout_type decin0, decout_type decin1, decout_type decin2, decout_type decin3, icin_type out){
	PIPELINE(decin0,2);
	PIPELINE(decin1,2);
	PIPELINE(decin2,2);
	PIPELINE(decin3,2);

	decout_type decin[4] = {decin0,decin1,decin2,decin3};

	for(int i=0; i < 4; i++){
        	if(decin[i].valid && (decin[i].jump_reg || (decin[i].jump_reg == 0 && decin[i].isjump && decin[i].predict))){
			//Jumping to somewhere
			if(decin[i].jump_reg)
				out.pc = decin[i].target;
			else
				out.pc = decin[i].pc + decin[i].imm;
			return;
		}
	}
	out.pc = (out.pc & ~0xF) + 0x10;
}

template <typename T> 
constexpr std::deque<T>  init_fifo() {
    std::deque<T> ret;
    for(uint32_t i=32; i < 512; i++)
	ret.push_back(i);
    return ret;
}

static uint32_t rat[32] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
void rat_tick(decout_type decin0, decout_type decin1, decout_type decin2, decout_type decin3, ratout_type (&out)[4]){
	PIPELINE(decin0,2);
	PIPELINE(decin1,2);
	PIPELINE(decin2,2);
	PIPELINE(decin3,2);

	static std::deque<uint32_t> rat_fifo = init_fifo<uint32_t>();
	
	decout_type decin[4] = {decin0,decin1,decin2,decin3};

	//Just copy the whole thing to get control signals
	for(int i=0; i < 4; i++)
		out[i].decout = decin[i];

	for(int i=0; i < 4; i++){
		if(!decin[i].valid)
			continue;

		out[i].decout.rs1 = rat[decin[i].rs1];
		out[i].decout.rs2 = rat[decin[i].rs2];
		out[i].rdold = rat[decin[i].rd];
	
		if(!decin[i].uses_rd)	
			continue;

		out[i].decout.rd = rat_fifo.front();
		rat_fifo.pop_front();
		rat[decin[i].rd] = out[i].decout.rd;
	}
}


int main(){
	icin_type icin;
	icout_type icout;
	decout_type decout[4];
	ratout_type ratout[4];
	
	uint32_t tick=0;
    	while(1){
		//These 3 are the main fetch pipeline. Everything else is a slave to the instructions fetched here
		icache_tick(icin,icout);
		decode_tick(icout,decout);
		jump_tick(decout[0],decout[1],decout[2],decout[3],icin);

		rat_tick(decout[0],decout[1],decout[2],decout[3],ratout);	
 
		printf("%d\n",tick++);
   	}
    return 0;
}
