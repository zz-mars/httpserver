/************************************************************************
 *
 * �ļ����ƣ� mpq_hash.h
 * �ļ���ʶ��
 * ����ժҪ�� 
 * ����˵����
 * ��ǰ�汾�� 0.1
 * ��    �ߣ� gavinsu
 * ������ڣ� 11/17/2009
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ�
 ************************************************************************/
#ifndef _MPQ_HASH_H
#define _MPQ_HASH_H

void MPQ_Init();


// �޶��棺�������к���ʱ���Զ�����MPQ_Init(), ����Ҫ�ֶ�����Init����.
// By jeeeryliu on 2013-1-24

unsigned int MPQ_Hash(const char *lpszFileName, unsigned int dwHashType, bool bCaseSensitive);


unsigned long long GetMPQHashFromString(const char *, bool bCaseSensitive = false);

#endif

