/*
 * VirtualMemoryManager implementation
 *
 * Used to facilitate demand paging through providing a means by which page
 * faults can be handled and pages loaded from and stored to disk.
*/

#include <stdlib.h>
#include <machine.h>
#include "virtualmemorymanager.h"
#include "system.h"

VirtualMemoryManager::VirtualMemoryManager()
{
    fileSystem->Create(SWAP_FILENAME, SWAP_SECTOR_SIZE * SWAP_SECTORS);
    swapFile = fileSystem->Open(SWAP_FILENAME);

    swapSectorMap = new BitMap(SWAP_SECTORS);
    physicalMemoryInfo = new FrameInfo[NumPhysPages];
    //swapSpaceInfo = new SwapSectorInfo[SWAP_SECTORS];
    nextVictim = 0;
}

VirtualMemoryManager::~VirtualMemoryManager()
{
    fileSystem->Remove(SWAP_FILENAME);
    delete swapFile;
    delete [] physicalMemoryInfo;
    //delete [] swapSpaceInfo;
}

int VirtualMemoryManager::allocSwapSector()
{
    int location = swapSectorMap->Find() * PageSize; // also marks the bit
    return location;
}
/*
SwapSectorInfo * VirtualMemoryManager::getSwapSectorInfo(int index)
{
    return swapSpaceInfo + index;
    
}
*/
void VirtualMemoryManager::writeToSwap(char *page, int pageSize,
                                       int backStoreLoc)
{
    swapFile->WriteAt(page, pageSize, backStoreLoc);
}

/*
 * Page replacement with  the second chance algorithm
 */
void VirtualMemoryManager::swapPageIn(int virtAddr)
{
   TranslationEntry* currPageEntry;
	/*
        if(nextVictim>= NumPhysPages) {//no more space available
                fprintf(stderr, "Fatal error: No more space available\n");
                exit(1);
                return;
        }
	*/

        FrameInfo * physPageInfo = physicalMemoryInfo + nextVictim;

        //If the physical page is both occupied and the use bit is set, unset the use bit of that page,
	//set the netvictim to be nextvictim+1%NumPhysPages so that if we get to the last page,the next page
	//is set back to the first page, and set the the physPageInfo accordingly
	while(physPageInfo->space != NULL && getPageTableEntry(physPageInfo)->use == true){
	  getPageTableEntry(physPageInfo)->use = false;
	  nextVictim = (nextVictim + 1) % NumPhysPages;
	  physPageInfo = physicalMemoryInfo + nextVictim;
	}

	//If the physical page if free:
	if(physPageInfo->space == NULL){
	  *physPageInfo = FrameInfo();
	  physPageInfo->space = currentThread->space;
	  physPageInfo->pageTableIndex = virtAddr / PageSize;
	  currPageEntry = getPageTableEntry(physPageInfo);
	  currPageEntry->physicalPage = memoryManager->getPage();
	  loadPageToCurrVictim(virtAddr);
	}
	//If the page is not free, but the use bit is not set:
	else{
	  TranslationEntry* oldPageEntry;
	  oldPageEntry = getPageTableEntry(physPageInfo);
	  if(oldPageEntry->dirty == true){
	    char* memLocation = machine->mainMemory+oldPageEntry->physicalPage*PageSize;
	    int diskLocation = physPageInfo->space->locationOnDisk[physPageInfo->pageTableIndex];
	    writeToSwap(memLocation, PageSize, diskLocation);
	    oldPageEntry -> dirty = false;
	  }
       
	 
	  physPageInfo->space = currentThread->space;
	  physPageInfo->pageTableIndex = virtAddr / PageSize;
	  currPageEntry = getPageTableEntry(physPageInfo);
	  currPageEntry->physicalPage = oldPageEntry->physicalPage;
	  loadPageToCurrVictim(virtAddr);
	  oldPageEntry->valid = false; 
	  
	}
	nextVictim = (nextVictim + 1) % NumPhysPages; 
}


/*
 * Cleanup the physical memory allocated to a given address space after its 
 * destructor invokes.
*/
void VirtualMemoryManager::releasePages(AddrSpace* space)
{
    for (int i = 0; i < space->getNumPages(); i++)
    {
        TranslationEntry* currPage = space->getPageTableEntry(i);
    //  int swapSpaceIndex = (currPage->locationOnDisk) / PageSize;
 //     SwapSectorInfo * swapPageInfo = swapSpaceInfo + swapSpaceIndex;
//      swapPageInfo->removePage(currPage);
      //swapPageInfo->pageTableEntry = NULL;

        if (currPage->valid == TRUE)
        {
            //int currPID = currPage->space->getPCB()->getPID();
            int currPID = space->getPCB()->getPID();
            DEBUG('v', "E %d: %d\n", currPID, currPage->virtualPage);
            memoryManager->clearPage(currPage->physicalPage);
            physicalMemoryInfo[currPage->physicalPage].space = NULL; 
        }
        swapSectorMap->Clear((space->locationOnDisk[i]) / PageSize);
    }
}

/*
 * After selecting a slot of physical memory as a victim and taking care of
 * synchronizing the data if needed, we load the faulting page into memory.
*/
void VirtualMemoryManager::loadPageToCurrVictim(int virtAddr)
{
    int pageTableIndex = virtAddr / PageSize;
    TranslationEntry* page = currentThread->space->getPageTableEntry(pageTableIndex);
    char* physMemLoc = machine->mainMemory + page->physicalPage * PageSize;
    int swapSpaceLoc = currentThread->space->locationOnDisk[pageTableIndex];
    swapFile->ReadAt(physMemLoc, PageSize, swapSpaceLoc);

  //  int swapSpaceIndex = swapSpaceLoc / PageSize;
 //   SwapSectorInfo * swapPageInfo = swapSpaceInfo + swapSpaceIndex;
    page->valid = TRUE;
//    swapPageInfo->setValidBit(TRUE);
//    swapPageInfo->setPhysMemPageNum(page->physicalPage);
}

/*
 * Helper function for the second chance page replacement that retrieves the physical page
 * which corresponds to the given physical memory page information that the
 * VirtualMemoryManager maintains.
 * This return page table entry corresponding to a physical page
 */
TranslationEntry* VirtualMemoryManager::getPageTableEntry(FrameInfo * physPageInfo)
{
    TranslationEntry* page = physPageInfo->space->getPageTableEntry(physPageInfo->pageTableIndex);
    return page;
}

void VirtualMemoryManager::copySwapSector(int to, int from)
{
    char sectorBuf[SectorSize];
    swapFile->ReadAt(sectorBuf, SWAP_SECTOR_SIZE, from);
    swapFile->WriteAt(sectorBuf, SWAP_SECTOR_SIZE, to);
}
