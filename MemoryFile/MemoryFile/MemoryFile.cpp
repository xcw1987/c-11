// MemoryFile.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//测试内存映射文件

//
#include <iostream>
#include "MemoryFile.h"
#include<Windows.h>
using namespace std;


#define BAD_POS 0xFFFFFFFF // returned by SetFilePointer and GetFileSize

void savaDataWin(const char* filename, const char* sharename, const vector<MarketDataFile>& vec, int flag)
{
	//const DWORD mmf_size = 512 * 1024;
	const size_t write_size = sizeof(MarketDataFile) * vec.size();
	std::cout << "data_size: " << write_size <<", vector_size:  " << vec.size() << std::endl;
	DWORD error_code;
	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);     //存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;	//共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;//|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING; //文件属性
	try
	{
		//创建文件
		HANDLE mmHandle = CreateFile(filename, access_mode, share_mode, NULL, OPEN_ALWAYS, flags, NULL);
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
			size_t totalsize = (flag == MEM_OVERRIDE ? write_size : (file_size + write_size));
			
			std::cout << "FILE_SIZE: " << file_size << ", HIGH_SIZE: " << high_size << std::endl;
			if (file_size == BAD_POS && (error_code = GetLastError()) != 0)
			{
				CloseHandle(mmHandle);
				cout << "error：" << error_code << endl;
				return;
			}
			cout << "create mmf sucessfully" << endl;
			DWORD size_high = 0;
			HANDLE mmfm = CreateFileMapping(mmHandle,
				NULL,
				PAGE_READWRITE | SEC_COMMIT,
				size_high,
				totalsize,
				sharename);
			error_code = GetLastError();
			if (error_code != 0)
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
					char* mmfm_base_address = (char*)MapViewOfFile(mmfm, view_access, 0, 0, totalsize);
					if (mmfm_base_address == NULL) {
						error_code = GetLastError();
						if (error_code != 0) {
							cout << "errorcode "<<error_code<<endl;
							return;
						}
					}
					else 
					{
						//向内存映射视图中写数据
						size_t pos = (flag == MEM_OVERRIDE ? 0 : (file_size));
						CopyMemory((PVOID)(mmfm_base_address + pos), vec.data(), write_size);
						//卸载映射
						UnmapViewOfFile(mmfm_base_address);
					}

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
		std::cout << "error：" << e.what() << std::endl;
	}

}

void readDataWin(const char* filename, const char* sharename, vector<MarketDataFile>& vec)
{
	
	vec.clear();
	DWORD error_code;
	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);     //存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;	//共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;//|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING; //文件属性

	try
	{
		//创建文件
		HANDLE mmHandle = CreateFile(filename, access_mode, share_mode, NULL, OPEN_ALWAYS, flags, NULL);
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
				return ;
			}
			size_t vecsize = file_size / sizeof(MarketDataFile);
			vec.resize(vecsize);
			std::cout << "FILE_SIZE: " << file_size << ", HIGH_SIZE: " << high_size << std::endl;
			if (file_size == BAD_POS && (error_code = GetLastError()) != 0)
			{
				CloseHandle(mmHandle);
				cout << "error：" << error_code << endl;
				return;
			}
			cout << "create mmf sucessfully" << endl;
			DWORD size_high = high_size;
		/*	HANDLE mmfm = CreateFileMapping(mmHandle,
				NULL,
				PAGE_READWRITE,
				size_high,
				file_size,
				sharename);*/

			//需要在另外一个进程打开
			HANDLE mmfm = OpenFileMapping(FILE_MAP_READ, false, sharename);
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
					char* mmfm_base_address = (char*)MapViewOfFile(mmfm, view_access, 0, 0, file_size);
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
		std::cout << "error：" << e.what() << std::endl;
	}
}




MemoryFile::MemoryFile(const char* filename, const char* sharefilename):m_strsharefilename(sharefilename),m_strfilename(filename)
{

}


MemoryFile::~MemoryFile()
{
}


void MemoryFile::saveData(const vector<MarketDataFile>& vec, int flag )
{
	savaDataWin(m_strfilename.c_str(), m_strsharefilename.c_str(),  vec, flag);
}
void MemoryFile::readData()
{
	readDataWin(m_strfilename.c_str(), m_strsharefilename.c_str(), m_vecData);
}
