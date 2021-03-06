// MemoryFile.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//测试内存映射文件

//
#include <iostream>
#include "MemoryFile.h"
#include<Windows.h>
#include"TimeCost.h"
using namespace std;


#define BAD_POS 0xFFFFFFFF // returned by SetFilePointer and GetFileSize
void savaDataWin(const char* filename, const char* sharename, const vector<MarketDataFile>& vec, int flag)
{
	const size_t write_size = sizeof(MarketDataFile) * vec.size();
	std::cout << "data_size: " << write_size << ", vector_size:  " << vec.size() << std::endl;
	DWORD error_code;
	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);     //存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;	//共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;//|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING; //文件属性
	HANDLE mmHandle = nullptr;
	HANDLE mmfm = nullptr;
	char* mmfm_base_address = nullptr;
	try
	{
		//创建文件
		mmHandle = CreateFile(filename, access_mode, share_mode, NULL, OPEN_ALWAYS, flags, NULL);
		if (mmHandle == INVALID_HANDLE_VALUE)
		{
			error_code = GetLastError();
			std::cout << "创建mmHandle失败:" << error_code << std::endl;
			return;
		}
		else
		{
			DWORD high_size;
			__int64 file_size = GetFileSize(mmHandle, &high_size);
			file_size = file_size | (((__int64)high_size) << 32);
			__int64 totalsize = (flag == MEM_OVERRIDE ? write_size : (file_size + write_size));
			//high_size = (DWORD)(totalsize >> 32);
			std::cout << "FILE_SIZE: " << file_size << ", HIGH_SIZE: " << high_size << std::endl;
			if (file_size == BAD_POS && (error_code = GetLastError()) != 0)
			{
				CloseHandle(mmHandle);
				cout << "error：" << error_code << endl;
				return;
			}
			cout << "create mmf sucessfully" << endl;
			DWORD size_high = 0;
			mmfm = CreateFileMapping(mmHandle,
				NULL,
				PAGE_READWRITE,
				(DWORD)(totalsize >> 32),
				(DWORD)totalsize,
				sharename);
			error_code = GetLastError();
			if (0)
			{
				std::cout << "CreateFileMapping error: " << error_code << std::endl;
				return;
			}
			else
			{
				if (mmfm == nullptr)
				{
					if (mmHandle != INVALID_HANDLE_VALUE)
					{
						CloseHandle(mmHandle);
						return;
					}
				}
				else
				{
					DWORD view_access = FILE_MAP_ALL_ACCESS;
					//获得映射视图
					DWORD highoffet = 0;
					DWORD loweroffet = 0;
					file_size = flag == MEM_OVERRIDE ? 0 : file_size;
					SYSTEM_INFO SysInfo;
					GetSystemInfo(&SysInfo);
					//块分配基本单位
					DWORD dwGran = SysInfo.dwAllocationGranularity;
					DWORD dwBlockBytes = 100 * dwGran;
					PBYTE buffer = (PBYTE)malloc(write_size);
					memset(buffer, 0x00, write_size);
					memcpy(buffer, (PBYTE)vec.data(), write_size);
					size_t bufferend = write_size;
					//文件偏移位置
					__int64 qwFileOffset = file_size;
					while (qwFileOffset < totalsize)
					{//每次写入数据少于4G

						highoffet = (DWORD)(qwFileOffset >> 32);
						//当前4G还剩余量
						__int64 diffoffet = ((__int64)1 << 32) - (DWORD)(qwFileOffset);
						if (diffoffet >= write_size)
						{
							diffoffet = write_size;
						}
						size_t n = (DWORD)(qwFileOffset) / dwBlockBytes;
						DWORD  dwBlockBytesoffset = n * dwBlockBytes;
						DWORD mapsize = (DWORD)diffoffet + (DWORD)(qwFileOffset) -dwBlockBytesoffset;
						mmfm_base_address = (char*)MapViewOfFile(mmfm, view_access, highoffet, dwBlockBytesoffset, mapsize);
						if (mmfm_base_address == NULL) {
							error_code = GetLastError();
							if (error_code != 0) {
								cout << "errorcode " << error_code << endl;
								return;
							}
						}
						else
						{
							//向内存映射视图中写数据
							DWORD pos = (flag == MEM_OVERRIDE ? 0 : (DWORD)(qwFileOffset) -dwBlockBytesoffset);
							CopyMemory((PVOID)(mmfm_base_address + pos), buffer, diffoffet);
							qwFileOffset += diffoffet;
							//卸载映射
							UnmapViewOfFile(mmfm_base_address);
							if (bufferend > diffoffet)
							{
								memmove(buffer, buffer + diffoffet, diffoffet);
								bufferend -= diffoffet;
							}
						}

					}
					if (buffer)
						free(buffer);
				}
				//关闭内存映射文件
				CloseHandle(mmfm);
			}

			//关闭文件
			CloseHandle(mmHandle);
		}
		std::cout << " data wirte success" << std::endl;
	}
	catch (const std::exception& e)
	{
		if (mmHandle != nullptr) CloseHandle(mmHandle);
		if (mmfm != nullptr) CloseHandle(mmfm);
		if (mmfm_base_address != nullptr) UnmapViewOfFile(mmfm_base_address);
		std::cout << "error：" << e.what() << std::endl;
	}
}

void readDataWin(const char* filename, const char* shrname, vector<MarketDataFile>& vec)
{

	vec.clear();
	const char* sharename = shrname;
	DWORD error_code;
	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);     //存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;	//共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;//|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING; //文件属性
	HANDLE mmHandle = nullptr;
	HANDLE mmfm = nullptr;
	char* mmfm_base_address = nullptr;
	try
	{
		//创建文件
		mmHandle = CreateFile(filename, access_mode, share_mode, NULL, OPEN_ALWAYS, flags, NULL);
		if (mmHandle == INVALID_HANDLE_VALUE)
		{
			error_code = GetLastError();
			std::cout << "创建mmf失败:" << error_code << std::endl;
			return;
		}
		else
		{
			DWORD high_size;
			DWORD file_size = GetFileSize(mmHandle, &high_size);
			if (file_size == 0)
			{
				return;
			}
			size_t vecsize = file_size / sizeof(MarketDataFile);
			vec.resize(vecsize);
			std::cout << "FILE_SIZE: " << file_size << ", HIGH_SIZE: " << high_size << std::endl;
			if (file_size == BAD_POS && (error_code = GetLastError()) != 0)
			{
				CloseHandle(mmHandle);
				std::cout << "error：" << error_code << endl;
				return;
			}
			std::cout << "create mmf sucessfully" << endl;
			DWORD size_high = high_size;
			mmfm = CreateFileMapping(mmHandle,
				NULL,
				PAGE_READWRITE,
				size_high,
				file_size,
				sharename);

			//需要在另外一个进程打开
			//HANDLE mmfm = OpenFileMapping(FILE_MAP_READ, false, sharename);
			if (mmfm == nullptr)
			{
				std::cout << "OpenFileMapping error: " << GetLastError() << std::endl;
				if (mmHandle != INVALID_HANDLE_VALUE)
				{
					CloseHandle(mmHandle);
					return;
				}
				return;
			}
			else
			{
				DWORD view_access = FILE_MAP_ALL_ACCESS;
				//获得映射视图
				mmfm_base_address = (char*)MapViewOfFile(mmfm, view_access, 0, 0, file_size);
				if (mmfm_base_address == NULL) {
					error_code = GetLastError();
					if (error_code != 0) {
						std::cout << "errorcode " << error_code << endl;
						return;
					}
				}
				else
				{
					//向内存映射视图中写数据
					//CopyMemory((PVOID)mmfm_base_address, vec.data(), write_size);
					memcpy(vec.data(), mmfm_base_address, file_size);
					//卸载映射
					UnmapViewOfFile(mmfm_base_address);
				}



				//关闭内存映射文件
				CloseHandle(mmfm);
			}

			//关闭文件
			CloseHandle(mmHandle);
		}
		std::cout << " data read success" << std::endl;

	}
	catch (const std::exception& e)
	{
		if (mmHandle != nullptr) CloseHandle(mmHandle);
		if (mmfm != nullptr) CloseHandle(mmfm);
		if (mmfm_base_address != nullptr) UnmapViewOfFile(mmfm_base_address);
		std::cout << "error：" << e.what() << std::endl;
	}
}


//进程通信不通过文件
void savaDataWin_NO_FILE(const char* sharename, const vector<MarketDataFile>& vec)
{
	HANDLE hMapFile = nullptr;
	LPCTSTR pBuf = nullptr;
	try
	{
		const size_t write_size = sizeof(MarketDataFile) * vec.size();
		std::cout << "data_size: " << write_size << ", vector_size:  " << vec.size() << std::endl;
		DWORD error_code;
		hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			1024000,
			sharename);
		if (hMapFile == nullptr)
		{
			std::cout << "Could not create file mapping object: " << GetLastError() << std::endl;
			return;
		}

		pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024000);
		if (pBuf == NULL)
		{
			std::cout << "Could not map view of filt: " << GetLastError() << std::endl;
			CloseHandle(hMapFile);
			return;
		}
		char length[7 + 1] = { 0 };
		snprintf(length, sizeof(length), "%d", write_size);
		CopyMemory((PVOID)pBuf, length, sizeof(length));
		CopyMemory((PVOID)(pBuf + sizeof(length)), vec.data(), write_size);
		//UnmapViewOfFile(pBuf);
		//CloseHandle(hMapFile);
	}
	catch (const std::exception& e)
	{
		if (hMapFile != nullptr) CloseHandle(hMapFile);
		if (pBuf != nullptr) UnmapViewOfFile(pBuf);
		std::cout << "error：" << e.what() << std::endl;
	}
	return;
}

//进程通信不通过文件
void readDataWin_NO_FILE(const char* sharename, vector<MarketDataFile>& vec)
{
	HANDLE hMapFile = nullptr;
	LPCTSTR pBuf = nullptr;
	try
	{
		DWORD error_code;
		hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS,
			FALSE,
			sharename);
		if (hMapFile == NULL)
		{
			std::cout << "Could not open file mapping object: " << GetLastError() << std::endl;
			return;
		}
		pBuf = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024000);
		if (pBuf == NULL)
		{
			std::cout << "Could not map view of filt: " << GetLastError() << std::endl;
			CloseHandle(hMapFile);
			return;
		}
		char length[7 + 1] = { 0 };
		memcpy(length, (PVOID)pBuf, sizeof(length));
		int len = atoi(length);
		size_t vecsize = len / sizeof(MarketDataFile);
		vec.resize(vecsize);
		memcpy(vec.data(), (PVOID)(pBuf + sizeof(length)), len);
		UnmapViewOfFile(pBuf);
		CloseHandle(hMapFile);
	}
	catch (const std::exception& e)
	{
		if (hMapFile != nullptr) CloseHandle(hMapFile);
		if (pBuf != nullptr) UnmapViewOfFile(pBuf);
		std::cout << "error：" << e.what() << std::endl;
	}
	return;
}


//读取大文件 
void  readDataWin_2(const char* filename, const char* shrname, vector<MarketDataFile>& vec)
{

	TimeCost  timecost("ReadData");
	vec.clear();
	PBYTE  buffer = nullptr;                                //数据缓存
	size_t bufferend = 0;                                   //缓存结束位置
	const char* sharename = shrname;                        //内存映射名称
	DWORD error_code;
	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);     //存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;	//共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;//|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING; //文件属性
	HANDLE hFile = nullptr;
	HANDLE hFileMap = nullptr;
	LPBYTE lpbMapAddress = nullptr;
	try
	{
		//创建文件
		hFile = CreateFile(filename, access_mode, share_mode, NULL, OPEN_ALWAYS, flags, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			error_code = GetLastError();
			std::cout << "创建文件对象失败:" << error_code << std::endl;
			return;
		}
		// 创建文件映射对象 视图取当前文件大小
		HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, sharename);
		if (hFileMap == NULL)
		{
			error_code = GetLastError();
			// 关闭文件对象  
			CloseHandle(hFile);
			std::cout << "创建文件映射对象失败:" << error_code << std::endl;
			return;
		}
		// 得到系统分配粒度  
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		//块分配基本单位
		DWORD dwGran = SysInfo.dwAllocationGranularity;
		// 得到文件大小 高位与低位 低于4G 高位为0
		DWORD dwFileSizeHigh;
		__int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
		qwFileSize |= (((__int64)dwFileSizeHigh) << 32);
		// 偏移地址   
		__int64 qwFileOffset = 0;
		// 块大小
		DWORD dwBlockBytes = 10 * dwGran;
		buffer = (PBYTE)malloc(dwBlockBytes * 2);
		if (qwFileSize < dwBlockBytes)
			dwBlockBytes = (DWORD)qwFileSize;
		while (qwFileSize > 0)
		{
			vec.clear();
			// 映射视图  
			//需为64K整数倍
			DWORD highoffset = (DWORD)(qwFileOffset >> 32);
			DWORD loweroffset = (DWORD)(qwFileOffset & 0xFFFFFFFF);
			lpbMapAddress = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, highoffset, loweroffset, dwBlockBytes);
			if (lpbMapAddress == NULL)
			{
				error_code = GetLastError();
				std::cout << "映射文件映射失败:" << error_code << std::endl;
				return;
			}
			// 对映射的视图进行访问  
			memcpy(buffer + bufferend, lpbMapAddress, dwBlockBytes);
			bufferend += dwBlockBytes;
			size_t vecLengh = bufferend / sizeof(MarketDataFile);
			vec.resize(vecLengh);
			memcpy(vec.data(), buffer, vecLengh * sizeof(MarketDataFile));
			memmove(buffer, buffer + vecLengh * sizeof(MarketDataFile), vecLengh * sizeof(MarketDataFile));
			bufferend -= vecLengh * sizeof(MarketDataFile);

			// 撤消文件映像  
			UnmapViewOfFile(lpbMapAddress);
			// 修正偏移量 
			qwFileOffset += dwBlockBytes;
			qwFileSize -= dwBlockBytes;
			if (qwFileSize < dwBlockBytes) dwBlockBytes = qwFileSize;
			lpbMapAddress = nullptr;
		}

		if (bufferend > 0)
		{
			vec.clear();
			vec.resize(bufferend / sizeof(MarketDataFile));
			memcpy(vec.data(), buffer, bufferend);
		}
		if (buffer != nullptr)free(buffer);
		// 关闭文件映射对象句柄  
		CloseHandle(hFileMap);
		CloseHandle(hFile);
		std::cout << "data read success" << std::endl;

	}
	catch (const std::exception& e)
	{
		if (hFile != nullptr) CloseHandle(hFile);
		if (hFileMap != nullptr) CloseHandle(hFileMap);
		if (lpbMapAddress != nullptr) UnmapViewOfFile(lpbMapAddress);
		std::cout << "error：" << e.what() << std::endl;
	}

}



MemoryFile::MemoryFile(const char* filename, const char* sharefilename) :m_strsharefilename(sharefilename), m_strfilename(filename)
{

}


MemoryFile::~MemoryFile()
{
}


void MemoryFile::saveData(const vector<MarketDataFile>& vec, int flag)
{
	savaDataWin(m_strfilename.c_str(), m_strsharefilename.c_str(), vec, flag);
	//savaDataWin_NO_FILE(m_strsharefilename.c_str(), vec);
}
void MemoryFile::readData()
{
	//readDataWin(m_strfilename.c_str(), m_strsharefilename.c_str(), m_vecData);
	//readDataWin_NO_FILE(m_strsharefilename.c_str(), m_vecData);
	readDataWin_2(m_strfilename.c_str(), m_strsharefilename.c_str(), m_vecData);
}
