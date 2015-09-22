#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <armci.h>
#include "px_remote.h"
#include "px_debug.h"


ARMCI_Group  g_world, my_grp;
/*Group and mpi ranks need to be same*/
int grp_my_rank, myrank, mypeer;
int nranks;
int grp_nproc;

int create_group ( int *members, int nmembers, int myrank,  int numrank);
void** group_create_memory(int nranks, size_t size);
int armci_remote_memcpy(void *src, int mypeer_rank,
				void **rmt_armci_ptr, size_t size);
int get_mypeer_group(int grp_my_rank);

int remote_barrier() {
	ARMCI_Barrier();
	return 0;
}

extern int buddy_offset;

/*We assume MPI_Init has already been invoked
 * If not, just add this line MPI_Init*/
int remote_init(int my_rank, int n_rank) {

	/*For now lets assume 1 buddy for each node*/
	int no_members=2;
	int members[2];
	int errors=0;

	myrank = my_rank;
	nranks = n_rank;
	errors = ARMCI_Init();

    assert( buddy_offset < n_rank && "n_offset should be smaller than the total rank\n");

	if(myrank % 2 == 0) {
		mypeer = (myrank + buddy_offset) % n_rank;
	}else {
        if(my_rank >= buddy_offset){
            mypeer = my_rank - buddy_offset;
        } else{
            mypeer = buddy_offset - my_rank;
        }
	}
    /*
     * Here we are making sure that both me and my peer create a group with the same rank assignments.
     * Eg: my rank = 5, then my peer rank = 4
     *
     * members array in me;  members[1] = 5, members[0] = 4
     * members array in my peer; members[0] = 4, members[1] = 5
     *
     * my peer and me agrres on a identical group ranks. :)
     */
	members[myrank % no_members] = myrank;
	members[mypeer % no_members] = mypeer;
//	if(isDebugEnabled()){
//		printf("Rank %d , members[%d] = %d \n",myrank,myrank%no_members,myrank);
//		printf("Rank %d , members[%d] = %d \n",myrank,mypeer%no_members,mypeer);
//	}
	/*registering with armci-lib about our grouping*/
	create_group(members, no_members, myrank, nranks);
	return errors;
}


/*
	allocate memory grid in the armci group.
	returns rmtprtrs[myrank] - the local pointer in memory grid
*/
void* remote_alloc(void ***memory_grid, size_t size){

	void **rmtptrs;
	rmtptrs = group_create_memory(nranks, size);
	*memory_grid = rmtptrs;
//	if(isDebugEnabled()){
//		printf("rmtptrs[0] = %p\n", rmtptrs[0]);
//		printf("rmtptrs[1] = %p\n", rmtptrs[1]);
//	}
	return rmtptrs[grp_my_rank];
}

int remote_free(void *ptr){
	int status = ARMCI_Free_group(ptr,&my_grp); 
	if(status){
		printf("Error : freeing memory");
		assert(0);
	}
	return 0;
}


int remote_write(void *src,void **memory_grid, size_t size){
	int peer = get_mypeer_group(grp_my_rank);
	if(isDebugEnabled()){
		printf("[%d] writing data to remote node,  remote_rank : %d remote addr : %p "
					"local src addr : %p \n",myrank, mypeer, memory_grid[peer],src);
	}
	int status = ARMCI_Put(src,memory_grid[peer],size,mypeer);
	if(status){
		printf("Error: copying data to remote node.\n");
		assert(0);	
	}
	return 0;
}

int remote_read(void *dest, void **memory_grid, size_t size){
	int peer = get_mypeer_group(grp_my_rank);
	//if(isDebugEnabled()){
	//	printf("Reading data local_rank:%d remote_rank : %d remote addr : %p " 
	//				 "local dest addr : %p \n", myrank, mypeer, memory_grid[peer],dest);
	//}
	int status = ARMCI_Get(memory_grid[peer],dest,size,mypeer);
	if(status){
		printf("Error: copying data from remote node.\n");
		assert(0);	
	}
	return 0;
}


/* free up resources */
int remote_finalize(void){
	ARMCI_Finalize();
	return 0;
}




int create_group ( int *members, int nmembers, int myrank,  int numrank) {
	if(isDebugEnabled()){
		printf("Creating a member group..\n");
	}
	ARMCI_Group_get_world(&g_world);
	ARMCI_Group_create_child(nmembers, members, &my_grp, &g_world);
	ARMCI_Group_rank(&my_grp, &grp_my_rank);
	ARMCI_Group_size(&my_grp, &grp_nproc);
	if(isDebugEnabled()){
		printf("Done creating the group..\n");
	}
	return 0;
}

/* Function to group allocate a chunk.
 */
void** group_create_memory(int nranks, size_t size) {
	armci_size_t u_bytes = size;	
	void **rmt_armci_ptr;

	rmt_armci_ptr = (void **) calloc(nranks,sizeof(void *));
	int status = ARMCI_Malloc_group(rmt_armci_ptr, u_bytes, &my_grp);
	if(status){
		printf("Error: creating group memory\n");
		assert(0);
	}
	ARMCI_Barrier();
	assert(rmt_armci_ptr);
	return rmt_armci_ptr;
}

int get_mypeer_group(int grp_my_rank){
	int gpeer_rank = 0;
	if(grp_my_rank == 0){
		gpeer_rank = grp_my_rank + 1;
		return gpeer_rank;
	}
	gpeer_rank = grp_my_rank - 1;
	return gpeer_rank;	
}

