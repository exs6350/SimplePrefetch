//Author: Ernesto Soltero
//File: Cache.cpp
#include "Cache.h"

using namespace std;

Cache::Cache(int numBits, bool prefetch=true):
i0("i0",numBits),
i1("i1", numBits),
i2("i2", numBits),
i3("i3", numBits),
i4("i4", numBits),
d0("d0", numBits),
d1("d1", numBits),
d2("d2", numBits),
d3("d3", numBits),
d4("d4", numBits),
prefetchAddr("pre", numBits),
cacheBus("CACHE", numBits),
temp("temp", numBits)
 {
	CACHE_DATA *instructionArray = new CACHE_DATA[5];
	CACHE_DATA *dataArray = new CACHE_DATA[5];
	//initialize the arrays to empty
	for(int i = 0; i < 5; i++){
		instructionArray[i].data = 0;
		instructionArray[i].address = 0;
		instructionArray[i].reg = 0;
		dataArray[i].data = 0;	
		dataArray[i].address = 0;
		dataArray[i].reg = 0;
	}
	prefetch = prefetch;
	size = numBits;
}


bool Cache::contains(long address, bool type){
	//check if prefetching is on
	if(!prefetch){return false;}
	//figure out if its a data or instruction that we're looking for
	//means its instruction type if true
	//use 4 LSB bits as hash 
	int index = (address & 0xF) % 5;
	if(type){
		if(instructionArray[index].address == address){
			return true;
		} else{ 
			return false;
		}
	} else{
		if(dataArray[index].address == address){
			return true;
		} else { 
			return false;
		}
	}
}

void Cache::setupConnections(Counter s3, Memory m, BusALU alu){
	d0.connectsTo(cacheBus.OUT());
	d0.connectsTo(cacheBus.IN());
	d1.connectsTo(cacheBus.OUT());
	d1.connectsTo(cacheBus.IN());
	d2.connectsTo(cacheBus.OUT());
	d2.connectsTo(cacheBus.IN());
	d3.connectsTo(cacheBus.OUT());
	d3.connectsTo(cacheBus.IN());
	d4.connectsTo(cacheBus.OUT());
	d4.connectsTo(cacheBus.IN());
	i0.connectsTo(cacheBus.OUT());
	i0.connectsTo(cacheBus.IN());
	i1.connectsTo(cacheBus.OUT());
	i1.connectsTo(cacheBus.IN());
	i2.connectsTo(cacheBus.OUT());
	i2.connectsTo(cacheBus.IN());
	i3.connectsTo(cacheBus.OUT());
	i3.connectsTo(cacheBus.IN());
	i4.connectsTo(cacheBus.OUT());
	i4.connectsTo(cacheBus.IN());

	//scratch register for moving to memory
	temp.connectsTo(cacheBus.IN());
	temp.connectsTo(cacheBus.OUT());
	temp.connectsTo(m.READ());
	temp.connectsTo(m.WRITE());
	prefetchAddr.connectsTo(m.WRITE());	

	//memory 
	m.MAR().connectsTo(cacheBus.OUT());
	
	//s3 connections
	s3.connectsTo(cacheBus.IN());
	s3.connectsTo(cacheBus.OUT());

	//Bus alu 
	prefetchAddr.connectsTo(alu.OUT());
}

void Cache::preFetch(bool type, Memory m){
	long address = prefetchAddr(size - 1, 0);
	//first check if this is already in the cache
	if(contains(address, type)){
		return;
	}
	//check if prefetching is on
	if(!prefetch){
		return;
	}
	//grab from memory
	cacheBus.IN().pullFrom(prefetchAddr);
	m.MAR().latchFrom(cacheBus.OUT());
	Clock::tick();
	m.read();
	temp.latchFrom(m.READ());	
	Clock::tick();
	cacheBus.IN().pullFrom(temp);
	prefetchAddr.latchFrom(cacheBus.OUT());
	Clock::tick();
	int index = (address & 0xF) % 5;
	//figure out where to put it
	//instruction type
	if(type){
		//we dont really care about swapping instructions so just overwrite
		instructionArray[index].address = address;
		instructionArray[index].data = prefetchAddr(size - 1, 0);
		instructionArray[index].reg = index;		
		//figure out the register to store 
		cacheBus.IN().pullFrom(prefetchAddr);
		switch(index){
			case 0:
				i0.latchFrom(cacheBus.OUT());
				instructionArray[index].reg = 0;
				break;
			case 1:
				i1.latchFrom(cacheBus.OUT());
				instructionArray[index].reg = 1;
				break;
			case 2:
				i2.latchFrom(cacheBus.OUT());
				instructionArray[index].reg = 2;
				break;
			case 3:
				i3.latchFrom(cacheBus.OUT());
				instructionArray[index].reg = 3;
				break;
			case 4:
				i4.latchFrom(cacheBus.OUT());
				instructionArray[index].reg = 4;
				break;
		}
	} else{
		//we really care about swapping data so we have to write back into 
		//memory if its not the same 
		if(dataArray[index].address != 0){
			//write into mem
			switch(index){
				case 0:
					cacheBus.IN().pullFrom(d0);
					break;
				case 1:
					cacheBus.IN().pullFrom(d1);
					break;
				case 2:
					cacheBus.IN().pullFrom(d2);
					break;
				case 3:
					cacheBus.IN().pullFrom(d3);
					break;
				case 4:
					cacheBus.IN().pullFrom(d4);
					break;
			}
			temp.latchFrom(cacheBus.OUT());
			//want to avoiding having to add in another register so 
			//kind of cheating and setting the address value using backdoor
			m.MAR().backDoor(dataArray[index].address);
			Clock::tick();
			m.WRITE().pullFrom(temp);
			m.write();
			Clock::tick();
			//wrote data now insert into reg
			dataArray[index].address = address;
			dataArray[index].data = prefetchAddr(size - 1, 0);
			cacheBus.IN().pullFrom(prefetchAddr);
			switch(index){
				case 0:
					dataArray[index].reg = 0;
					d0.latchFrom(cacheBus.OUT());
					break;
				case 1:
					dataArray[index].reg = 1;
					d1.latchFrom(cacheBus.OUT());
					break;
				case 2:
					dataArray[index].reg = 2;
					d2.latchFrom(cacheBus.OUT());
					break;
				case 3:
					dataArray[index].reg = 3;
					d3.latchFrom(cacheBus.OUT());
					break;
				case 4:
					dataArray[index].reg = 4;
					d4.latchFrom(cacheBus.OUT());
					break;
			}
		} else{
			//safe to write nothing in register
			dataArray[index].address = address;
			dataArray[index].data = prefetchAddr(size - 1, 0);
			cacheBus.IN().pullFrom(prefetchAddr);
			switch(index){
				case 0:
					dataArray[index].reg = 0;
					d0.latchFrom(cacheBus.OUT());
					break;
				case 1:
					dataArray[index].reg = 1;
					d1.latchFrom(cacheBus.OUT());
					break;
				case 2:
					dataArray[index].reg = 2;
					d2.latchFrom(cacheBus.OUT());
					break;
				case 3:
					dataArray[index]. reg = 3;
					d3.latchFrom(cacheBus.OUT());
					break;
				case 4:
					dataArray[index].reg = 4;
					d4.latchFrom(cacheBus.OUT());
					break;
			}
		}
	}
}


bool Cache::retrieve(long address, bool type, Counter s3){
	//check if prefetching is on
	if(!prefetch){
		return false;
	}
	//check if we have what were looking for 
	if(!contains(address, type)){
		return false;
	}
	//We do have address so find it and place it in s3
	int index = (address & 0xF) % 5;
	if(type){
		switch(index){
			case 0:
				cacheBus.IN().pullFrom(i0);
				break;
			case 1:
				cacheBus.IN().pullFrom(i1);
			case 2:
				cacheBus.IN().pullFrom(i2);
				break;
			case 3:
				cacheBus.IN().pullFrom(i3);
				break;
			case 4:
				cacheBus.IN().pullFrom(i4);
				break;
		}
		s3.latchFrom(cacheBus.OUT());
	} else{
		switch(index){
			case 0:
				cacheBus.IN().pullFrom(d0);
				break;
			case 1:
				cacheBus.IN().pullFrom(d1);
			case 2:
				cacheBus.IN().pullFrom(d2);
				break;
			case 3:
				cacheBus.IN().pullFrom(d3);
				break;
			case 4:
				cacheBus.IN().pullFrom(d4);
				break;
		}
		s3.latchFrom(cacheBus.OUT());
	}
	Clock::tick();
	return true;
}

void Cache::debugInfo(){
	printf("Printing put the Cache contents\n\n");
	printf("Instruction cache contents:\n");
	printf("i0: %16x address: %16x\n", instructionArray[0].data, 
		instructionArray[0].address); 	
	printf("i1: %16x address: %16x\n", instructionArray[1].data, 
		instructionArray[1].address);
	printf("i2: %16x address: %16x\n", instructionArray[2].data, 
		instructionArray[2].address); 
	printf("i3: %16x address: %16x\n", instructionArray[3].data, 
		instructionArray[3].address); 
	printf("i4: %16x address: %16x\n\n", instructionArray[4].data, 
		instructionArray[4].address); 
	printf("Data cache contents:\n");
	printf("d0: %16x address: %16x\n", dataArray[0].data,
		dataArray[0].address);
	printf("d1: %16x address: %16x\n", dataArray[1].data,
		dataArray[1].address);
	printf("d0: %16x address: %16x\n", dataArray[2].data,
		dataArray[2].address);
	printf("d0: %16x address: %16x\n", dataArray[3].data,
		dataArray[3].address);
	printf("d0: %16x address: %16x\n\n", dataArray[4].data,
		dataArray[4].address);
}
