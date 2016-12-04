#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr = 0;

	assert(log_block_nr >= 0);
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
	} else {
		log_block_nr -= NR_DIRECT_BLOCKS;

		if (log_block_nr >= NR_INDIRECT_BLOCKS){
			//Lab 6 code
			log_block_nr -= NR_INDIRECT_BLOCKS;
		
			if(log_block_nr >= NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS){
				//return if block # greater than double indirect capacity
				return -EFBIG;
			}
			if(in->in.i_dindirect > 0){
				read_blocks(in->sb, block, in->in.i_dindirect, 1);
				int i_index = log_block_nr/NR_INDIRECT_BLOCKS;
				int di_addr = ((int *)block)[i_index];
				if(di_addr > 0){
					read_blocks(in->sb, block, di_addr, 1);
					//calculate offset in double indirect table
					int di_index = log_block_nr % NR_INDIRECT_BLOCKS;
					phy_block_nr = ((int*)block)[di_index];
				}
				else{//di_addr is not allocated. return -ve value
					return di_addr;
				}
			}

		}
		
		else if (in->in.i_indirect > 0) {
			read_blocks(in->sb, block, in->in.i_indirect, 1);
			phy_block_nr = ((int *)block)[log_block_nr];
		}
	}
	if (phy_block_nr > 0) {
		read_blocks(in->sb, block, phy_block_nr, 1);
	} else {
		/* we support sparse files by zeroing out a block that is not
		 * allocated on disk. */
		bzero(block, BLOCK_SIZE);
	}
	return phy_block_nr;
}

int
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;

	assert(buf);
	if (start + (off_t) size > in->in.i_size) {
		size = in->in.i_size - start;
	}
	if (block_ix + size > BLOCK_SIZE) {
		//Lab 6 code
		//if(size > /*9999999999){*/NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS){// + NR_INDIRECT_BLOCKS){
		//	return -EFBIG;
		//}
		ret = testfs_read_block(in, block_nr, block);
		if(ret < 0){
			return ret; //invalid negative
		}
		//copy first block. This block may contain some offset
		memcpy(buf, block+block_ix, BLOCK_SIZE - block_ix);
		int leftover_size = size - BLOCK_SIZE + block_ix;
		int block_increment = 1;
		while(leftover_size > BLOCK_SIZE){
			ret = testfs_read_block(in, block_nr + block_increment, block);
			if(ret < 0){
				return ret; //negative return
			}
			memcpy(buf - block_ix + (BLOCK_SIZE*block_increment), block, BLOCK_SIZE);
			leftover_size -= BLOCK_SIZE;
			block_increment++;
		}
		//copy last block. This copy size may be less than block size
		ret = testfs_read_block(in, block_nr + block_increment, block);
		if(ret < 0){
			return ret;
		}
		memcpy(buf + (BLOCK_SIZE*block_increment) - block_ix, block, leftover_size);
		return size;
	}
	if ((ret = testfs_read_block(in, block_nr, block)) < 0)
		return ret;
	memcpy(buf, block + block_ix, size);
	/* return the number of bytes read or any error */
	return size;
}

/* given logical block number, allocate a new physical block, if it does not
 * exist already, and return the physical block number that is allocated.
 * returns negative value on error. */
static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr;
	char indirect[BLOCK_SIZE];
	int indirect_allocated = 0;

	assert(log_block_nr >= 0);
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
	if (phy_block_nr != 0)
		return phy_block_nr;

	/* allocate a direct block */
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		assert(in->in.i_block_nr[log_block_nr] == 0);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr >= 0) {
			in->in.i_block_nr[log_block_nr] = phy_block_nr;
		}
		return phy_block_nr;
	}

	log_block_nr -= NR_DIRECT_BLOCKS;
	if (log_block_nr >= NR_INDIRECT_BLOCKS){
		//Lab6 code
		log_block_nr -= NR_INDIRECT_BLOCKS;
		if(log_block_nr >= NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS){
			return -EFBIG;
		}
		char di_block[BLOCK_SIZE];
		int di_block_available = 0;
		int di_index = log_block_nr/NR_INDIRECT_BLOCKS;
		
		//allocate the double indirect block
		if(in->in.i_dindirect == 0){
			bzero(di_block, BLOCK_SIZE);
			phy_block_nr = testfs_alloc_block_for_inode(in);
			if(phy_block_nr < 0){
				return phy_block_nr;
			}
			di_block_available = 1;
			in->in.i_dindirect = phy_block_nr;
		}
		else{
			read_blocks(in->sb, di_block, in->in.i_dindirect, 1);
		}

		//allocate the indirect blocks in the double indirect block
		if(((int*)di_block)[di_index] == 0){
			bzero(indirect, BLOCK_SIZE);
			phy_block_nr = testfs_alloc_block_for_inode(in);
			if(phy_block_nr < 0){
				if(di_block_available){
					testfs_free_block_from_inode(in, in->in.i_dindirect);
				}
				return phy_block_nr;
				
			}
			else{
				((int*)di_block)[di_index] = phy_block_nr;
				write_blocks(in->sb, di_block, in->in.i_dindirect, 1);
				indirect_allocated = 1;
			}
		}
		else{
			read_blocks(in->sb, indirect, ((int*)di_block)[di_index], 1);
		}

		//allocate the direct blocks of the indirect blocks
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if(phy_block_nr < 0){
			if(di_block_available){
				testfs_free_block_from_inode(in, ((int*)di_block)[di_index]);
				testfs_free_block_from_inode(in, in->in.i_dindirect);
			}
			else if(indirect_allocated){
				testfs_free_block_from_inode(in, ((int*)di_block)[di_index]);
			}
			return phy_block_nr;
		}
		else{// allocate the physical block to the inode as direct block
			((int*)indirect)[log_block_nr % NR_INDIRECT_BLOCKS] = phy_block_nr;
			write_blocks(in->sb, indirect, ((int*)di_block)[di_index], 1);
		}
		return phy_block_nr;
	}

	if (in->in.i_indirect == 0) {	/* allocate an indirect block */
		bzero(indirect, BLOCK_SIZE);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr < 0)
			return phy_block_nr;
		indirect_allocated = 1;
		in->in.i_indirect = phy_block_nr;
	} else {	/* read indirect block */
		read_blocks(in->sb, indirect, in->in.i_indirect, 1);
	}

	/* allocate direct block */
	assert(((int *)indirect)[log_block_nr] == 0);	
	phy_block_nr = testfs_alloc_block_for_inode(in);

	if (phy_block_nr >= 0) {
		/* update indirect block */
		((int *)indirect)[log_block_nr] = phy_block_nr;
		write_blocks(in->sb, indirect, in->in.i_indirect, 1);
	} else if (indirect_allocated) {
		/* free the indirect block that was allocated */
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}
	return phy_block_nr;
}

int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;

	if (block_ix + size > BLOCK_SIZE) {
		//Lab6 code
		ret = testfs_allocate_block(in, block_nr, block);
		if(ret < 0){
			return ret;
		}
		//write the first block with the offset
		memcpy(block + block_ix, buf, BLOCK_SIZE - block_ix);
		write_blocks(in->sb, block, ret, 1);
		int leftover_size = size - BLOCK_SIZE + block_ix;
		int increment = 1;
		while(leftover_size > BLOCK_SIZE){
			ret = testfs_allocate_block(in, block_nr + increment, block);
			if(ret < 0){
				return ret;
			}
			memcpy(block, buf + ((BLOCK_SIZE*increment) - block_ix), BLOCK_SIZE);
			write_blocks(in->sb, block, ret, 1);
			leftover_size -= BLOCK_SIZE;
			increment++;
		}
		ret = testfs_allocate_block(in, block_nr + increment, block);
		if(ret < 0){
			in->in.i_size = 34376597504;//magic number 
			in->i_flags = in->i_flags|I_FLAGS_DIRTY;
			return ret;
		}
		memcpy(block, buf + ((BLOCK_SIZE*increment) - block_ix), leftover_size);
		write_blocks(in->sb, block, ret, 1);
		if(size > 0){
			in->in.i_size = MAX(in->in.i_size, start + (off_t)size);
		}	
		in->i_flags |= I_FLAGS_DIRTY;
		return size;

	}
	/* ret is the newly allocated physical block number */
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0)
		return ret;
	memcpy(block + block_ix, buf, size);
	write_blocks(in->sb, block, ret, 1);
	/* increment i_size by the number of bytes written. */
	if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
	in->i_flags |= I_FLAGS_DIRTY;
	/* return the number of bytes written or any error */
	return size;
}

int
testfs_free_blocks(struct inode *in)
{
	int i;
	int e_block_nr;

	/* last block number */
	e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

	/* remove direct blocks */
	for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
		if (in->in.i_block_nr[i] == 0)
			continue;
		testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
		in->in.i_block_nr[i] = 0;
	}
	e_block_nr -= NR_DIRECT_BLOCKS;

	/* remove indirect blocks */
	if (in->in.i_indirect > 0) {
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_indirect, 1);
		for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
			testfs_free_block_from_inode(in, ((int *)block)[i]);
			((int *)block)[i] = 0;
		}
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}

	e_block_nr -= NR_INDIRECT_BLOCKS;
	if (e_block_nr >= 0) {
		//lab6 code
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_dindirect, 1);
		int counter = 0;
		int j;
		for(i = 0; counter < e_block_nr && i < NR_INDIRECT_BLOCKS; i++){
			if(((int*)block)[i] <= 0){
				counter += NR_INDIRECT_BLOCKS;
				continue;
			}
			if(((int*)block)[i] > 0){
				char direct_block[BLOCK_SIZE];
				read_blocks(in->sb, direct_block, ((int*)block)[i], 1);
				for(j = 0; counter < e_block_nr && j < NR_INDIRECT_BLOCKS; j++){
					testfs_free_block_from_inode(in, ((int*)direct_block)[j]);
					((int*)direct_block)[j] = 0;
					counter++;
				}
				testfs_free_block_from_inode(in, ((int*)block)[i]);
				((int*)block)[i] = 0;
			}
			
		}
		testfs_free_block_from_inode(in, in->in.i_dindirect);
		in->in.i_dindirect = 0;
	}
	
	in->in.i_size = 0;
	in->i_flags |= I_FLAGS_DIRTY;
	return 0;
}
