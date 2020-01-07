// Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <io.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <share.h>
#include "RM_Manager.h"
#include<iostream>
#include<bitset>
#include<assert.h>
using namespace std;

void outBin(int x);
void TestCreatFile();
void TestOpenFile();
void TestOperateRec();
//void TestScan();
void printlnMsg(RC tmp);
int main(void)
{

	TestCreatFile();
	TestOpenFile();
	TestOperateRec();
	//TestScan();
	return 0;
}

void TestCreatFile() {
	char fileName[] = "123.txt";
	if (_access(fileName, 0) != -1) {
		remove(fileName);
	}
	int recordSize = 12;
	RC tmp = RM_CreateFile(fileName, 12);
	assert(tmp==SUCCESS);
	int fd = _open(fileName, O_RDONLY | _O_BINARY);
	Page page0, page1;

	_read(fd, &page0, sizeof(Page));
	_read(fd, &page1, sizeof(Page));
	_close(fd);


	char* bitmap = page0.pData + (int)PF_FILESUBHDR_SIZE;
	PF_FileSubHeader* fileSubHeader = (PF_FileSubHeader*)page0.pData;
	assert(fileSubHeader->nAllocatedPages == 2);
	assert(fileSubHeader->pageCount == 1);
	assert((int)bitmap[0] == 0x3);
	assert(page0.pageNum == 0);

	char* recBitmap = page1.pData + RM_FILESUBHDR_SIZE;
	RM_FileSubHeader* rmFileSubHeader = (RM_FileSubHeader*)page1.pData;
	assert(page1.pageNum == 1);
	/*printf("recordSize: %d nRecords: %d firstRecordOffset: %d  recordsPerPage: %d \n",
		rmFileSubHeader->recordSize, rmFileSubHeader->nRecords,
		rmFileSubHeader->firstRecordOffset, rmFileSubHeader->recordsPerPage);*/
	assert(rmFileSubHeader->recordSize == 12);
	assert(rmFileSubHeader->nRecords == 0);
	assert(rmFileSubHeader->firstRecordOffset == (int)ceil((float)PF_PAGE_SIZE
		/ (float)(8 * recordSize + 1)));
	assert(rmFileSubHeader->recordsPerPage == (PF_PAGE_SIZE - rmFileSubHeader->firstRecordOffset)
		/ recordSize);
	assert((int)recBitmap[0] == 0x3);
	std::cout << "TestCreatFile:" << "SUCCESS" << std::endl;
	
}


void TestOpenFile() {
	RM_FileHandle rmFileHandle;
	char fileName[] = "123.txt";
	if (_access(fileName, 0) != -1) {
		remove(fileName);
	}
	RM_CreateFile(fileName, 12);
	RC tmp = RM_OpenFile(fileName, &rmFileHandle);
	assert(tmp==SUCCESS);
	

	PF_FileSubHeader* fileSubHeader = (PF_FileSubHeader*)rmFileHandle.pFileHandler->pFileSubHeader;
	Frame* pHdrFrame = rmFileHandle.pFileHandler->pHdrFrame;
	char* bitmap = rmFileHandle.pFileHandler->pBitmap;
	//std::cout << "fileSubHeader->nAllocatedPage: " << fileSubHeader->nAllocatedPages << std::endl;
	assert(fileSubHeader->nAllocatedPages == 2);
	assert(fileSubHeader->pageCount == 1);
	assert((int)bitmap[0] == 0x3);
	assert(rmFileHandle.pFileHandler->pHdrPage->pageNum == 0);
	assert(pHdrFrame->bDirty == false && pHdrFrame->pinCount == 1);
	

	RM_FileSubHeader* rmFileSubHeader = rmFileHandle.pRecordFileSubHeader;
	Frame* recFrame = rmFileHandle.pRecFrame;
	char* recBitmap = rmFileHandle.pRecordBitmap;
	int recordSize = rmFileSubHeader->recordSize;
	assert(rmFileHandle.bOpen);
	/*printf("recordSize: %d nRecords: %d firstRecordOffset: %d  recordsPerPage: %d \n",
		rmFileSubHeader->recordSize, rmFileSubHeader->nRecords,
		rmFileSubHeader->firstRecordOffset, rmFileSubHeader->recordsPerPage);*/
	assert(rmFileSubHeader->recordSize == 12);
	assert(rmFileSubHeader->nRecords == 0);
	assert(rmFileSubHeader->firstRecordOffset == (int)ceil((float)PF_PAGE_SIZE
		/ (float)(8 * recordSize + 1)));
	assert(rmFileSubHeader->recordsPerPage == (PF_PAGE_SIZE - rmFileSubHeader->firstRecordOffset)
		/ recordSize);
	assert((int)recBitmap[0] == 0x3);
	assert(rmFileHandle.pRecPage->pageNum == 1);
	assert(recFrame->bDirty == false && recFrame->pinCount == 1);
	
	RM_CloseFile(&rmFileHandle);
	std::cout << "TestOpenFile:" << "SUCCESS" << std::endl;

}

void TestOperateRec() {
	char pData[12];
	*((int*)pData) = 5;
	*(((int*)pData)+1) = 2;
	*(((int*)pData) + 2) = 1;

	RID rid;
	RM_FileHandle rmFileHandle;
	char fileName[] = "123.txt";
	if (_access(fileName, 0) != -1) {
		remove(fileName); 
	}
	RM_CreateFile(fileName, 12);
	RC tmp = RM_OpenFile(fileName, &rmFileHandle);
	assert(tmp==SUCCESS);

	tmp = InsertRec(&rmFileHandle, pData, &rid);
	assert(tmp==SUCCESS);

	RM_FileSubHeader* rmFileSubHeader = rmFileHandle.pRecordFileSubHeader;
	Frame* recFrame = rmFileHandle.pRecFrame;
	char* recBitmap = rmFileHandle.pRecordBitmap;
	int recordSize = rmFileSubHeader->recordSize;
	

	assert(rid.bValid == true);
	assert(rid.pageNum == 2);
	assert(rid.slotNum == 0);
	assert(rmFileSubHeader->nRecords == 1);
	assert((int)recBitmap[0] == 0x3);


	tmp = InsertRec(&rmFileHandle, pData, &rid);
	assert(rid.slotNum == 1);
	assert(rmFileSubHeader->nRecords == 2);
	RM_CloseFile(&rmFileHandle);

	tmp = RM_OpenFile(fileName, &rmFileHandle);
	assert(tmp==SUCCESS);

	tmp = InsertRec(&rmFileHandle, pData, &rid);
	assert(tmp==SUCCESS);

	assert(rid.bValid == true);
	assert(rid.pageNum == 2);
	assert(rid.slotNum == 2);
	assert(rmFileSubHeader->nRecords == 3);
	std::cout << "TestInsertRec:" << "SUCCESS" << std::endl;

	RM_Record record;
	tmp = GetRec(&rmFileHandle, &rid, &record);
	assert(tmp == SUCCESS);
	assert(equalStr(record.pData, pData, rmFileHandle.rmPageHandle->recordSize));
	std::cout << "TestGetRec:" << "SUCCESS" << std::endl;

	char pData0[12];
	*((int*)pData0) = 4;
	*(((int*)pData0) + 1) = 3;
	*(((int*)pData0) + 2) = 2;
	record.bValid = true;
	record.pData = pData0;
	record.rid = rid;
	tmp = UpdateRec(&rmFileHandle, &record);
	assert(tmp == SUCCESS);
	GetRec(&rmFileHandle, &rid, &record);
	assert(equalStr(record.pData, pData0, rmFileHandle.rmPageHandle->recordSize));
	std::cout << "TestUpdateRec:" << "SUCCESS" << std::endl;

	rid.pageNum = 2;
	rid.slotNum = 0;
	tmp = GetRec(&rmFileHandle, &rid, &record);
	assert(tmp == SUCCESS);
	assert(equalStr(record.pData, pData, rmFileHandle.rmPageHandle->recordSize));

	rid.pageNum = 2;
	rid.slotNum = 1;
	tmp = GetRec(&rmFileHandle, &rid, &record);
	assert(tmp == SUCCESS);
	assert(equalStr(record.pData, pData, rmFileHandle.rmPageHandle->recordSize));




	rid.pageNum = 2;
	rid.slotNum = 2;

	tmp = DeleteRec(&rmFileHandle, &(rid));
	assert(tmp == SUCCESS);
	assert(rmFileHandle.pRecordFileSubHeader->nRecords == 2);
	std::cout << "TestDeleteRec:" << "SUCCESS" << std::endl;


	tmp = GetRec(&rmFileHandle, &rid, &record);
	assert(tmp == RM_INVALIDRID);

	RM_CloseFile(&rmFileHandle);
	tmp = RM_OpenFile(fileName, &rmFileHandle);
	assert(tmp == SUCCESS);
	rid.pageNum = 2;
	rid.slotNum = 0;
	tmp = GetRec(&rmFileHandle, &rid, &record);
	assert(tmp == SUCCESS);
	assert(equalStr(record.pData, pData, rmFileHandle.rmPageHandle->recordSize));
	assert(rmFileHandle.pRecordFileSubHeader->nRecords == 2);
	assert(rmFileHandle.pRecordFileSubHeader->recordSize == 12);
	assert((int)rmFileHandle.rmPageHandle->bitmap[0] == 0x3);

	RM_FileScan fileScan;
	Con con;

	OpenScan(&fileScan, &rmFileHandle, 0, &con);
	GetRec(&rmFileHandle, &rid, &record);
	int i = 0;
	while (true)
	{
		tmp = GetNextRec(&fileScan, &record);
		if (tmp == RM_EOF)
			break;
		if (i >= 2) {
			assert(false);//多于两个记录
			break;
		}
		assert(tmp == SUCCESS);
		assert(record.bValid == true);
		assert(record.rid.bValid == true);
		assert(record.rid.pageNum == 2);
		assert(record.rid.slotNum == i);
		i++;
		assert(equalStr(record.pData, pData, rmFileHandle.rmPageHandle->recordSize));
	}
	std::cout << "TestScan:" << "SUCCESS" << std::endl;
	RM_CloseFile(&rmFileHandle);
}









void outBin(int x) {
	std::cout << bitset<sizeof(int) * 8>(x) << std::endl;
}
void outHex(int x) {
	std::cout << hex << x << std::endl;
}


void printlnMsg(RC tmp) {
	switch (tmp)
	{
	case SUCCESS:	            printf("  SUCCESS:             	\n "); break;
	case SQL_SYNTAX:			printf("  SQL_SYNTAX:			\n "); break;
	case PF_EXIST:				printf("  PF_EXIST:				\n "); break;
	case PF_FILEERR:			printf("  PF_FILEERR:			\n "); break;
	case PF_INVALIDNAME:		printf("  PF_INVALIDNAME:		\n "); break;
	case PF_WINDOWS:			printf("  PF_WINDOWS:			\n "); break;
	case PF_FHCLOSED:			printf("  PF_FHCLOSED:			\n "); break;
	case PF_FHOPEN:				printf("  PF_FHOPEN:			\n "); break;
	case PF_PHCLOSED:			printf("  PF_PHCLOSED:			\n "); break;
	case PF_PHOPEN:				printf("  PF_PHOPEN:			\n "); break;
	case PF_NOBUF:				printf("  PF_NOBUF:				\n "); break;
	case PF_EOF:				printf("  PF_EOF:				\n "); break;
	case PF_INVALIDPAGENUM:		printf("  PF_INVALIDPAGENUM:	\n "); break;
	case PF_NOTINBUF:			printf("  PF_NOTINBUF:			\n "); break;
	case PF_PAGEPINNED:			printf("  PF_PAGEPINNED:		\n "); break;
	case RM_FHCLOSED:			printf("  RM_FHCLOSED:			\n "); break;
	case RM_FHOPENNED:			printf("  RM_FHOPENNED:			\n "); break;
	case RM_INVALIDRECSIZE:		printf("  RM_INVALIDRECSIZE:	\n "); break;
	case RM_INVALIDRID:			printf("  RM_INVALIDRID:		\n "); break;
	case RM_FSCLOSED:			printf("  RM_FSCLOSED:			\n "); break;
	case RM_NOMORERECINMEM:		printf("  RM_NOMORERECINMEM:	\n "); break;
	case RM_FSOPEN:				printf("  RM_FSOPEN:			\n "); break;
	case RM_EOF:				printf("  RM_EOF:				\n "); break;
	case IX_IHOPENNED:			printf("  IX_IHOPENNED:			\n "); break;
	case IX_IHCLOSED:			printf("  IX_IHCLOSED:			\n "); break;
	case IX_INVALIDKEY:			printf("  IX_INVALIDKEY:		\n "); break;
	case IX_NOMEM:				printf("  IX_NOMEM:				\n "); break;
	case RM_NOMOREIDXINMEM:		printf("  RM_NOMOREIDXINMEM:	\n "); break;
	case IX_EOF:				printf("  IX_EOF:				\n "); break;
	case IX_SCANCLOSED:			printf("  IX_SCANCLOSED:		\n "); break;
	case IX_ISCLOSED:			printf("  IX_ISCLOSED:			\n "); break;
	case IX_NOMOREIDXINMEM:		printf("  IX_NOMOREIDXINMEM:	\n "); break;
	case IX_SCANOPENNED:		printf("  IX_SCANOPENNED:		\n "); break;
	case FAIL:					printf("  FAIL:					\n "); break;
	case DB_EXIST:				printf("  DB_EXIST:				\n "); break;
	case DB_NOT_EXIST:			printf("  DB_NOT_EXIST:			\n "); break;
	case NO_DB_OPENED:			printf("  NO_DB_OPENED:			\n "); break;
	case TABLE_NOT_EXIST:		printf("  TABLE_NOT_EXIST:		\n "); break;
	case TABLE_EXIST:			printf("  TABLE_EXIST:			\n "); break;
	case TABLE_NAME_ILLEGAL:	printf("  TABLE_NAME_ILLEGAL:	\n "); break;
	case FLIED_NOT_EXIST:		printf("  FLIED_NOT_EXIST:		\n "); break;
	case FIELD_NAME_ILLEGAL:	printf("  FIELD_NAME_ILLEGAL:	\n "); break;
	case FIELD_MISSING:			printf("  FIELD_MISSING:		\n "); break;
	case FIELD_REDUNDAN:		printf("	FIELD_REDUNDAN:		\n "); break;
	case FIELD_TYPE_MISMATCH:	printf("	FIELD_TYPE_MISMATCH:\n "); break;
	case RECORD_NOT_EXIST:		printf("	RECORD_NOT_EXIST:	\n "); break;
	case INDEX_NAME_REPEAT:		printf("  INDEX_NAME_REPEAT:	\n "); break;
	case INDEX_EXIST:			printf("	INDEX_EXIST:		\n "); break;
	case INDEX_NOT_EXIST:		printf("  INDEX_NOT_EXIST:		\n "); break;
	case INDEX_CREATE_FAILED:	printf("  INDEX_CREATE_FAILED:	\n "); break;
	case INDEX_DELETE_FAILED:	printf("  INDEX_DELETE_FAILED:	\n "); break;
	case INCORRECT_QUERY_RESULT:printf("	INCORRECT_QUERY_RESU\n "); break;
	case ABNORMAL_EXIT:			printf("	ABNORMAL_EXIT:		\n "); break;
	case TABLE_CREATE_REPEAT:  	printf("	TABLE_CREATE_REPEAT:\n "); break;
	case TABLE_CREATE_FAILED:  	printf("	TABLE_CREATE_FAILED:\n "); break;
	case TABLE_COLUMN_ERROR:   	printf("	TABLE_COLUMN_ERROR:	\n "); break;
	case TABLE_ROW_ERROR:		printf("  TABLE_ROW_ERROR:		\n "); break;
	case TABLE_DELETE_FAILED:	printf("	TABLE_DELETE_FAILED:\n "); break;
	case DATABASE_FAILED:		printf("  DATABASE_FAILED:		\n "); break;
	default: break;
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
