//Ernesto Soltero
//File: CPU.cpp
//Description: Contains the main code to run instructions given by an
// object file

#include "include.h"

//Data bits 
const unsigned int DATA(16);
//Immediate Values
const unsigned int IMM(8);

//Buses
Bus abus("ADDRBUS", DATA);
Bus dbus("DATABUS", DATA);

//Registers
StorageObject ir("IR", DATA);
//S0 is special in that should only hold immediate values so it truncates 
//to only 8 bits
Counter s0("s0", IMM);
Counter pc("PC", DATA);
Counter npc("NPC", DATA);
Counter s1("S1", DATA);
Counter s2("S2", DATA);
Clearable s3("S3", DATA);

//Memory 
Memory m("MEM", DATA, DATA);

//ALU
BusALU mainAlu("ADDER", DATA);
BusALU preAlu("PRE", DATA);

//control variables


/*
* Creates the connections between components as specified by the hardware
* document.
*/
void setup(){
	//Memory connections
	m.MAR().connectsTo(abus.OUT());
	
	//s1 register	
	s1.connectsTo(dbus.IN());
	s1.connectsTo(dbus.OUT());
	s1.connectsTo(mainAlu.OP1());
	s1.connectsTo(mainAlu.OP2());
	
	//s2 register
	s2.connectsTo(dbus.IN());
	s2.connectsTo(dbus.OUT());
	s2.connectsTo(mainAlu.OP1());
	s2.connectsTo(mainAlu.OP2());

	//s3 register
	s3.connectsTo(abus.OUT());
	s3.connectsTo(abus.IN());
	s3.connectsTo(dbus.OUT());
	s3.connectsTo(dbus.IN());
	s3.connectsTo(m.READ());
	s3.connectsTo(m.WRITE());
	s3.connectsTo(mainAlu.OUT());
	
	//Program Counter
	pc.connectsTo(abus.OUT());
	pc.connectsTo(abus.IN());
	pc.connectsTo(mainAlu.OP2());
	pc.connectsTo(preAlu.OP2());

	//New Program Counter
	npc.connectsTo(abus.OUT());
	npc.connectsTo(abus.IN());
	npc.connectsTo(preAlu.OP1());
	
	//s0 register
	s0.connectsTo(abus.OUT());
	s0.connectsTo(abus.IN());
	s0.connectsTo(dbus.IN());
	s0.connectsTo(dbus.OUT());
	s0.connectsTo(m.READ());
	s0.connectsTo(mainAlu.OP1());
	s0.connectsTo(mainAlu.OP2());
	
	//Instruction register
	ir.connectsTo(m.READ());
	ir.connectsTo(dbus.IN());
	ir.connectsTo(dbus.OUT());
}

//Fetch the next instruction and place into instruction register
void fetch() {
	abus.IN().pullFrom(pc);
	m.MAR().latchFrom(abus.OUT());
	Clock::tick();
	ir.latchFrom(m.READ());
	m.read();
	Clock::tick();
}


//adds two values depending on the memory mode
void add_sub_instruction(bool add=false) {
	//figure out the address mode
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	if(result == 0){
		//check to see what register to use
		switch(ir(DATA-5, DATA-7)){
			//data in s1 or s2
			case 0:
			case 1:
				//if source is reg 0 the S1 - S2
				//else S2 - S1
				if(source == 0){
					mainAlu.OP1().pullFrom(s1);
					mainAlu.OP2().pullFrom(s2);
				} else {
					mainAlu.OP1().pullFrom(s2);
					mainAlu.OP2().pullFrom(s1);
				}
				if(add){
					mainAlu.perform(BusALU::op_add);
				} else {
					mainAlu.perform(BusALU::op_sub);
				}
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				//figure out what the source register is and put back result
				dbus.IN().pullFrom(s3);
				if(source == 0){
					//reg s1
					s1.latchFrom(dbus.OUT());
				} else {
					//reg s2
					s2.latchFrom(dbus.OUT());
				}
				break;
			//data in immediate
			case 4:
				//Source Register - IMM
				mainAlu.OP2().pullFrom(s0);
				s3.latchFrom(mainAlu.OUT());
				if(add){
					mainAlu.perform(BusALU::op_add);
				} else {
					mainAlu.perform(BusALU::op_sub);
				}
				if(source == 0){ 
					mainAlu.OP1().pullFrom(s1);
				} else {
					mainAlu.OP1().pullFrom(s2);
				}
				Clock::tick();
				//now put back in source
				dbus.IN().pullFrom(s3);
				if(source == 0){
					//reg s1
					s1.latchFrom(dbus.OUT());
				} else {
					//reg s2
					s2.latchFrom(dbus.OUT());
				}
				break;
		}
	} else if(result == 1) {
		//EA is in s3 so need to fetch
		abus.IN().pullFrom(s3);
		m.MAR().latchFrom(abus.OUT());
		Clock::tick();
		m.read();
		s0.latchFrom(m.READ());
		Clock::tick();
		mainAlu.OP2().pullFrom(s0);
		s3.latchFrom(mainAlu.OUT());
		if(add) {
			mainAlu.perform(BusALU::op_add);
		} else {
			mainAlu.perform(BusALU::op_sub);
		}
		//figure out where to pull data
		if(source == 0) {
			mainAlu.OP1().pullFrom(s1);
		} else{
			mainAlu.OP1().pullFrom(s2);
		}
		Clock::tick();
		//now put data back in source register
		dbus.IN().pullFrom(s3);
		if(source == 0){
			s1.latchFrom(dbus.OUT());
		} else {
			s2.latchFrom(dbus.OUT());
		}
	}
	Clock::tick();
}

/*
*
*/
void store(){
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	abus.IN().pullFrom(s3);
	m.MAR().latchFrom(abus.OUT());
	Clock::tick();
	if(source == 0){
		mainAlu.OP1().pullFrom(s1);
	} else {
		mainAlu.OP1().pullFrom(s2);
	}
	mainAlu.perform(BusALU::op_rop1);
	s3.latchFrom(mainAlu.OUT());
	Clock::tick();
	m.WRITE().pullFrom(s3);
	m.write();			
	Clock::tick();
}

/*
*
*/
void load(){
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	abus.IN().pullFrom(s3);
	m.MAR().latchFrom(abus.OUT());
	Clock::tick();
	m.read();
	s3.latchFrom(m.READ());
	Clock::tick();
	dbus.IN().pullFrom(s3);
	if(source == 0){
		s1.latchFrom(dbus.OUT());
	} else {
		s2.latchFrom(dbus.OUT());
	}
	Clock::tick();
}

void and_or_instruction(bool andInst = true){
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	if(result == 0){
		switch(ir(DATA-5, DATA-7)){
			case 0:
			case 1:
				mainAlu.OP1().pullFrom(s1);
				mainAlu.OP2().pullFrom(s2);
				if(andInst){
					mainAlu.perform(BusALU::op_and);
				} else {
					mainAlu.perform(BusALU::op_or);
				}
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				//figure out what the source register is
				dbus.IN().pullFrom(s3);
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
			case 4:
				s0.latchFrom(dbus.OUT());
				dbus.IN().pullFrom(ir);
				Clock::tick();
				mainAlu.OP1().pullFrom(s0);
				s3.latchFrom(mainAlu.OUT());
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
		}
	} else {
		//EA is in s3 so need to fetch
		abus.IN().pullFrom(s3);
		m.MAR().latchFrom(abus.OUT());
		Clock::tick();
		m.read();
		s0.latchFrom(m.READ());
		Clock::tick();
		mainAlu.OP2().pullFrom(s0);
		s3.latchFrom(mainAlu.OUT());
		if(andInst) {
			mainAlu.perform(BusALU::op_and);
		} else {
			mainAlu.perform(BusALU::op_or);
		}
		//figure out where to pull data
		if(source == 0) {
			mainAlu.OP1().pullFrom(s1);
		} else{
			mainAlu.OP1().pullFrom(s2);
		}
		Clock::tick();
		//now put data back in source register
		dbus.IN().pullFrom(s3);
		if(source == 0){
			s1.latchFrom(dbus.OUT());
		} else {
			s2.latchFrom(dbus.OUT());
		}
	}
	Clock::tick();
}

void shift_instruction(bool left = true){
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	if(result == 0){
		switch(ir(DATA-5, DATA-7)){
			case 0:
			case 1:
				if(source = 0){
					mainAlu.OP1().pullFrom(s1);
					mainAlu.OP2().pullFrom(s2);
				} else {
					mainAlu.OP1().pullFrom(s2);
					mainAlu.OP2().pullFrom(s1);	
				}
				if(left){
					mainAlu.perform(BusALU::op_lshift);
				} else {
					mainAlu.perform(BusALU::op_rshift);
				}
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				dbus.IN().pullFrom(s3);
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
			case 4:
				dbus.IN().pullFrom(ir);
				s0.latchFrom(dbus.OUT());
				Clock::tick();
				mainAlu.OP2().pullFrom(s0);
				if(source == 0){
					mainAlu.OP1().pullFrom(s1);
				} else {
					mainAlu.OP1().pullFrom(s2);
				}
				if(left){
					mainAlu.perform(BusALU::op_lshift);
				} else {
					mainAlu.perform(BusALU::op_lshift);
				}
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				dbus.IN().pullFrom(s3);
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
		}
	} else {
			//EA is in s3 so need to fetch
			abus.IN().pullFrom(s3);
			m.MAR().latchFrom(abus.OUT());
			Clock::tick();
			m.read();
			s0.latchFrom(m.READ());
			Clock::tick();
			mainAlu.OP2().pullFrom(s0);
			s3.latchFrom(mainAlu.OUT());
			if(left) {
				mainAlu.perform(BusALU::op_lshift);
			} else {
				mainAlu.perform(BusALU::op_rshift);
			}
			//figure out where to pull data
			if(source == 0) {
				mainAlu.OP1().pullFrom(s1);
			} else{
				mainAlu.OP1().pullFrom(s2);
			}
			Clock::tick();
			//now put data back in source register
			dbus.IN().pullFrom(s3);
			if(source == 0){
				s1.latchFrom(dbus.OUT());
			} else {
				s2.latchFrom(dbus.OUT());
			}
	}
	Clock::tick();
}

void complement_instruction(){
	long result = compute_address();
	long source = ir(DATA-8, DATA-8);
	if(result == 0){
		switch(ir(DATA-5, DATA-7)){
			case 0:
			case 1:
				if(source = 0){
					mainAlu.OP1().pullFrom(s1);
				} else {
					mainAlu.OP1().pullFrom(s2);
				}
				mainAlu.perform(BusALU::op_not);
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				dbus.IN().pullFrom(s3);
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
			case 4:
				dbus.IN().pullFrom(ir);
				s0.latchFrom(dbus.OUT());
				Clock::tick();
				mainAlu.OP1().pullFrom(s0);
				mainAlu.perform(BusALU::op_not);
				s3.latchFrom(mainAlu.OUT());
				Clock::tick();
				dbus.IN().pullFrom(s3);
				if(source == 0){
					s1.latchFrom(dbus.OUT());
				} else {
					s2.latchFrom(dbus.OUT());
				}
				break;
		}
	} else {
		//EA is in s3 so need to fetch
		abus.IN().pullFrom(s3);
		m.MAR().latchFrom(abus.OUT());
		Clock::tick();
		m.read();
		s0.latchFrom(m.READ());
		Clock::tick();
		mainAlu.OP1().pullFrom(s0);
		s3.latchFrom(mainAlu.OUT());
		mainAlu.perform(BusALU::op_not);
		Clock::tick();
		//now put data back in source register
		dbus.IN().pullFrom(s3);
		if(source == 0){
			s1.latchFrom(dbus.OUT());
		} else {
			s2.latchFrom(dbus.OUT());
		}
	}	
	Clock::tick();	
}
	

/*
*
*/
long compute_address(){
	long addr = ir(DATA-5, DATA-7);
	switch(addr){
		//data is in s1
		//data is in s2
		case 0:
		case 1:
			//can handle this individually
			return 0;
			break;
		//displacement with s1 + imm
		case 2:
			//imm is last 10 bits throw into s0
			dbus.IN().pullFrom(ir);
			s0.latchFrom(dbus.OUT());
			Clock::tick();
			//add to get effective address and move to s3
			mainAlu.OP2().pullFrom(s1);
			mainAlu.OP1().pullFrom(s0);
			s3.latchFrom(mainAlu.OUT());
			mainAlu.perform(BusALU::op_add);
			Clock::tick();
			return 1;
			break;
		//displacement with s2 + imm
		case 3:
			dbus.IN().pullFrom(ir);
			s0.latchFrom(dbus.OUT());
			Clock::tick();
			mainAlu.OP1().pullFrom(s0);
			mainAlu.OP2().pullFrom(s2);
			s3.latchFrom(mainAlu.OUT());
			mainAlu.perform(BusALU::op_add);
			Clock::tick();
			return 1;
			break;
		//data is in immediate 
		case 4:
			//move to s0
			dbus.IN().pullFrom(ir);
			s0.latchFrom(dbus.OUT());
			Clock::tick();
			return 0;
			break;
		//EA = imm so move to s3
		case 5:
			dbus.IN().pullFrom(ir);
			s3.latchFrom(dbus.OUT());
			Clock::tick();
			return 1;
			break;
		//EA = PC + imm 
		case 6:
			dbus.IN().pullFrom(ir);
			s0.latchFrom(dbus.OUT());
			Clock::tick();
			mainAlu.OP1().pullFrom(s0);
			mainAlu.OP2().pullFrom(pc);
			mainAlu.perform(BusALU::op_add);
			Clock::tick();
			return 1;
			break;
		//Everything else is illegal
		case 7:
			printf("Illegal address mode: %ld", ir(DATA-5, DATA-7));
			exit(1);
	}
}

int main(int argc, char *argv[]){
	long opc;
	long temp;
	long output;
	long stride;
	long nextAddress;
	bool done = false;
	if(argc != 2) {
		cerr << "Usage: " << argv[0] << "object-file-name -prefetch\n\n";
		exit(1);
	}
	bool prefetch = false;
	if(argv[2] == "-n") {
		prefetch = true;
	}
	cout << hex;

	try{
		//create connections
		setup();
		//load object file and place 1st address into pc
		m.load(argv[1]);
		s3.latchFrom(m.READ());
		Clock::tick();
		abus.IN().pullFrom(s3);
		pc.latchFrom(abus.OUT());
		npc.latchFrom(abus.OUT());
		Clock::tick();

		//loop and execute instructions
		while(!done){
			fetch();
			opc = ir(DATA - 1, DATA-4);
			
			//decode
			//printing format is: pcaddress instr S0 S1 S2 S3
			switch(opc){
				//HALT
				//halt the machine
				case 0:
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3 = %ld\n", 
					pc(DATA-1, 0), "HLT", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					cout << "MACHINE halted due to halt instruction\n" << endl;
					done = true;
					break;
				//NOP
				//Essentially creates a stall
				case 1:
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "NOP", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//ADD
				//Adds two registers 
				case 2:
					//We know its an add so add to npc and prefetch next data
					npc.incr();
					add_sub_instruction(true);
					stride = npc(DATA-1, 0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1, 0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "ADD", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//SUB
				//Subtract two registers
				case 3:
					npc.incr();
					add_sub_instruction();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "SUB", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//STR
				//Store
				case 4:
					npc.incr();
					store();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "SUB", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//LDR
				//Load
				case 5:
					npc.incr();
					load();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "SUB", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//AND
				//And two registers
				case 6:
					npc.incr();
					and_or_instruction();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "AND", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//SRA
				//Shift register right
				case 7:
					npc.incr();
					shift_instruction(false);
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "SRA", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//SLL
				//Shift register left
				case 8:
					npc.incr();
					shift_instruction();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "SLL", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//JMP
				//Jump
				case 9:
					//no prefetch here because we have no idead where we are 
					//going with stride prediction
					compute_address();
					//move to npc
					abus.IN().pullFrom(s3);
					npc.latchFrom(abus.OUT());
					Clock::tick();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "JMP", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));	
					break;
				//BEZ
				//Branch on les than zero
				case 10:
					break;
				//BLT
				//Branch on less than 
				case 11:
					break;
				//CLR
				//Clear
				case 12:
					//just set the register to 0
					npc.incr();
					temp = ir(DATA-8, DATA-8);
					if(temp == 0){
						s1.clear();
					} else {
						s2.clear();
					}
					Clock::tick();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "CLR", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//CMP
				//Complement
				case 13:
					npc.incr();
					complement_instruction();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "CMP", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//INC
				//Increment
				case 14:
					npc.incr();
					temp = ir(DATA-8, DATA-8);
					if(temp == 0){
						s1.incr();
					} else {
						s2.incr();
					}
					Clock::tick();
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "INC", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));
					break;
				//OR
				//or two registers
				case 15:
					npc.incr();
					and_or_instruction(false);
					stride = npc(DATA-1,0) - pc(DATA-1, 0);
					nextAddress = npc(DATA-1,0) + stride;
					printf("Stride dist: %04x Next address to prefetch: %04x\n",
					stride, nextAddress);
					printf("%04x:  %s s0 = %ld s1 = %ld s2 = %ld s3  = %ld\n", 
					pc(DATA-1, 0), "CMP", s0(IMM-1, 0), 
					s1(DATA-1, 0), s2(DATA-1, 0), s3(DATA-1, 0));	
				//Other stuff
				default:
					break;
			}
			//move npc to pc
			abus.IN().pullFrom(npc);
			pc.latchFrom(abus.OUT());
			Clock::tick();
		}
	}
	catch(ArchLibError &err) {
		cout << endl << "Simulation aborted - ArchLib runtime error" << endl
		<< "Cause: " << err.what() << endl;
		return 1;
	}
	return 0;
}
