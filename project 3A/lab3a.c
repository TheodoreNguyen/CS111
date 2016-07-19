#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/types.h>

#define max_buffer_size 5000

struct ext2_super_block {
	__u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
	__u32	s_free_blocks_count;	/* Free blocks count */
	__u32	s_free_inodes_count;	/* Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
	__u32	s_log_block_size;	/* Block size */
	__s32	s_log_frag_size;	/* Fragment size */
	__u32	s_blocks_per_group;	/* # Blocks per group */
	__u32	s_frags_per_group;	/* # Fragments per group */
	__u32	s_inodes_per_group;	/* # Inodes per group */
	__u32	s_mtime;		/* Mount time */
	__u32	s_wtime;		/* Write time */
	__u16	s_mnt_count;		/* Mount count */
	__s16	s_max_mnt_count;	/* Maximal mount count */
	__u16	s_magic;		/* Magic signature */
	__u16	s_state;		/* File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level; 	/* minor revision level */
	__u32	s_lastcheck;		/* time of last check */
	__u32	s_checkinterval;	/* max. time between checks */
	__u32	s_creator_os;		/* OS */
	__u32	s_rev_level;		/* Revision level */
	__u16	s_def_resuid;		/* Default uid for reserved blocks */
	__u16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__u32	s_first_ino; 		/* First non-reserved inode */
	__u16   s_inode_size; 		/* size of inode structure */
	__u16	s_block_group_nr; 	/* block group # of this superblock */
	__u32	s_feature_compat; 	/* compatible feature set */
	__u32	s_feature_incompat; 	/* incompatible feature set */
	__u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	__u32	s_reserved[230];	/* Padding to the end of the block */
};

struct ext2_group_desc
{
	__u32	bg_block_bitmap;		/* Blocks bitmap block */
	__u32	bg_inode_bitmap;		/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_pad;
	__u32	bg_reserved[3];
};

struct ext2_inode {

	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Creation time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32  l_i_reserved1;
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
		struct {
			__u32  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */

	__u32	i_block[15];/* Pointers to blocks */
	__u32	i_version;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_dir_acl;	/* Directory ACL */
	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__u16	i_pad1;
			__u32	l_i_reserved2[2];
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

struct ext2_dir_entry {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u16	name_len;		/* Name length */
	char	name[255];	/* File name */
};

struct ext2_dir_entry_2 {
	__u32   inode;          /* Inode number */
	__u16   rec_len;        /* Directory entry length */
	__u8    name_len;       /* Name length */
	__u8    file_type;
	char    name[255];    /* File name */
 };

struct block {
	int address;
	struct block *next;
	__u32 con_block;
};

struct block* indirect_read_inodeblk(struct block* t, int block_num, int fd, __u32 block_size, int opt)
{
	if(opt != 1 || opt != 2 || opt != 3)
	{
		perror("need correct options.");
		exit(1);
	}
	int i;
	__u32 *blk = malloc(block_size);
	pread(fd, blk, block_size, block_num * block_size);
	for(i = 0; i < block_size / 4; i++)
	{
		if(opt == 1)
		{
			if(blk[i])
			{
				struct block* element = malloc(sizeof(struct block));
				element->next = NULL;
				element->address = blk[i] * block_size;
				element->con_block = block_num;
				t->next = element;
				t = element;
			}
		}
		else if(opt == 2 || opt == 3)
		{
			if(blk[i])
			{
				t = indirect_read_inodeblk(t, blk[i], fd, block_size, 1);
			}
		}
		else
		{
			perror("how did you get here??");
			exit(1);
		}
	}
	free(blk);
	return t;
}

void indirect_write_inodeblk(int block_num, int fd, __u32 block_size, int opt, int outputfd)
{
	int i, ret;
	__u32 *blk = malloc(sizeof(__u32));
	char indirect_buffer[max_buffer_size];
	pread(fd, blk, block_size, block_num * block_size);
	for(i = 0; i < block_size/4; i++)
	{
		if(blk[i])
		{
			ret = sprintf(indirect_buffer, "%x,%"PRIu32",%x,\n",
				block_num, i, blk[i]);
			write(outputfd, indirect_buffer, ret);
		}
	}
	if(opt == 2 || opt == 3)
	{
		for(i = 0; i < block_size/4; i++)
			if(blk[i])
				indirect_write_inodeblk(blk[i], fd, block_size, 1, outputfd);
	}
}

int main(int argc, char* argv[])
{
	//check if number of args sent correctly
	if(argc < 2 || argc >= 3)
	{
		perror("Correct usage is ./lab3a <filename>");
		exit(1);
	}

	//open the image file
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		perror("Image open failure.");
		exit(1);
	}

	int i, j, k, n;


	//write the superblock
	//-----------------------------------------------------------------------
	struct ext2_super_block superblock;
	int ret = pread(fd, &superblock, 1024, 1024);

	__u32 block_size = 1024 << superblock.s_log_block_size;

	__s32 fragment_shift = (__s32) superblock.s_log_frag_size;
	__u32 fragment_size = 0;
	if(fragment_shift > 0)
		fragment_size = 1024 << fragment_shift;
	else
		fragment_size = 1024 >> -fragment_shift;

	int superblock_fd = open("super.csv", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if(superblock_fd == -1)
	{
		perror("Error creating output file super.csv");
		exit(1);
	}
	char superblock_buffer[max_buffer_size];
	ret = sprintf(superblock_buffer, "%x,%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32"\n",
		superblock.s_magic, superblock.s_inodes_count, superblock.s_blocks_count, block_size,
		fragment_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group, superblock.s_frags_per_group,
		superblock.s_first_data_block);
	int super_csv_write = write(superblock_fd, superblock_buffer, ret);
	//-----------------------------------------------------------------------



	//write the group descriptors
	//-----------------------------------------------------------------------
	__u32 num_groups = 1 + (superblock.s_blocks_count-1) / superblock.s_blocks_per_group;
	__u32 descr_list_size = num_groups * sizeof(struct ext2_group_desc);
	int offset = (superblock.s_first_data_block + 1) * block_size;
	struct ext2_group_desc group_desc[num_groups];
	for(i = 0; i != num_groups; i++)
	{
		ret = pread(fd, &group_desc[i], sizeof(struct ext2_group_desc), offset);
		offset += sizeof(struct ext2_group_desc);
	}

	int group_desc_fd = open("group.csv", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if(group_desc_fd == -1)
	{
		perror("Error creating output file group.csv");
		exit(1);
	}
	char* group_desc_buffer = malloc(sizeof(max_buffer_size)*num_groups);
	char each_buffer[max_buffer_size];
	for(i = 0; i != num_groups; i++)
	{
		__u32 num_contained_blocks = superblock.s_blocks_per_group;
		if(i == num_groups - 1)
			num_contained_blocks = superblock.s_blocks_count % superblock.s_blocks_per_group;
		ret = sprintf(each_buffer,"%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%x,%x,%x\n", num_contained_blocks,
			group_desc[i].bg_free_blocks_count, group_desc[i].bg_free_inodes_count, group_desc[i].bg_used_dirs_count,
			group_desc[i].bg_inode_bitmap, group_desc[i].bg_block_bitmap, group_desc[i].bg_inode_table);
		strcat(group_desc_buffer, each_buffer);
	}
	int group_csv_write = write(group_desc_fd, group_desc_buffer, strlen(group_desc_buffer));
	//------------------------------------------------------------------------



	//write the free bitmap entry
	//------------------------------------------------------------------------
	int inode_number = 1;
	int block_number = 1;
	int inode_bound = 0;
	int block_bound = 0;
	char bitmap_buffer[max_buffer_size];
	__u8 *inode_bitmap_block = malloc(sizeof(__u8)*block_size);
	__u8 *block_bitmap_block = malloc(sizeof(__u8)*block_size);

	int bitmap_fd = creat("bitmap.csv", 0666);
	if(bitmap_fd == -1)
	{
		perror("Error creating output file bitmap.csv");
		exit(1);
	}

	//over the total number of groups (there is a inode/block bitmap per block group)
	for(i = 0; i != num_groups; i++)
	{
		block_bound += superblock.s_blocks_per_group;
		inode_bound += superblock.s_inodes_per_group;
		if(i == num_groups - 1)
		{
			block_bound = superblock.s_blocks_count;
			inode_bound = superblock.s_inodes_count;
		}

		ret = pread(fd, block_bitmap_block, block_size, block_size * group_desc[i].bg_block_bitmap);
		//over the total number of bytes in the the bitmap
		for(j = 0; j != block_size; j++)
		{
			//over the number of bits in each byte in each block bitmap
			for(k = 0; k != 8; k++)
			{
				if(block_number >= block_bound)
					break;
				if((block_bitmap_block[j] & (1 << k)) == 0)
				{
					ret = sprintf(bitmap_buffer, "%x,%"PRIu32"\n",
						group_desc[i].bg_block_bitmap, block_number);
					write(bitmap_fd, bitmap_buffer, ret);
				}
				block_number++;
			}
		}
		ret = pread(fd, inode_bitmap_block, block_size, block_size * group_desc[i].bg_inode_bitmap);
		for(j = 0; j != block_size; j++)
		{
			//over the number of bits in each byte in each inode bitmap
			for(k = 0; k != 8; k++)
			{
				if(inode_number >= inode_bound)
					break;
				if((inode_bitmap_block[j] & (1 << k)) == 0)
				{
					ret = sprintf(bitmap_buffer, "%x,%"PRIu32"\n",
						group_desc[i].bg_inode_bitmap, inode_number);
					write(bitmap_fd, bitmap_buffer, ret);
				}
				inode_number++;
			}
		}
	}
	//------------------------------------------------------------------------



	//write the inode
	//-----------------------------------------------------------------------


	char inode_buffer[max_buffer_size];
	inode_number = 1;
	inode_bound = 0;
	int inode_fd = creat("inode.csv", 0666);
	if(inode_fd == -1)
	{
		perror("Error creating output file inode.csv");
		exit(1);
	}
	__u8 *inode_bitmap_block2 = malloc(sizeof(__u8)*block_size);
	struct ext2_inode inode;

	for(i = 0; i != num_groups; i++)
	{
		inode_bound += superblock.s_inodes_per_group;
		if(i == num_groups - 1)
			inode_bound = superblock.s_inodes_count;

		ret = pread(fd, inode_bitmap_block2, block_size, block_size * group_desc[i].bg_inode_bitmap);
		for(j = 0; j != block_size; j++)
		{
			if(inode_number >= inode_bound)
				break;
			for(k = 0; k != 8; k++)
			{
				if((inode_bitmap_block2[j] & (1 << k)) != 0)				//(group_desc[i].bg_inode_table + inode_number*sizeof(struct ext2_inode))
				{
					ret = pread(fd, &inode, sizeof(struct ext2_inode), group_desc[i].bg_inode_table * block_size + sizeof(struct ext2_inode)*(j*8+k));
					char type;
					if(inode.i_mode & 0xA000)
						type = 's';
					else if(inode.i_mode & 0x8000)
						type = 'f';
					else if(inode.i_mode & 0x4000)
						type = 'd';
					else
						type = '?';

					__u32 ownerID = (inode.osd2.hurd2.h_i_uid_high << 16) + inode.i_uid;
					__u32 groupID = (inode.osd2.hurd2.h_i_gid_high << 16) + inode.i_gid;
					__u32 blocks = inode.i_blocks/ (2 << superblock.s_log_block_size);
					ret = sprintf(inode_buffer, "%"PRIu32",%c,%o,%"PRIu32",%"PRIu32",%"PRIu32",%x,%x,%x,%"PRIu32",%"PRIu32"",
						inode_number, type, inode.i_mode, ownerID, groupID, inode.i_links_count, inode.i_ctime, inode.i_mtime,
						inode.i_atime, inode.i_size, inode.i_blocks);
					write(inode_fd, inode_buffer, ret);
					for(n = 0; n != 15; n++)
					{
						ret = sprintf(inode_buffer, ",%x", inode.i_block[n]);
						write(inode_fd, inode_buffer, ret);
					}
					write(inode_fd, "\n", 1);
				}
				inode_number++;
			}
		}
	}



	//-----------------------------------------------------------------------


	//write the directory entry
	//-----------------------------------------------------------------------
	char directory_buffer[max_buffer_size];
	inode_number = 1;
	inode_bound = 0;
	int directory_fd = creat("directory.csv", 0666);
	if(directory_fd == -1)
	{
		perror("Error creating output file directory.csv");
		exit(1);
	}
	__u8 *inode_bitmap_block3 = malloc(sizeof(__u8)*block_size);
	struct ext2_dir_entry directory;

	for(i = 0; i != num_groups; i++)
	{
		inode_bound += superblock.s_inodes_per_group;
		if(i == num_groups - 1)
			inode_bound = superblock.s_inodes_count;

		ret = pread(fd, inode_bitmap_block3, block_size, block_size * group_desc[i].bg_inode_bitmap);
		for(j = 0; j != block_size; j++)
		{
			if(inode_number >= inode_bound)
				break;
			for(k = 0; k != 8; k++)
			{
				if((inode_bitmap_block3[j] & (1 << k)) != 0)				//(group_desc[i].bg_inode_table + inode_number*sizeof(struct ext2_inode))
				{
					ret = pread(fd, &inode, sizeof(struct ext2_inode), group_desc[i].bg_inode_table * block_size + sizeof(struct ext2_inode)*(j*8+k));
					if(inode.i_mode & 0x4000)
					{
						struct block *head;

						struct ext2_inode inode_check;
						if(inode_number > 0)
						{
							int m = inode_number - 1;
							int blk_grp = m / superblock.s_inodes_per_group;
							int blk_grp_off = m % superblock.s_inodes_per_group;
							__u8 inode_bitmap;
							pread(fd, &inode_bitmap, 1, group_desc[blk_grp].bg_inode_bitmap * block_size + blk_grp_off / 8);
							if(inode_bitmap & (1 << (blk_grp_off % 8)))
							{
								pread(fd, &inode_check, sizeof(struct ext2_inode), group_desc[blk_grp].bg_inode_table*block_size + sizeof(struct ext2_inode) * blk_grp_off);
							}
							struct block *h = NULL;
							struct block *t = NULL;
							int l;
							for(l = 0; l < 12 && inode_check.i_block[l]; l++)
							{

								struct block* ele = malloc(sizeof(struct block));
								ele->next = NULL;
								ele->address = inode_check.i_block[l]*block_size;
								ele->con_block = -1;
								if(t == NULL)
									h = ele;
								else
									t->next = ele;
								t = ele;
							}

							if(inode_check.i_block[12])
								t = indirect_read_inodeblk(t, inode_check.i_block[12], fd, block_size, 1);
							if(inode_check.i_block[13])
								t = indirect_read_inodeblk(t, inode_check.i_block[13], fd, block_size, 2);
							if(inode_check.i_block[14])
								t = indirect_read_inodeblk(t, inode_check.i_block[14], fd, block_size, 3);
							head = h;
						}
						else
							head = NULL;

						struct block *other = head;
						__u32 nnum = 0;

						while(other != NULL)
						{
							int pos = other->address;
							memset(directory.name, 0, 255);
							pread(fd, &directory, 8, pos);
							pread(fd, &(directory.name), directory.name_len, pos + 8);
							if(directory.inode != 0)
							{
								ret = sprintf(directory_buffer, "%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%s\n",
									inode_number, nnum, directory.rec_len, directory.name_len, directory.inode, directory.name);
								write(directory_fd, directory_buffer, ret);
							}
							nnum++;

							while(directory.rec_len + pos < other->address + block_size)
							{
								pos += directory.rec_len;
								memset(directory.name, 0, 255);
								pread(fd, &directory, 8, pos);
								pread(fd, &(directory.name), directory.name_len, pos + 8);
								if(directory.inode != 0)
								{
									ret = sprintf(directory_buffer, "%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%s\n",
										inode_number, nnum, directory.rec_len, directory.name_len, directory.inode, directory.name);
									write(directory_fd, directory_buffer, ret);
								}
							}
							other = other->next;
						}
					}
				}
				inode_number++;
			}
		}
	}

	//----------------------------------------------------------------------------



	//write the indirect block entry
	//-----------------------------------------------------------------------------

	inode_number = 1;
	inode_bound = 0;
	int indirect_fd = creat("indirect.csv", 0666);
	if(indirect_fd == -1)
	{
		perror("Error creating output file indirect.csv");
		exit(1);
	}
	__u8 *inode_bitmap_block4 = malloc(sizeof(__u8)*block_size);

	for(i = 0; i != num_groups; i++)
	{
		inode_bound += superblock.s_inodes_per_group;
		if(i == num_groups - 1)
			inode_bound = superblock.s_inodes_count;

		ret = pread(fd, inode_bitmap_block4, block_size, block_size * group_desc[i].bg_inode_bitmap);
		for(j = 0; j != block_size; j++)
		{
			if(inode_number >= inode_bound)
				break;
			for(k = 0; k != 8; k++)
			{
				if((inode_bitmap_block4[j] & (1 << k)) != 0)				//(group_desc[i].bg_inode_table + inode_number*sizeof(struct ext2_inode))
				{
					ret = pread(fd, &inode, sizeof(struct ext2_inode), group_desc[i].bg_inode_table * block_size + sizeof(struct ext2_inode)*(j*8+k));
					__u32 blocks = inode.i_blocks / (2 << superblock.s_log_block_size);
					if(blocks <= 10)
						break;
					else
					{
						if(inode.i_block[12])
							indirect_write_inodeblk(inode.i_block[12], fd, block_size, 1, indirect_fd);
						if(inode.i_block[13])
							indirect_write_inodeblk(inode.i_block[13], fd, block_size, 2, indirect_fd);
						if(inode.i_block[14])
							indirect_write_inodeblk(inode.i_block[14], fd, block_size, 3, indirect_fd);
					}
				}
				inode_number++;
			}
		}
	}

	//------------------------------------------------------------------------------

	return 0;
}
