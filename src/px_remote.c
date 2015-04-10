#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <armci.h>
#include <sys/time.h>
#include "px_remote.h"
#include "px_checkpoint.h"


ARMCI_Group  g_world, my_grp;
/*Group and mpi ranks need to be same*/
int grp_my_rank, myrank, mypeer;
int nranks;
int grp_nproc;

int create_group ( int *members, int nmembers, int myrank,  int numrank);
void** group_create_memory(int nranks, size_t size);
int armci_remote_memcpy(int my_rank, int mypeer_rank,
				void **rmt_armci_ptr, size_t size);




int invoke_barrier() {
	ARMCI_Barrier();
	return 0;
}

/*We assume MPI_Init has already been invoked
 * If not, just add this line MPI_Init*/
int remote_init(int my_rank, int n_rank) {

	/*For now lets assume 1 buddy for each node*/
	int no_members=2;
	int members[no_members];
	int errors=-1;
	
	myrank = my_rank;
	nranks = n_rank;
	errors = ARMCI_Init();

	if(myrank % 2 == 0) {
		mypeer = myrank +1;
	}else {
		mypeer = myrank -1;
	}
	members[myrank % no_members] = myrank;
	members[mypeer % no_members] = mypeer;
	/*registering with armci-lib about our grouping*/
	create_group(members, no_members, myrank, nranks);
	return errors;
}


/*
	allocate remote memory.
	returns rmtptrs
		rmtprtrs[myrank] - 
*/
void* alloc_remote(void ***memory_grid, size_t size){

	void **rmtptrs;
	rmtptrs = group_create_memory(nranks, size);
	*memory_grid = rmtptrs;
	return rmtptrs[grp_my_rank];
}

int remote_write(int myrank,void ** memory_grid, size_t size){
	armci_remote_memcpy(myrank,mypeer,memory_grid, size);
	invoke_barrier();
	return 0;
}

/* TODO : not working like this */
int remote_read(int myrank, void** remote_ptr, size_t size){
	armci_remote_memcpy(mypeer, myrank,remote_ptr, size);
	invoke_barrier();
	return 0;
}

/* free up resources */
int remote_finalize(void){
	ARMCI_Finalize();
	return 0;
}




int create_group ( int *members, int nmembers, int myrank,  int numrank) {
	ARMCI_Group_get_world(&g_world);
	ARMCI_Group_create_child(nmembers, members, &my_grp, &g_world);
	ARMCI_Group_rank(&my_grp, &grp_my_rank);
	ARMCI_Group_size(&my_grp, &grp_nproc);
	return 0;
}

/* Function to group allocate a chunk.
 */
void** group_create_memory(int nranks, size_t size) {
	armci_size_t u_bytes = size;	
	void **rmt_armci_ptr;

	rmt_armci_ptr = (void **) calloc(nranks,sizeof(void *));
	assert(rmt_armci_ptr);
	ARMCI_Malloc_group(rmt_armci_ptr, u_bytes, &my_grp);
	ARMCI_Barrier();
	assert(rmt_armci_ptr);
	return rmt_armci_ptr;
}


int armci_remote_memcpy(int my_rank, int mypeer_rank,
				void **rmt_armci_ptr, size_t size){

	int gpeer_rank = 0;
	/* grp_my_rank, group_peer set when creating
	 * a group. These are not same as global
	 * rank and peer
	 */
	if(grp_my_rank == 0){
		gpeer_rank = grp_my_rank + 1;
	}else{
		gpeer_rank = grp_my_rank - 1;
	}

	ARMCI_Put(rmt_armci_ptr[grp_my_rank],rmt_armci_ptr[gpeer_rank],size,mypeer_rank);
	ARMCI_Barrier();
	return 0;
}
