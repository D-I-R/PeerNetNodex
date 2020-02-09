// BlkChain.h                                              (c) DIR, 2019
//
//              ���������� ������� ������ ��������
//
// CBlockChain - ������ ��������
//
////////////////////////////////////////////////////////////////////////

#pragma once

//#include "afxtempl.h"
#include "FileBase.h"

#define  CMDFIELDSIZ  4     //������ ���� �������
#define  SHORTNUMSIZ  2     //������ ������ ��� ���������� ������
#define  LONGNUMSIZE  8     //������ ������ ����������
#define  VOTECOLNUM   6     //���������� �������� � ������� �����������
#define  TRANCOLNUM   4     //���������� �������� � ������� ����������
// ��� ���������� ������ ���������� (UNICODE):
#define  KEYCONTNAME  TEXT("Peer Net Key Container")

// �������������� (����) ��������� ������� DTB
enum ESubcmdCode {
  ScC_ATR = 11, //Add in Transaction Log - ����� �������� � ������ ����������
  ScC_GTB,      //Get Transaction log Block - ���� ���� ������� ����������
  ScC_TAQ,      //TransAction reQuest    - ������ ����������
  ScC_TAR,      //TransAction Reply      - ����� �� ������ ����������
  ScC_TBL,      //Transaction log BLock  - ���� ����������
  ScC_TRA,      //execute TRansAction    - ��������� ����������
};
// ����������� ��� "��������� ��������� (����������)"
struct TSubcmd {
  ESubcmdCode  _eCode;              //��� ����������
  TCHAR  _szNode[SHORTNUMSIZ+1];    //����� ���� (2)
  TCHAR  _szBlock[SHORTNUMSIZ+1];   //����� ��� ���������� ������ (2)
  TCHAR  _szTrans[LONGNUMSIZE+1];   //����� ���������� (8)
  BYTE   _chHashPrev[_HASHSIZE];     //���-��� ����������� �����
  BYTE   _chHashFrom[_HASHSIZE];     //���-��� ��� ����� �����-�����������
  BYTE   _chHashTo[_HASHSIZE];       //���-��� ��� ����� �����-����������
  TCHAR  _szAmount[NAMESBUFSIZ+1];  //����� ���������� (��� ��� �������)
  // ��������� ���������
  void  Clear() {
    memset(&_eCode, 0, sizeof(TSubcmd));
  }
  void  SetNode(LPTSTR pszNode) {
    memcpy(_szNode, pszNode, SHORTNUMSIZ * sizeof(TCHAR));
    _szNode[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetNode() {
    DWORD n;
    swscanf_s(_szNode, _T("%02d"), &n);
    return (WORD)n;
  }
  void  SetBlockNo(LPTSTR pszNo) {
    memcpy(_szBlock, pszNo, SHORTNUMSIZ * sizeof(TCHAR));
    _szBlock[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetBlockNo() {
    DWORD n;
    swscanf_s(_szBlock, _T("%02d"), &n);
    return (WORD)n;
  }
  void  SetTransNo(LPTSTR pszNo) {
    memcpy(_szTrans, pszNo, LONGNUMSIZE * sizeof(TCHAR));
    _szTrans[LONGNUMSIZE] = _T('\0');
  }
  WORD  GetTransNo() {
    DWORD n;
    swscanf_s(_szTrans, _T("%08d"), &n);
    return (WORD)n;
  }
  void  SetHashPrev(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashPrev, cHash, _HASHSIZE-2);
    memset(_chHashPrev + _HASHSIZE-2, 0, 2);
#else
    memcpy(_chHashPrev, cHash, _HASHSIZE);
#endif
  }
  void  SetHashFrom(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashFrom, cHash, _HASHSIZE-2);
    memset(_chHashFrom + _HASHSIZE-2, 0, 2);
#else
    memcpy(_chHashFrom, cHash, _HASHSIZE);
#endif
  }
  void  SetHashTo(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashTo, cHash, _HASHSIZE-2);
    memset(_chHashTo + _HASHSIZE-2, 0, 2);
#else
    memcpy(_chHashTo, cHash, _HASHSIZE);
#endif
  }
  void  SetAmount(LPTSTR pszSum) {
    memcpy(_szAmount, pszSum, NAMESBUFSIZ * sizeof(TCHAR));
    _szAmount[NAMESBUFSIZ] = _T('\0');
  }
  double  GetAmount() {
    double rSum;
    swscanf_s(_szAmount, _T("%lf"), &rSum);
    return rSum;
  }
}; // struct TSubcmd

// ����������� ��� "������ ������� ����������� �����������"
struct TVotingRes {
  WORD    _iUser;       //�������� ���� (������ � �������)
  WORD    _nTrans;      //����� ����� ����������
  WORD    _iUserFrom;   //������������ "���" (������ � �������)
  WORD    _iUserTo;     //������������ "����" (������ � �������)
  double  _rSum;        //����� ���������� ��� ������� ������� �����
  // ���������������� ���������
  void  Init(WORD iUser) {
    memset(&_iUser, 0, sizeof(TVotingRes));
    _iUser = iUser;
  }
};

class CPeerNetDialog;
class CPeerNetNode;

// ���������� ������ CBlockChain - ������ ��������
////////////////////////////////////////////////////////////////////////

class CBlockChain
{
  CPeerNetDialog  * m_pMainWin;     //������� ���� ���������
  CPeerNetNode    * m_paNode;       //���� ������������ ����
  CCrypto           m_aCrypt;       //������ ������������
  CUserRegister     m_rUsers;       //������� �������������
  CBalanceRegister  m_rBalances;    //������� ������� ��������
  CTransRegister    m_rTransacts;   //������� ����������
  // ������� ������������ iNode->iUser, � ����� ������� ����������� �����:
  TVotingRes  m_tVoteRes[NODECOUNT];
  WORD        m_nVote;              //���������� ��������������� �����
  WORD        m_iNodes[NODECOUNT];      //������� ������������ iUser->iNode
  WORD        m_iActNodes[NODECOUNT];   //������ "����������" ����� ����
  WORD        m_nActNodes;      //���������� "����������" ����� � ������
  WORD        m_iActNode;       //������� ������� ������ "����������" �����
public:                         // (��� ��������)
  // ����������� � ����������
  CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode);
  ~CBlockChain(void);
  // ������� ������
  // -------------------------------------------------------------------
  // ���� ��������� �������� �������������
  CUserRegister    * UserRegister() { return &m_rUsers; }
  // ���� ��������� �������� ������� ��������
  CBalanceRegister * BalanceRegister() { return &m_rBalances; }
  // ���� ��������� �������� ����������
  CTransRegister   * TransRegister() { return &m_rTransacts; }
  // ��������� ��������������� ������ ������������
  WORD  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd);
  WORD  AuthorizationCheck(CString sLogin, CString sPasswd);
  // ������������ ������������
  bool  Authorize(WORD iUser);
  // ���� ��������������� ������������ (��������� ����)
  WORD  GetAuthorizedUser() { return m_rUsers.OwnerIndex(); }
  // �������� ����������� ������������
  void  CancelAuthorization();
  // ������� ������������ � ����� ����
  void  TieUpUserAndNode(WORD iUser, WORD iNode);
  // �������� ������������ �� ���� ����
  void  UntieUserAndNode(WORD iNode);
  // ���� ������������ ��������� ����
  WORD  GetNodeOwner(WORD iNode) { return m_tVoteRes[iNode]._iUser; }
  // ������������ ����� �� ��������� DTB, ���������� ��������� �������
  bool  GetReplyOnDTB(TMessageBuf &mbDTB, LPTSTR pszMessg);
  // ���������� ��������� DTB �� ������� (���������� ��������� �������)
  bool  ProcessDTB();
  // ��������� � �������� ����������
  bool  Transaction(WORD iUserTo, double rAmount);
  // ����������� ���������?
  bool  IsTheVoteOver() { return (m_nVote == m_paNode->m_nNodes); }
  // ���������������� ���������� �����������
  bool  AnalyzeVoting();
  // ��������� ������� ������������ ������������ �� ���������� �����
  void  ActualizeTransactReg();
  // �������� ���������� ����������
  void  ExecuteTransaction(TMessageBuf &mbTAQ);
  // �������� ���������� �� ���� ����
  bool  ExecuteMyTransact(TMessageBuf &mbTRA);
private:
  // ���������� ������
  // -------------------------------------------------------------------
  // ���������������� ��� ������� � ������
  //bool  Initialize();
  // ������������ ���������� ������� ��� ���������� ����������
  void  CreateTransactSubcommand(LPTSTR pszMsg, LPTSTR pszSbcmd,
                                 WORD nTrans, WORD iUserFrom,
                                 WORD iUserTo, double rAmount);
  // ������� ���� ������ ������� ��������� ������������ ������ � �������� �����
  TMessageBuf * RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // ����������� ����� ������ ����
  TMessageBuf * EmulateReply(TMessageBuf &mbRequest);
  // ��������� ���������. ����� ���������� ��� ���������� � ��������� �����.
  ESubcmdCode  ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd);
  // ��������� ����� � ������� �����������
  bool  AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbReply);
  // ��������������� ���� ������� ����������
  bool  UpdateTransactionRegister(WORD iNodeFrom, WORD nBlocks);
  // ���������� ��������� GTB "���� ���� �������� ����������".
  //� ����� ����������� ��������� TBL.
  bool  GetTransactionBlock(WORD nBlock, TMessageBuf &mbReply);
  // ���������� ��������� TAQ "������ ����������".
  //� ����� ����������� ��������� TAR.
  bool  TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbReply);
  // �������� � ������� ���������� ����� ���� (��� ������������).
  bool  AddTransactionBlock(TMessageBuf &mbDTB);
};

// End of class CBlockChain declaration --------------------------------
