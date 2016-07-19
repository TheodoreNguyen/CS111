#!/bin/python

#import libraries
import csv
import numpy as np



#define global data types to be filled up from files
total_inodes = 0
total_blocks = 0
block_size = 0
blockpergroup = 0
inodepergroup = 0
firstdatablock = 0

inodeBitmapBlocks = []
blockBitmapBlocks = []
inodeFreeList = []
blockFreeList = []

inodeAllocated = {}
blockAllocated = {}
indirectTable = {}
directoryTable = {}

inodeTableStart = [];



#define data structures
class Inode:
	def __init__(self, INODE_NUMBER, NLINKS, NBLOCKS):
		self.inode_number = INODE_NUMBER
		self.nlinks = NLINKS
		self.nblocks = NBLOCKS
		self.ptrs = []
		self.referenced_by_list = []
		return

class Block:
	def __init__(self, BLOCK_NUMBER):
		self.block_number = BLOCK_NUMBER
		self.referenced_by_list = []
		return

#def indirectBLock(block,iode,indirectblock,entry,num):
	

#define methods to print errors to file		
def unallocated_block(FILE, block_num, inode_num, entry_num):
	FILE.write("UNALLOCATED BLOCK < ")
	FILE.write(str(block_num))
	FILE.write(" > REFERENCED BY INODE < ")
	FILE.write(str(inode_num))
	FILE.write(" > ENTRY < ")
	FILE.write(str(entry_num))
	FILE.write(" >\n")
	
def duplicately_allocated_block(FILE, block_num):
	FILE.write("MULTIPLY REFERENCED BLOCK < ")
	FILE.write(str(block_num))
	FILE.write(" > BY")
	for ref in blockAllocated[block_num].referenced_by_list:
		FILE.write(" INODE < ")
		FILE.write(str(ref[0])) 
		FILE.write(" > ENTRY < ")
		FILE.write(str(ref[2]))
		FILE.write(" >")
	FILE.write("\n")
		
def unallocated_inode(FILE, inode_num, dinode_num, entry_num):
	FILE.write("UNALLOCATED INODE < ")
	FILE.write(str(inode_num))
	FILE.write(" > REFERENCED BY DIRECTORY < ")
	FILE.write(str(dinode_num))
	FILE.write(" > ENTRY < ")
	FILE.write(str(entry_num))
	FILE.write(" >\n")
	
def missing_inode(FILE, inode_num, block_num):
	FILE.write("MISSING INODE < ")
	FILE.write(str(inode_num))
	FILE.write(" > SHOULD BE IN FREE LIST < ")
	FILE.write(str(block_num))
	FILE.write(" >\n")
	
def incorrect_link_count(FILE, inode_num, link_countF, link_countT):
	FILE.write("LINKCOUNT < ")
	FILE.write(str(inode_num))
	FILE.write(" > IS < ")
	FILE.write(str(link_countF))
	FILE.write(" > SHOULD BE < ")
	FILE.write(str(link_countT))
	FILE.write(" >\n")
	
def incorrect_directory_entry(FILE, inode_num, entry_name, incorrect_inode, correct_inode):
	FILE.write("INCORRECT ENTRY IN < ")
	FILE.write(str(inode_num))
	FILE.write(" > NAME < ")
	FILE.write(str(entry_name))
	FILE.write(" > LINK TO < ")
	FILE.write(str(incorrect_inode))
	FILE.write(" > SHOULD BE < ")
	FILE.write(str(correct_inode))
	FILE.write(" >\n")
	
def invalid_block_pointer(FILE, block_num, inode_num, entry_num, indirect=-1000):
	if (indirect == -1000):
		FILE.write("INVALID BLOCK < ")
		FILE.write(str(block_num))
		FILE.write(" > IN INODE < ")
		FILE.write(str(inode_num))
		FILE.write(" > ENTRY < ")
		FILE.write(str(entry_num))
		FILE.write(" >\n")
	else:
		FILE.write("INVALID BLOCK < ")
		FILE.write(str(block_num))
		FILE.write(" > IN INODE < ")
		FILE.write(str(inode_num))
		FILE.write(" > INDIRECT BLOCK < ")
		FILE.write(str(indirect))
		FILE.write(" > ENTRY < ")
		FILE.write(str(entry_num))
		FILE.write(" >\n")
	return

	
#load all the csvs into list variables
superlist = []
file = open('super.csv')
csv_super = csv.reader(file)
for row in csv_super:
	superlist.append(row)

grouplist = []
file = open('group.csv')
csv_group = csv.reader(file)
for row in csv_group:
	grouplist.append(row)
	
bitmaplist = []
file = open('bitmap.csv')
csv_bitmap = csv.reader(file)
for row in csv_bitmap:
	bitmaplist.append(row)
	
directorylist = []
file = open('directory.csv')
csv_directory = csv.reader(file)
for row in csv_directory:
	directorylist.append(row)
	
indirectlist = []
file = open('indirect.csv')
csv_indirect = csv.reader(file)
for row in csv_indirect:
	indirectlist.append(row)

inodelist = []
file = open('inode.csv')
csv_inode = csv.reader(file)
for row in csv_inode:
	inodelist.append(row)

	
#open the output file we will be writing errors to
output = open('lab3b_check.txt','w+')


#parse the loaded csvs, adding their contents to the data structures and data types
#parse super.csv
for superblock in superlist:
	total_inodes=(int(superblock[1]))
	total_blocks=(int(superblock[2]))
	block_size=(int(superblock[3]))
	blockpergroup=(int(superblock[5]))
	inodepergroup=(int(superblock[6]))
	firstdatablock=(int(superblock[8]))
	

#parse group.csv
for group in grouplist:
	inodeBitmapBlocks.append(int(group[4],16))		#all values should be int 16?
	blockBitmapBlocks.append(int(group[5],16))
	inodeTableStart.append(int(group[6],16))
	blockAllocated[int(group[4],16)] = Block(int(group[4],16))	#these two make no sense
	blockAllocated[int(group[5],16)] = Block(int(group[5],16))

	
#parse bitmap.csv
for BlockOrInode in bitmaplist:					#BlockOrInode[0] should be int 16
	if int(BlockOrInode[0],16) in inodeBitmapBlocks:	#blockOrInode[1] should be reg int
		inodeFreeList.append(int(BlockOrInode[1]))
	elif int(BlockOrInode[0],16) in blockBitmapBlocks:
		blockFreeList.append(int(BlockOrInode[1]))
	else:
		print "how did i get here again?"

		
#parse indirect.csv		#block num cont block & bp-val should be int 16; #entry num should be reg int
for row in indirectlist:#block num cont block, entry num = block pointer val
	indirectTable[(int(row[0],16), int(row[1]))] = int(row[2],16);

	
#parse inode.csv											#these 3 are all reg ints
for allocatedInode in inodelist:				#inode num			link count			blocks count
	inodeAllocated[int(allocatedInode[0])] = Inode(int(allocatedInode[0]), int(allocatedInode[5]), int(allocatedInode[10]))
	#the rest are all int 16's
	for blockPointerNum in range(11,11+min(int(allocatedInode[10]),12)):
		blocknumber = int(allocatedInode[blockPointerNum],16)			
		if blocknumber == 0 or blocknumber > total_blocks:
			invalid_block_pointer(output, blocknumber, int(allocatedInode[0]), 0, blockPointerNum - 11)
		else:
			if blocknumber not in blockAllocated:
				blockAllocated[blocknumber] = Block(blocknumber)	#tuple: (inode #, indirect block #, entry #)
			blockAllocated[blocknumber].referenced_by_list.append((int(allocatedInode[0]), 0, blockPointerNum - 11))
	
	#indirect blocks
	#singly-indirect block
	remaining_blocks = int(allocatedInode[10]) - 12;
	if remaining_blocks <= 0 or remaining_blocks > 15:
		continue
	blocknumber = int(allocatedInode[23],16)				
	if blocknumber == 0 or blocknumber > total_blocks:
		invalid_block_pointer(output, blocknumber, int(allocatedInode[0]), 12)	#add invalid block to list of invalid blocks

		
	#doubly-indirect block
	remaining_blocks = remaining_blocks - 1;
	if remaining_blocks <= 0 or remaining_blocks > 15:
		continue
	blocknumber = int(allocatedInode[24],16)
	if blocknumber == 0 or blocknumber > total_blocks:
		invalid_block_pointer(output, blocknumber, int(allocatedInode[0]), 13)

		
	#triply-indirect block
	remaining_blocks = remaining_blocks - 1;
	print(str(remaining_blocks))
	if remaining_blocks <= 0 or remaining_blocks > 15:
		continue
	blocknumber = int(allocatedInode[25],16)
	if blocknumber == 0 or blocknumber > total_blocks:
		invalid_block_pointer(output, blocknumber, int(allocatedInode[0]), 14)



#parse directory.csv		#parentinode, childinode, entrynum ALL reg ints. entryname is string still cuz use to rpint
for obj in directorylist:		#NEED TO CHECK ERROR OUTPUT HERE
	if int(obj[4]) != int(obj[0]) or int(obj[0]) == 2:
		directoryTable[int(obj[4])] = int(obj[0])
	if int(obj[4]) in inodeAllocated:
		inodeAllocated[int(obj[4])].referenced_by_list.append((int(obj[0]),int(obj[1])))
	else:
		unallocated_inode(output, str(obj[4]), str(obj[0]), str(obj[1]))
		
	if int(obj[1]) == 0 and int(obj[4]) != int(obj[0]):
		incorrect_directory_entry(output, str(obj[0]), str(obj[5]), str(obj[4]), str(obj[0]))		#third cond?
	elif int(obj[1]) == 1 and (int(obj[4]) != directoryTable[int(obj[0])] or int(obj[0]) not in directoryTable):
		incorrect_directory_entry( output, str(obj[0]), str(obj[5]), str(obj[4]), directoryTable[int(obj[0])] )

#check for any other errors we havent checked for yet

for inode in inodeAllocated:
	linkcount = len(inodeAllocated[inode].referenced_by_list)
	if linkcount == 0 and int(inode) > 10:
		missing_inode(output, str(inode), inodeBitmapBlocks[inode/inodepergroup])
	elif linkcount != int(inodeAllocated[inode].nlinks):
		incorrect_link_count(output, str(inode), str(inodeAllocated[inode].nlinks), linkcount)

	if inode in inodeFreeList:
		unallocated_inode(str(inode), str(inodeAllocated[inode].referenced_by_list[0][0]), str(inodeAllocated[inode].referenced_by_list[0][1])) 

for block in blockAllocated:
	if len(blockAllocated[block].referenced_by_list) > 1:
		duplicately_allocated_block(output, block)

for x in (val for val in blockFreeList if val in blockAllocated):
	unallocated_block(output, str(x), str(blockAllocated[x].referenced_by_list[0][0]), str(blockAllocated[x].referenced_by_list[0][2]))

	
	

	