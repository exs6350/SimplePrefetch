#ifndef _CACHE_H_
#define _CACHE_H_

#include <StorageObject.h>
#include <Bus.h>
#include <Memory.h>
#include <Counter.h>
#include <BusALU.h>
#include <Clock.h>
#include <iostream>
#include <stdio.h>
#include <cstdlib>

using namespace std;

struct cache_data{
	long data;
	long address;
	int reg;
} typedef CACHE_DATA;

class Cache{
	public:
		Cache(int, bool);
		~Cache();
		void setupConnections( Counter, Memory, BusALU);
		bool contains(long, bool);
		void preFetch(bool, Memory);
		int miss;
		int hits;
		Counter prefetchAddr;
		void debugInfo(void);
		bool retrieve(long, bool, Counter);

	private:
		bool prefetch;
		Bus cacheBus;
		StorageObject temp;
		StorageObject i0;
		StorageObject i1;
		StorageObject i2;
		StorageObject i3;
		StorageObject i4;
		StorageObject d0;
		StorageObject d1;
		StorageObject d2;
		StorageObject d3;
		StorageObject d4;
		CACHE_DATA instructionArray[5];
		CACHE_DATA dataArray[5];	
		void swap(long,long,bool);
		long prefetchAddress;		
		int size;
};
#endif
