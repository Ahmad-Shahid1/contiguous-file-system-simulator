#include <stdio.h> // including this library to handle input and output statements such as printf, fgets,fflush and sscanf
#include <stdlib.h> // using it to sort directory entries alphabetically
#include <string.h> // including this for string and memory hanlding like strcmp,strncpy, strcspn etcetera

#define SIZEOFBLOCK 1024 // specifies the block size where every block is 1024 bytes or equal to 1 kilobyte
#define TOTALNUMOFBLOCKS 200 // 200 blocks here means that we basically will have 200 kilobytes in total as specifed in the assignment slides
#define NUMOFMAXFILES  52 // this limits the maximum number of files that can be in the directory
#define FILENAMELENGTH 70 // specifies the length of a file name
#define COMPACTIONLIMIT 160 // this is 80% of the total number of blocks which i have defined earlier or on line 6, when this limit is crossed or is reached, disk starts compacting

#define USEDBLOCK 1 // shows that a block is holding data or is being used
#define FREEBLOCK 0 // means block is open, in the dump this will be represented by O
#define DELETEDBLOCK 2 // means that the block was freed by del

typedef struct {
	char fileName[FILENAMELENGTH]; // line for the filename
	int sizeInBytes; // size in bytes
	int firstBlock; // this is for the first block of the file
	int numOfBlocksForFile; // this is for how many blocks the file occupies
	int slotInUse; // 1 in the slot means it is occupied or is being used and 0 means its empty
}
directoryEntry; // for one row in the theDirectory[] table

int mapForBlocks[TOTALNUMOFBLOCKS]; // this helps in keeping track of the state of each and every block on the disk we are simulating
directoryEntry theDirectory[NUMOFMAXFILES]; // this for the directory table that is in-memory
int numberOfLiveFiles =0; // this is for number of files that are live and that currently exist
int nextFreeBlock=0; // for the next file that will be written


void diskFormat(void){ // this function resets everything to an empty disk
	int i; // loop counter 
	for (i=0; i < TOTALNUMOFBLOCKS; i++) // loop through all 200 blocks one by one
		mapForBlocks[i]= FREEBLOCK; // every block is initially free
	for (i=0; i< NUMOFMAXFILES;i++)
		theDirectory[i].slotInUse=0; // marks each slot as empty as no files exist at this point
	numberOfLiveFiles=0; // this shows that no files are on disk 
	nextFreeBlock=0; // the process of allocation starts from the first block
}
static int blocksForBytes(int bytes){ // blockForBytes calculates how many blocks are needed to accomadate or hold a certain number of bytes
	return (bytes+SIZEOFBLOCK-1)/SIZEOFBLOCK; // I am using ceiling division here so that for example a 1025 bytes doesnt occupy 1 block but occupies or gets two blocks
	// the excess space is internal fragmentation, this happens when the memory assigned is larger than what is actually needed, this is a concept i learned about while reading chapter 3
}
static int fileFinding(const char*fileName){ // this searches for the directory for filename, returns the index if found or returns -1 when if its not found
	int i; // loop counter
	for(i=0; i <NUMOFMAXFILES;i++) // loops through all 52 of the slots
		if (theDirectory[i].slotInUse && strcmp(theDirectory[i].fileName,fileName)==0) //checks if the slot matches and the name matches
			return i; // returns the index if found
	return -1; // returns -1 if nothing is found
}
void compaction(void){ // this function forces all files that are live towards block zero to start closing the gaps that were left by files that were deleted
	int i; // loop counter for iterating through slots and blocks
	int x; // this second loop counter is for the inner loops
	int writeBlock=0; // this keeps track of which block is next for write process, this begins at the beginning of the disk
	int positionOfDiskBefore=nextFreeBlock; // this helps in remembering where the disk was before the process of compaction. this helps in calculating how much space was freed exactly
	int completed[NUMOFMAXFILES]; // as the name suggests, this keeps track of which files have passed this
	for (i=0;i < NUMOFMAXFILES;i++) // loops through all the slots and assigns zero
		completed[i]=0; // this for a file means that the file has been moved as of yet
	printf("The compaction process has started and the pointer for allocation is at block %d (%d%% of disk).\n", nextFreeBlock,(nextFreeBlock *100)/TOTALNUMOFBLOCKS); // this print statement shows which block started or triggered compaction and shows a percentage of how much disk is full at the point
	int filesMoved=0; // this counter keeps track of how many files have been moved during the process of compaction
	while (filesMoved <numberOfLiveFiles){ // this while helps us in finding the live file that hasn't been moved yet and has the smallest firstBlock
		int fileToBeMoved=-1; // index of the file with the smallest start block, -1 here means nothing has been found as mentioned earlier
		int earliestBlock=TOTALNUMOFBLOCKS+1; // setting this high so that a real block number will be smaller
		for (i=0; i<NUMOFMAXFILES; i++){ // loops through  all directory slots
			if (theDirectory[i].slotInUse && !completed[i] && theDirectory[i].firstBlock <earliestBlock){ // checks if the the slot has been occupied, has not been moved and is closer to the start of the disk than which is current
				earliestBlock=theDirectory[i].firstBlock; // updates it to the smallest firstblock found yet
				fileToBeMoved=i; // remembers which file it belongs to
			}
		}
		if (fileToBeMoved==-1) // all files have been moved so it breaks
			break;
		directoryEntry*currentFile= &theDirectory[fileToBeMoved];// we get the memory address using & so we can modify it directly
		for (x=(*currentFile).firstBlock; x<(*currentFile).firstBlock +(*currentFile).numOfBlocksForFile; x++) // for loop
			mapForBlocks[x]=FREEBLOCK; // clears the old block range
		(*currentFile).firstBlock=writeBlock; // updates the directory entry
		for (x=writeBlock;x <writeBlock+(*currentFile).numOfBlocksForFile;x++) // for loop
			mapForBlocks[x]=USEDBLOCK; // marks the new range as used
		writeBlock +=(*currentFile).numOfBlocksForFile; // advances pointer for write process past this file
		completed[fileToBeMoved]=1;
		filesMoved++;
	}
	for (i=writeBlock;i<positionOfDiskBefore;i++)
		mapForBlocks[i]=FREEBLOCK; // clears the tail thats leftover whih used to hold deleted blocks
	nextFreeBlock=writeBlock; // resets the pointer for allocation to right after all the live files
	printf("Process of compaction done, reclaimed %d blocks. next free block is: %d (the %d remaining).\n ",positionOfDiskBefore-nextFreeBlock, nextFreeBlock,TOTALNUMOFBLOCKS-nextFreeBlock);
}
void saveFile(const char*fileName, int numberOfBytes){ // this function allocates contiguous blocks for a new file and then records it in the directory, numberOfBytes stores the file size user types in
	int blocksRequired=blocksForBytes(numberOfBytes); // for how many blocks this file needs
	int i;
	int slot;
	if (fileFinding(fileName)!=-1){
		printf("Error!!!: file %s already exits \n ",fileName);
		return;
	}
	if (numberOfBytes <=0){
		printf("Error!!: file size must be something thats positive or should be a positive number of bytes \n");
		return;
	}
	if (nextFreeBlock>=COMPACTIONLIMIT)
		compaction();
	if (nextFreeBlock+blocksRequired>TOTALNUMOFBLOCKS){ // in case not enough space is present even after the process of compaction
		printf("File not saved; file system is full \n");
		return;
	}
	slot=-1; // to find an empty slot in the directory
	for(i=0;i<NUMOFMAXFILES;i++){
		if(!theDirectory[i].slotInUse){
			slot=i;
			break;
		}
	}
	if (slot==-1){
		printf("error!, the directory is full, max %d files \n",NUMOFMAXFILES);
		return;
	}
	theDirectory[slot].slotInUse =1; // marks the slot as occupied or used so the rest of the code knows the presence of the file here
	strncpy(theDirectory[slot].fileName,fileName,69); // copies the file name into the directory entry
	theDirectory[slot].fileName[69]='\0'; // this is null terminator just in case if the filename exceeds the limit
	theDirectory[slot].sizeInBytes=numberOfBytes;// stores how many bytes the user asked for
	theDirectory[slot].firstBlock=nextFreeBlock;// records from which block this file starts at
	theDirectory[slot].numOfBlocksForFile=blocksRequired;// records how many blocks this file actually occupies

	for (i=nextFreeBlock;i<nextFreeBlock+blocksRequired;i++)
		mapForBlocks[i]=USEDBLOCK; // this marks all the blocks that were allocated as used
	nextFreeBlock=nextFreeBlock+blocksRequired; // this changes the pointer position past this file
	numberOfLiveFiles++;
	printf("Writing %s to disk in  the %d blocks\n",fileName,blocksRequired);
}
void fileRead(const char*fileName){ // this simulates reading a file, then reports actual bytes and the on disk bytes too
	int fileIndex=fileFinding(fileName);
	if(fileIndex==-1){
		printf("Error! the file %s was not found \n",fileName);
		return;
	}
	directoryEntry*currentFile= &theDirectory[fileIndex];
	int i; // the loop counter
	int bytesOnDisk=0; // this will accumlate the total space that is consumed on disk
	for (i=0;i <currentFile->numOfBlocksForFile;i++) // adds one block's worth of bytes for each
		bytesOnDisk=bytesOnDisk+SIZEOFBLOCK;
	int spaceWasted=bytesOnDisk-currentFile->sizeInBytes; // internal fragmentation in the last block being practiced here basically
	printf("reading the file %s: %d bytes %d ,the bytes on disk\n",fileName,currentFile->sizeInBytes,bytesOnDisk);
	if (spaceWasted>0)
		printf("This is for Internal fragmentation: %d bytes were wasted in the last block \n",spaceWasted);
	printf("The blocks that were read:%d | first block or start block %d \n",currentFile->numOfBlocksForFile,currentFile->firstBlock);
}
void deleteFile (const char*fileName){ // this function removes a file from the directory and marks its contents or blocks as deleted
	int fileIndex=fileFinding(fileName);
	if (fileIndex==-1){
		printf("error! deleting file:  %s\n",fileName); // two spaces here to match your sample output Dr Steinwand
		return;

	}
	directoryEntry*currentFile=&theDirectory[fileIndex];
	int i;
	for (i=currentFile->firstBlock;i<currentFile->firstBlock+currentFile->numOfBlocksForFile;i++)
		mapForBlocks[i]=DELETEDBLOCK;// this marks the blocks as deleted, are shown as D in the dump
	currentFile->slotInUse=0; // frees up the slot for use in the future
	numberOfLiveFiles=numberOfLiveFiles-1;
}
static int sortingAlphabetically(const void *left, const void *right){ // acts a comparator function for qsort, learned about the concept of comparator in computer science iii, this helps in sorting the diretory alphabetically
	const directoryEntry*leftEntry=(const directoryEntry *)left; // this casts left pointer to a directory entry
	const directoryEntry*rightEntry=(const directoryEntry*)right; // this like the line abpve casts the right pointer to an entry in the directory
	if (!leftEntry->slotInUse&& !rightEntry->slotInUse)
		return 0;
	if (!leftEntry->slotInUse)
		return 1; // empty slots sink to the bottom
	if(!rightEntry->slotInUse)
		return -1;
	return strcmp(leftEntry-> fileName,rightEntry->fileName); // alphabetical comparison
}
void listFiles(void){ // lists all the files alphabetically with their size and location on disk
	if(numberOfLiveFiles==0){
		printf("Directory listing (empty)\n:");
		return;
	}
	directoryEntry sorting[NUMOFMAXFILES];
	int i; // loop counter
	for(i=0;i<NUMOFMAXFILES;i++){
		sorting[i]=theDirectory[i]; // copies each entry in the directory one by one into the sorted array
	}
		qsort(sorting,NUMOFMAXFILES,sizeof(directoryEntry),sortingAlphabetically);
		printf("Lisitng the directory:\n");
		for (i=0;i<NUMOFMAXFILES && sorting[i].slotInUse;i++){ // stopping at the very first empty slot
			printf("%s: %d bytes in %d blocks, starting at block: %d\n",sorting[i].fileName,sorting[i].sizeInBytes,sorting[i].numOfBlocksForFile,sorting[i].firstBlock);
		}
}
void printingMapForBlocks(void){ // this function prints a row map of every blocks state, used your output as a reference where U meant used, D meant deleted and O meant open or free
	int i;
	printf("The block-map dump:\n");
	for(i=0;i<TOTALNUMOFBLOCKS;i++){
		if (i%10==0)
			printf("%5d:  ",i);// row labels are aligned to the right in 5 characters, this is my attempt to match your output format
		char symbolForBlock;
		switch(mapForBlocks[i]){ // checking the state of the current block
		    case USEDBLOCK: symbolForBlock='U'; // block is holding data
		    	break;
		    case DELETEDBLOCK: symbolForBlock='D'; // block was deleted
		    	break;
		    default: symbolForBlock='O'; // block is free
		    	break;
		}
		if (i%10==9)
			printf("%c\n ",symbolForBlock); // newline at the end of the row
		else
			printf("%c ",symbolForBlock); // space between blocks in a row
	}
}
void printingDetailedMapForBlocks(void){ // this is the function for dump-all, 
	int i; // this loop counter helps in iterating through all 200 blocks
	int x; // this second loop counter is used when scanning directory slots to find map of each file block
	printf("Detailed Block Map Dump:\n");
	for(i=0;i<TOTALNUMOFBLOCKS;i++){
		if (i%10==0)
			printf("%5d:  ",i);// row labels are aligned to the right in 5 characters, this is my attempt to match your output format
		char symbolForBlock;
		switch(mapForBlocks[i]){ // checking the state of the current block
		    case USEDBLOCK: symbolForBlock='U'; // block is holding data
		    	break;
		    case DELETEDBLOCK: symbolForBlock='D'; // block was deleted
		    	break;
		    default: symbolForBlock='O'; // block is free
		    	break;
		}
		if (i%10==9) 
			printf("%c\n",symbolForBlock); // newline at the end of the row
		else
			printf("%c ",symbolForBlock); // space between blocks in a row
	}
	printf("\n ownership for blocks per block\n"); // every block with its own line
	for (i=0;i <TOTALNUMOFBLOCKS;i++){
		char symbolForBlock;
		switch (mapForBlocks[i]){ // checks the state of block being handled currently
		    case USEDBLOCK:
		    	symbolForBlock='U'; // block is holding data
		    	break;
		    case DELETEDBLOCK:
		    	symbolForBlock='D'; // block was deleted, hasn't been reclaimed yet either
		    	break;
		    default:
		    	symbolForBlock='O'; // block is available
		    	break;
		}
		printf("block %3d: %c  ",i,symbolForBlock); // printing the block number and the symbol first
		if (mapForBlocks[i]==FREEBLOCK){
			printf("block is free\n");
		}else if(mapForBlocks[i]==DELETEDBLOCK){
			printf("block was deleted\n");
		}else{
			int blockFound=0;
			for (x=0;x<NUMOFMAXFILES;x++){ // scanning directory to find which file occupies the block
				if (theDirectory[x].slotInUse && i >=theDirectory[x].firstBlock && i <theDirectory[x].firstBlock+theDirectory[x].numOfBlocksForFile){
					int blockPosition=i- theDirectory[x].firstBlock + 1;
					printf("%s (block %d of %d)\n",theDirectory[x].fileName,blockPosition,theDirectory[x].numOfBlocksForFile);
					blockFound=1;
					break;
				}
			}
			if (!blockFound)
				printf("used but no file in the directory claims this block \n"); // this shouldnt happen
		
		}
	}
}
void help(void){
	printf(" Contiguous File System Simulation -Commands:\n");
	printf(" dir                    List the filesystem contents and attributes\n");
	printf(" store filename bytes   store a file of given size \n");
	printf(" access filename        Access a file and reporting the cost in block\n");
	printf(" del filename           delete a file and mark its block as deleted\n");
	printf(" dump                   show the whole block map");
	printf(" dump-all               the whole block map plus the per block file ownership \n ");
	printf(" help                   prints this command list");
	printf(" exit                   exiting the filesystem Simulation\n");

}
int main(void){
	char inputFromUser[256]; // this acts as a buffer for user input
	diskFormat(); // this allows us to set up an empty disk
	printf("---Contiguous filesystem simulator---\n");
	printf("Disk: %d blocks x %d bytes=%d Kilobyte in total\n",TOTALNUMOFBLOCKS,SIZEOFBLOCK,(TOTALNUMOFBLOCKS * SIZEOFBLOCK)/1024);
	printf("the compaction limit or threshold is: 80%% = block %d\n",COMPACTIONLIMIT);
	printf("if need help, type in 'help' for an extensive list of commands. \n\n");
	while (1){
		printf("enter command> ");
		if (!fgets(inputFromUser,sizeof(inputFromUser),stdin)) // ensures that we exit cleanly at end of the file
			break;
		inputFromUser[strcspn(inputFromUser,"\n")]='\0';// this strips the newline character
		if (inputFromUser[0]=='\0')
			continue; // helps in skipping blank lines
		char *token= strtok(inputFromUser, " "); // splitting the input through spaces, the first token is the command, learned about tokenizing in CS III too with Dr Cami
		if (token==NULL)
			continue; // if nothing was typed, skip
		if (strcmp(token, "dir")==0){
			listFiles();
		}else if(strcmp(token,"store")==0){
			char*fileName=strtok(NULL," "); // the second token is the filename
			char *bytes=strtok(NULL," "); // the third token is the byte count
			if (fileName!=NULL && bytes!=NULL)
				saveFile(fileName,atoi(bytes)); // atoi here converts the byte count to an integer and is part of the stdlib library
			else 
			    printf("Usage:store filename, bytes \n");

		}else if(strcmp(token, "access")==0){
			char*fileName=strtok(NULL," "); //second token here is the file name
			if (fileName!=NULL)
				fileRead(fileName);
			else
				printf("Usage:access filename \n");

		}else if(strcmp(token, "del")==0){
			char*fileName=strtok(NULL, " ");// the second token is the file name
			if(fileName!=NULL)
				deleteFile(fileName);
			else
				printf("Usage: del filename \n");
		}else if(strcmp(token,"dump")==0){
			printingMapForBlocks();
		}else if (strcmp(token,"dump-all")==0){
			printingDetailedMapForBlocks();
		}else if (strcmp(token,"help")==0){
			help();
		}else if(strcmp(token,"exit")==0){
			printf("Goodbye, it was a pleasure\n");
			break; // exits the loop and finally the program ends
		} else{
			printf("the command typed in is unknown: %s, please type 'help' for a list \n",token);
		}
	}
	return 0; // program ends
}





