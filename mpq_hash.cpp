/************************************************************************
 *
 * 文件名称： mpq_hash.c
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
#include <ctype.h>
#include "mpq_hash.h"

static unsigned int cryptTable[0x500];

static bool bAlreadyMpqInit = false;

void MPQ_Init()
{
    unsigned int seed = 0x00100001, index1 = 0, index2 = 0, i;
    for( index1 = 0; index1 < 0x100; index1++ )
    {
        for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
        {
            unsigned int temp1, temp2;
             seed = (seed * 125 + 3) % 0x2AAAAB;
             temp1 = (seed & 0xFFFF) << 0x10;
             seed = (seed * 125 + 3) % 0x2AAAAB;
             temp2 = (seed & 0xFFFF);
             cryptTable[index2] = ( temp1 | temp2 );
        }
    }
    bAlreadyMpqInit = true;
}

unsigned int MPQ_Hash(const char *pstrSourceSTR, unsigned int dwHashType, bool bCaseSensitive)
{ 
	if(!bAlreadyMpqInit)
	{
		MPQ_Init();
	}
    unsigned char *key = (unsigned char *)pstrSourceSTR; 
    unsigned int seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE; 
    int ch; 

    while(*key != 0) 
    { 
        ch=bCaseSensitive?(*key):(toupper(*key));
        key++;

        seed1 = cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2); 
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3; 
    } 
    return seed1; 
}


unsigned long long GetMPQHashFromString(const char *szDomain, bool bCaseSensitive)
{
	unsigned long long key = MPQ_Hash(szDomain, 0, bCaseSensitive);
	unsigned long long value1 = MPQ_Hash(szDomain, 1, bCaseSensitive);
	unsigned long long value2 = MPQ_Hash(szDomain, 2, bCaseSensitive);
	unsigned long long ullHash = (value1 << 32) + value2;
	return ullHash;
}
