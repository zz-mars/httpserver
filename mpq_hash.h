/************************************************************************
 *
 * 文件名称： mpq_hash.h
 * 文件标识：
 * 内容摘要： 
 * 其它说明：
 * 当前版本： 0.1
 * 作    者： gavinsu
 * 完成日期： 11/17/2009
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容：
 ************************************************************************/
#ifndef _MPQ_HASH_H
#define _MPQ_HASH_H

void MPQ_Init();


// 修订版：调用下列函数时会自动调用MPQ_Init(), 不需要手动调用Init函数.
// By jeeeryliu on 2013-1-24

unsigned int MPQ_Hash(const char *lpszFileName, unsigned int dwHashType, bool bCaseSensitive);


unsigned long long GetMPQHashFromString(const char *, bool bCaseSensitive = false);

#endif

