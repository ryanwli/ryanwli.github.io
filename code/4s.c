#include <stdio.h>

//answer a
typedef struct {
	
	int id;
	char* name;
	char* reg;
	int numOfAddrs;
	char* addrs[];
} account;

//answer b
int listMatchAccounts(char* addr, account allAccounts[], int numOfAccounts)
{
	int matchCount = 0;
	for (int i = 0; i < numOfAccounts; ++i)
	{
		if (allAccounts[i].numOfAddrs > 0 && allAccounts[i].addrs[0] == addr)
		{
			printf("name:%s, address:%s\n", allAccounts[i].name, allAccounts[i].addrs[0]);
			matchCount++;
		}
	}
	return matchCount;
};

//test
int main(int argc, char const *argv[])
{
	account data[3] = {{1,"ryan","AECCS",1},{2,"jack","ESCSA",1},{1,"helen","JKDDC",1}};
	data[0].addrs[0] = "chengdu";
	data[0].numOfAddrs = 1;
	data[1].addrs[0] = "chengdu";
	data[1].numOfAddrs = 1;
	data[2].addrs[0] = "shanghai";
	data[2].numOfAddrs = 1;


	printf("total records:%d\n", listMatchAccounts("chengdu", data, 3));
 
	return 0;
};

//answer c, research in google to find answer...

//print out--------------------
/**
name:ryan, address:chengdu
name:jack, address:chengdu
total records:2
**/

