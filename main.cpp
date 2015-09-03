#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <queue>

#include "main.h"

/*************************************************************
	Cycle "accurate" simulator for the Oxygen processor
	CPU does not actually exist yet so how accurate can it be. 
	This is to explore implementation realities. Some
	of the black box modules have been implemented so their
	hardware resources and latencies have been defined. 
*************************************************************/

//For now we are simulating zero i cache misses so black box here is trivial
static uint32_t imem[8192];
void icache_tick(icin_type in, icout_type &out){
	PIPELINE(in,3);

	out.valid=0;
	if(in.valid){
		out.valid = 0xF & (0xF << ((in.pc / 4) % 4));
//		printf("out.valid: %X\n", out.valid);
	}
	in.pc = in.pc & 0x7FFF;
	for(int i=0; i < 4; i++){
		out.pc[i] = (((in.pc >> 2) & ~0x3)+i)<<2;
		out.instr[i] = imem[out.pc[i]>>2];
	}
}

enum jump_states { jump_nospec,jump_rightspec,jump_wrongspec };
void jump_tick(decout_type decin0, decout_type decin1, decout_type decin2, decout_type decin3, icin_type &out){
	PIPELINE(decin0,2);
	PIPELINE(decin1,2);
	PIPELINE(decin2,2);
	PIPELINE(decin3,2);

	static uint32_t jump_state = jump_nospec;
	static uint32_t spec_done = ~0;

	decout_type decin[4] = {decin0,decin1,decin2,decin3};
	if(out.valid)
		out.pc = (out.pc & ~0xF) + 0x10;

	out.valid = 1;

	//printf("in.valid: %d\n", decin0.valid);

	for(int i=0; i < 4; i++){
		if(decin[i].valid && decin[i].pc == spec_done){
			spec_done = ~0;
			jump_state = jump_rightspec;
		}
        	if(decin[i].valid && (decin[i].jump_reg || (decin[i].jump_reg == 0 && decin[i].isjump && decin[i].predict)) && jump_state != jump_wrongspec){
			//Jumping to somewhere
			if(decin[i].jump_reg){
				out.pc = decin[i].target;
				printf("RJump to: %X\n", out.pc);
			}else{
				out.pc = decin[i].pc + decin[i].imm;
				printf("Jump to: %X %X %X\n", out.pc,decin[i].pc,decin[i].imm);
			}
			//TODO: more advanced stuff here. it's not even worth the fetch flush if branch target is sufficiently close
			if(decin[i].pc == out.pc){
				jump_state = jump_rightspec;
				continue;
			}
			jump_state = jump_wrongspec;
			spec_done = out.pc;
		}
	}
}

template <typename T> 
constexpr std::deque<T>  init_fifo() {
    std::deque<T> ret;
    for(uint32_t i=32; i < 512; i++)
	ret.push_back(i);
    return ret;
}

//TODO: this initialization is a bit weird. it starts with 32 registers allocated but since startup has all contents
//undefined it is unclear if this is needed. Could map everything to 0. but must make sure 0 reg is never freed. 
static uint32_t rat[32] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
void rat_tick(decout_type decin0, decout_type decin1, decout_type decin2, decout_type decin3, ratout_type (&out)[4]){
	PIPELINE(decin0,2);
	PIPELINE(decin1,2);
	PIPELINE(decin2,2);
	PIPELINE(decin3,2);

	static std::deque<uint32_t> rat_fifo = init_fifo<uint32_t>();
	
	decout_type decin[4] = {decin0,decin1,decin2,decin3};

	//TODO: for now nobody is returning anything to rat so we will have to quit
	if(rat_fifo.size() < 4)
		exit(0);

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

void reserver_tick(ratout_type ratout0, ratout_type ratout1, ratout_type ratout2, ratout_type ratout3){
	PIPELINE(ratout0,2);
	PIPELINE(ratout1,2);
	PIPELINE(ratout2,2);
	PIPELINE(ratout3,2);

	ratout_type ratout[4] = {ratout0,ratout1,ratout2,ratout3};
	bool used_jump=0;
	bool stall=0;

	for(int i=0; i < 4; i++){
		if(!ratout[i].decout.valid)
			continue;
		//Try to find an execution unit for every instruction
		if(ratout[4].decout.isjump){
			if(used_jump){
				stall=1;
				continue;
			}
			//Must go to EX3
			used_jump = 1;
			continue;
		}

		//if ld/store, target ex4 ex5
		
		//Must be ALU
				
	}
}


int main(){
	icin_type icin;
	icout_type icout;
	decout_type decout[4];
	ratout_type ratout[4];
	reservation_station stations[5];
	
	//Load the data file
	FILE* f = fopen("test.bin","r");
	fseek(f, 0L, SEEK_END);
	int32_t sz = ftell(f);
	fseek(f, 0L, SEEK_SET);
	fread(imem,std::min(sz,32*1024),1,f);
	fclose(f);	

	uint32_t tick=0;
    	while(1){
		//These 3 are the main fetch pipeline. Everything else is a slave to the instructions fetched here
		icache_tick(icin,icout);
		decode_tick(icout,decout);
		jump_tick(decout[0],decout[1],decout[2],decout[3],icin);

		rat_tick(decout[0],decout[1],decout[2],decout[3],ratout);	
		reserver_tick(ratout[0],ratout[1],ratout[2],ratout[3]);
 
//		printf("%d\n",tick++);
   	}
    return 0;
}
