// BlkChain.cpp                                            (c) DIR, 2019
//
//              ���������� ������� ������ ��������
//
// CBlockChain - ������ ��������
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif


//      ���������� ������ CBlockChain
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//

// ����������� � ����������
CBlockChain::CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode)
           : m_aCrypt(KEYCONTNAME),
             m_rUsers(&m_aCrypt),
             m_rBalances(&m_aCrypt),
             m_rTransacts(&m_aCrypt, &m_rUsers)
{
  m_pMainWin = pMainWin;    //������� ���� ���������
  m_paNode = paNode;        //���� ������������ ����
  CancelAuthorization();    //�������� ������ �����������
  m_nActNodes = 0;
  m_iActNode = 0xFFFF;
}
CBlockChain::~CBlockChain(void)
{
} // CBlockChain::~CBlockChain()

//public:
// ������� ������
// -------------------------------------------------------------------
// ������������� �����. ��������� ��������������� ������ �
//���������������� ������������
WORD  CBlockChain::AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd)
{
  WORD iUser;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b)  b = m_rUsers.AuthorizationCheck(pszLogin, pszPasswd, iUser);
  else {
    // ������ �������� �������� �������������
    ; //TraceW(m_rUsers.ErrorMessage());
  }
  return b ? iUser : UNAUTHORIZED;
}
WORD  CBlockChain::AuthorizationCheck(CString sLogin, CString sPasswd)
{
  return AuthorizationCheck(sLogin.GetBuffer(), sPasswd.GetBuffer());
} // CBlockChain::AuthorizationCheck()

// ������������ ������������
bool  CBlockChain::Authorize(WORD iUser)
{
  // ������������� ������� ������������
  if (!m_aCrypt.IsInitialized())  m_aCrypt.Initialize();
  bool b = m_rUsers.IsLoaded();
  if (b) {  //����� ������� ������������� m_rUsers ��� ������ ���� ��������!
    m_rUsers.SetOwner(iUser);
    // ��������� ������� ������� ��������
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    // ��������� ������� ����������
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Authorize()

// �������� ����������� ������������
void  CBlockChain::CancelAuthorization()
{
  m_rUsers.ClearOwner();
  m_rBalances.ClearOwner();
  m_rTransacts.ClearOwner();
  memset(m_tVoteRes, 0xFF, sizeof(m_tVoteRes));
  memset(m_iNodes, 0xFF, sizeof(m_iNodes));
  memset(m_iActNodes, 0, sizeof(m_iActNodes));
} // CBlockChain::CancelAuthorization()

// ������� ������������ � ���� ���� (����� ����������� � ������� ����)
void  CBlockChain::TieUpUserAndNode(WORD iUser, WORD iNode)
{
  ASSERT(0 <= iUser && iUser < NODECOUNT && 0 <= iNode && iNode < NODECOUNT);
  m_iNodes[iUser] = iNode;
  m_tVoteRes[iNode].Init(iUser);
} // CBlockChain::TieUpUserAndNode()

// �������� ������������ �� ���� ����
void  CBlockChain::UntieUserAndNode(WORD iNode)
{
  if (0 <= iNode && iNode < NODECOUNT) //0 <= iUser && iUser < NODECOUNT && 
  {
    WORD iUser = m_tVoteRes[iNode]._iUser;
    memset(m_tVoteRes+iNode, 0xFF, sizeof(TVotingRes));
    m_iNodes[iUser] = 0xFFFF;
  }
} // CBlockChain::UntieUserAndNode()

// ������������ ����� �� ��������� DTB, ���������� ��������� �������
bool  CBlockChain::GetReplyOnDTB(TMessageBuf &mbDTB, LPTSTR pszMessg)
//mbDTB    - ����� ��������� (������ ��������� ������ ����� DTB)
//pszMessg - ��������� ��������� ������ ��� ������
{
  // ����������� ���������� � ����� ��� ���������
  TMessageBuf  tMessBuf(mbDTB.MessBuffer(), mbDTB.MessageBytes());
  //TCHAR chMessg[CHARBUFSIZE];  //����� ��������� (TCHAR=wchar_t)
  WORD iNodeFrom;
  // ������� ���������� �� DTB, ���������� iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;  bool b;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
case ScC_ATR:   //Add in Transaction Register
    // ��������������� � ���� (tCmd._szNode) (tCmd._szBlock) ������ 
    //�������� ����������
    b = UpdateTransactionRegister(tCmd.GetNode(), tCmd.GetBlockNo());
    goto NoWrap;
case ScC_GTB:   //Get Transaction Register Block
    b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf);   //����� - TBL
    break;
case ScC_TAQ:   //TransAction reQuest
    b = TransActionreQuest(tCmd, tMessBuf);     //����� - TAR
    break;
case ScC_TRA:   //execute TRansAction
    b = ExecuteMyTransact(tMessBuf);
case 0:         //��� �� ������� - ����� ACK
NoWrap:
    swprintf_s(pszMessg, CHARBUFSIZE, _T("ACK %02d%02d"),
               m_paNode->m_iOwnNode, iNodeFrom);
    return true;
  } //switch
  m_paNode->WrapData(iNodeFrom, tMessBuf);      //��������� ����� � DTB
  // ����������� ��������� �� ������� �����
  memcpy(pszMessg, tMessBuf.MessBuffer(), tMessBuf.MessageBytes());
  pszMessg[tMessBuf.MessageBytes()] = _T('\0'); //�� ������ ������
  return true;
} // CBlockChain::GetReplyOnDTB()

// ���������� ��������� DTB �� ������� (���������� ��������� �������)
bool  CBlockChain::ProcessDTB()
{
  TMessageBuf *pmbDTB, *pmbReply;
  bool b = m_paNode->m_oDTBQueue.Get(pmbDTB);   //���� DTB
  if (!b)  return b;
  // ����������� ��������� DTB � ����� ��� ���������
  TMessageBuf  tMessBuf(pmbDTB->MessBuffer(), pmbDTB->MessageBytes());
  WORD iNodeFrom;
  // ������� ���������� �� DTB, ���������� iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
  case ScC_ATR:   //Add in Transaction Register
      // ��������������� � ���� (tCmd._szNode) (tCmd._szBlock) ������ 
      //�������� ����������
      b = UpdateTransactionRegister(tCmd.GetNode(), tCmd.GetBlockNo());
      break;
  case ScC_GTB:   //Get Transaction Register Block
      b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf); //����� - TBL
      pmbReply = RequestNoReply(iNodeFrom, tMessBuf);
      break;
  case ScC_TAQ:   //TransAction reQuest
      b = TransActionreQuest(tCmd, tMessBuf);              //����� - TAR
      pmbReply = RequestNoReply(iNodeFrom, tMessBuf);
      break;
  case ScC_TAR:   //TransAction request Replay
      b = AddInVotingTable(iNodeFrom, &tMessBuf);   //������� � ������� �����
      break;
  case ScC_TBL:   //Transaction BLock
      b = AddTransactionBlock(tMessBuf);    //�������� � ������� ����������
      break;
  case ScC_TRA:   //execute TRansAction
      b = ExecuteMyTransact(tMessBuf);
  case 0:         //��� �� ������� - ����� ACK
      break;
/*  NoWrap:
      swprintf_s(pszMessg, CHARBUFSIZE, _T("ACK %02d%02d"),
          m_paNode->m_iOwnNode, iNodeFrom);
      return true; */
  } //switch
/*  m_paNode->WrapData(iNodeFrom, tMessBuf);      //��������� ����� � DTB
  // ����������� ��������� �� ������� �����
  memcpy(pszMessg, tMessBuf.MessBuffer(), tMessBuf.MessageBytes());
  pszMessg[tMessBuf.MessageBytes()] = _T('\0'); //�� ������ ������ */
  return b;
} // CBlockChain::ProcessDTB()

// ������������ ����������
bool  CBlockChain::Transaction(WORD iUserTo, double rAmount)
//iUserTo - ������ ������������ "����"
//rAmount - ����� ����������
{
  // ��������� ��������� ����������
  ASSERT(0 <= iUserTo && iUserTo < NODECOUNT);
  TBalance *ptBalance = m_rBalances.GetAt(iUserTo);
  double rBalance = ptBalance->GetBalance();
  if (rAmount > rBalance) {
    m_rBalances.SetError(7);    //����� ���������� ��������� �������
    return false;
  }
  if (!m_paNode->IsQuorumPresent())  return false;
  // ������������ ���������� TAQ "��������� ����������"
  WORD nTrans = m_rTransacts.GetNewTransactOrd(); //����� ����� ����������
  WORD iUserFrom = m_rUsers.OwnerIndex();         //������ ��������� ����
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CreateTransactSubcommand(szMessBuf, _T("TAQ"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  TMessageBuf mbRequest, *pmbTRA = NULL;
  WORD iNodeTo;  bool b;
  // ���� ����������� - ��������� ������� TAQ ���� ����� ���� (������� ����)
  for (b = m_paNode->GetFirstNode(iNodeTo, true);
       b; b = m_paNode->GetNextNode(iNodeTo, true)) {
    mbRequest.PutMessage(szMessBuf);    //������� � �����
    if (iNodeTo == m_paNode->m_iOwnNode) {
      pmbTRA = EmulateReply(mbRequest);
      b = AddInVotingTable(iNodeTo, pmbTRA);
    }
    else
      pmbTRA = RequestNoReply(iNodeTo, mbRequest); //������ mbRequest!
    if (pmbTRA) {
      // ����� ������� - ������� � ������� �����������
	  delete pmbTRA;  pmbTRA = NULL;
    }
  }
  return true;
} // CBlockChain::Transaction()

// ���������������� ���������� �����������
bool  CBlockChain::AnalyzeVoting()
{
  return true;
} // CBlockChain::AnalyzeVoting()

// ��������� ������� ������������ ������������ �� ���������� �����
void  CBlockChain::ActualizeTransactReg()
{
  //return true;
} // CBlockChain::ActualizeTransactReg()

// ������������� �����. �������� ���������� ����������
void  CBlockChain::ExecuteTransaction(TMessageBuf &mbTAQ)
//mbTAQ - ����� � ����������� TAQ
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  // ����������� ���������� � ����� ��� ���������
  int len = min(_tcslen(mbTAQ.Message()), ERMES_BUFSIZE-1);
  memcpy(szMessBuf, mbTAQ.Message(), len * sizeof(TCHAR));
  memset(szMessBuf+len, 0, (ERMES_BUFSIZE-len) * sizeof(TCHAR));
  // ������������� �� ����� ���������� TAQ "��������� ����������" � 
  //���������� TRA "��������� ����������"
  memcpy(szMessBuf, _T("TRA"), 3 * sizeof(TCHAR));
  bool b;  WORD iNodeTo;  TMessageBuf mbRequest, *pmbReply;
  // �������� ���������� �� �������� ����� ����
  for (b = m_paNode->GetFirstNode(iNodeTo);
       b; b = m_paNode->GetNextNode(iNodeTo)) {
    mbRequest.PutMessage(szMessBuf);    //������� � �����
    pmbReply = RequestNoReply(iNodeTo, mbRequest);
    if (pmbReply) {
      // ����� ������� - ���������
      b = (pmbReply->Compare(_T("ACK"), 3) == 0);   //����� �.�. ACK
      delete pmbReply;  pmbReply = NULL;
    }
  }
  // �������� ���������� �� ���� ����
  mbRequest.PutMessage(szMessBuf);  //������� � �����
  b = ExecuteMyTransact(mbRequest);
} // CBlockChain::ExecuteTransaction()

// �������� ���������� �� ���� ����
bool  CBlockChain::ExecuteMyTransact(TMessageBuf &mbTRA)
//iUserFrom - ������ ������������-�����������
//iUserTo   - ������ ������������-����������
//rAmount   - ����� ����������
{
  TSubcmd tCmd;
  if (ParseMessage(mbTRA, tCmd) != ScC_TRA)  return false;
  // ������� ��������� ����������
  WORD nTrans = tCmd.GetTransNo(),
#ifdef DBGHASHSIZE
   iUserFrom = CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashFrom, SHORTNUMSIZ),
   iUserTo = CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashTo, SHORTNUMSIZ);
#else
   iUserFrom = m_rUsers.GetUserByLoginHash(tCmd._chHashFrom),
   iUserTo = m_rUsers.GetUserByLoginHash(tCmd._chHashTo);
#endif
  double rAmount = tCmd.GetAmount();
  ASSERT(nTrans == m_rTransacts.GetNewTransactOrd());
  TTransact *ptRansact =
    m_rTransacts.CreateTransactBlock(iUserFrom, iUserTo, rAmount);
  m_rTransacts.Add(ptRansact);   //������� ����������
  m_pMainWin->AddInTransactTable(ptRansact);
  return true;
} // CBlockChain::ExecuteMyTransact()

//private:
// ���������� ������
// -------------------------------------------------------------------
/* ���������������� ��� ������� � ������
bool  CBlockChain::Initialize()
{
  if (!m_rUsers.IsThereSignedInUser())  return false;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b) {
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Initialize() */

// ������������ ���������� �������, ������ ��� ���������� ����������
void  CBlockChain::CreateTransactSubcommand(
  LPTSTR pszMsg, LPTSTR pszSbcmd, WORD nTrans,
  WORD iUserFrom, WORD iUserTo, double rAmount)
{
  // ������������ ���������� ���������� ���������� �������
  //                                    ==================
  swprintf_s(pszMsg, ERMES_BUFSIZE, _T("%s %08d%02d%02d%016.2f"),
             pszSbcmd, nTrans, iUserFrom, iUserTo, rAmount); //������������
} // CBlockChain::CreateTransactSubcommand()

// ������� ���� ������ ������� ��������� ������������ ������ � �������� �����
TMessageBuf * CBlockChain::RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest)
//iNode     - ������ ����-��������
//mbRequest - ����� �������
{
  TMessageBuf *pmbReply = NULL;
  WORD iNodeFrom;
  m_paNode->WrapData(iNodeTo, mbRequest);   //��������� ���������� � DTB
  // ��������, ����������� ��������� ������� ��� ������ �����������
  //for (WORD i = 0; i < 3; i++) { //max number of attempts = 3
    pmbReply = m_paNode->RequestAndReply(iNodeTo, mbRequest);
    if (pmbReply) {   //����� �������! �.�. ACK
      m_paNode->UnwrapData(iNodeFrom, pmbReply); //������� ���������� �� DTB
      ASSERT(iNodeFrom == iNodeTo);
      delete pmbReply;
    }
  //}
  return NULL;
} // RequestNoReply()

// ����������� ����� ������ ����
TMessageBuf * CBlockChain::EmulateReply(TMessageBuf &mbRequest)
{
  // ������������ ���������� TAR "��������� ����������" �� ����� 
  //���������� TAQ
  TMessageBuf *pmbReply = new TMessageBuf(mbRequest.Message());
  memcpy(pmbReply->Message(), _T("TAR"), 3 * sizeof(TCHAR));
  return pmbReply;
} // CBlockChain::EmulateReply()

// ��������� ���������. ����� ���������� ��� ���������� � ��������� �����.
ESubcmdCode  CBlockChain::ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd)
//mbMess[in] - ���������
//tCmd[out]  - ��������� �������, ��������� ���������
{
  static LPTSTR sCmds[] = {
//������             ��� ����������
/* 0 */ _T("ATR"),  //ScC_ATR = 11, Add in Transact Reg - �������� � ������ ��
/* 1 */ _T("GTB"),  //ScC_GTB,      Get Trans log Block - ���� ���� ������� ��
/* 2 */ _T("TAQ"),  //ScC_TAQ,      TransAction reQuest - ������ ����������
/* 3 */ _T("TAR"),  //ScC_TAR,      TransAction Reply   - ����� �� ������ ����
/* 4 */ _T("TBL"),  //ScC_TBL,      Transact log BLock  - ���� ����������
/* 5 */ _T("TRA"),  //ScC_TRA,      execute TRansAction - ��������� ����������
  };
  static WORD n = sizeof(sCmds) / sizeof(LPTSTR);
  WORD i = 0;  //ESubcmdCode eSbcmd;
  for ( ; i < n; i++) {
    if (mbMess.Compare(sCmds[i], 3) == 0) {
      tCmd._eCode = (ESubcmdCode)(ScC_ATR + i);  goto SbcmdFound;
    }
  }
  tCmd._eCode = (ESubcmdCode)0;
SbcmdFound:
  // ���������� ����������, ����������� ���������
  switch (tCmd._eCode) {
case ScC_ATR:   //Add in Transaction Register
    // ����� (������) ����
    tCmd.SetNode(mbMess.Message() + CMDFIELDSIZ);
    // ���������� ������
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ+SHORTNUMSIZ);
    break;
case ScC_GTB:   //Get Transaction Register Block
    // ����� ����� �� ����� ����
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ);
    break;
case ScC_TAQ:   //TransAction reQuest
case ScC_TAR:   //TransAction Reply
case ScC_TRA:   //execute TRansAction
    // ����� ����������
    tCmd.SetTransNo(mbMess.Message() + CMDFIELDSIZ);
    // ���-��� �����������
    tCmd.SetHashFrom(mbMess.MessBuffer() +
                     (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR));
    // ���-��� ����������
    tCmd.SetHashTo(mbMess.MessBuffer() +
                   (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+_HASHSIZE-2);
    // ����� ����������
    tCmd.SetAmount(mbMess.Message() +
                   CMDFIELDSIZ+LONGNUMSIZE+2*(_HASHSIZE/sizeof(TCHAR)-1));
    break;
case ScC_TBL:   //Transact register BLock
    // ���-��� ����������� ����� ����������
    tCmd.SetHashPrev(mbMess.MessBuffer() + CMDFIELDSIZ*sizeof(TCHAR));
    // ����� ����������
    tCmd.SetTransNo(mbMess.Message() + CMDFIELDSIZ+_HASHSIZE/sizeof(TCHAR)-1);
    // ���-��� �����������
    tCmd.SetHashFrom(mbMess.MessBuffer() +
                     (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+_HASHSIZE-2);
    // ���-��� ����������
    tCmd.SetHashTo(mbMess.MessBuffer() +
                   (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+2*_HASHSIZE-4);
    // ����� ����������
    tCmd.SetAmount(mbMess.Message() +
                   CMDFIELDSIZ+LONGNUMSIZE+3*(_HASHSIZE/sizeof(TCHAR)-1));
    break;
  } //switch
  return tCmd._eCode;   //���������� �� ����������
} // CBlockChain::ParseMessage()

// ��������� ����� � ������� �����������
bool  CBlockChain::AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbTRA)
{
  TSubcmd tCmd;
  if (ParseMessage(*pmbTRA, tCmd) != ScC_TAR)  return false;
  WORD iUser = m_tVoteRes[iNodeFrom]._iUser;
  m_tVoteRes[iNodeFrom]._nTrans = tCmd.GetTransNo();
  m_tVoteRes[iNodeFrom]._iUserFrom =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashFrom, 0); //GetUserByLoginHash
  m_tVoteRes[iNodeFrom]._iUserTo =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashTo, 0);   //GetUserByLoginHash
  m_tVoteRes[iNodeFrom]._rSum = tCmd.GetAmount();
  CString sNode, sAmount;
  TCHAR szOwner[NAMESBUFSIZ], szUserFrom[NAMESBUFSIZ], szUserTo[NAMESBUFSIZ],
        *pszFields[VOTECOLNUM];
  sNode.Format(_T("���� #%d"), iNodeFrom);
  pszFields[0] = sNode.GetBuffer();     //����� ����
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUser, szOwner);
  pszFields[1] = szOwner;               //��� ��������� ����
  pszFields[2] = tCmd._szTrans;         //����� ����������
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserFrom, szUserFrom);
  pszFields[3] = szUserFrom;            //��� ������������-�����������
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserTo, szUserTo);
  pszFields[4] = szUserTo;              //��� ������������-����������
  sAmount.Format(_T("%.2lf"), m_tVoteRes[iNodeFrom]._rSum);
  pszFields[5] = sAmount.GetBuffer();   //�����
  bool b = m_pMainWin->AddInVotingTable(pszFields);
  if (b) {
    m_nVote++;
    if (IsTheVoteOver()) {
      // ���������������� ���������� �����������
      if (!AnalyzeVoting()) {
        // ���������� ���������: ����� ���������� ������� - ��������� ���
        //������� ������������ ������������ � ����������� ������� �������
        ActualizeTransactReg();
      }
      // �������� ���������� ����������
      //mbRequest.PutMessage(szMessBuf);      //������� � �����
      ExecuteTransaction(*pmbTRA);
    }
  }
  return b;
} // CBlockChain::AddInVotingTable()

// ��������������� ���� ������� ����������
bool  CBlockChain::UpdateTransactionRegister(WORD iNodeFrom, WORD nBlocks)
//iNodeFrom - ����� (������) ����, � �������� ����� ��������� ����� ����������
//nBlocks   - ���������� ����������� ��������� ������ ����������
{
  return true;
} // CBlockChain::UpdateTransactionRegister()

// ���������� ��������� GTB "���� ���� �������� ����������".
//� ����� ����������� ��������� TBL.
bool  CBlockChain::GetTransactionBlock(WORD nBlock, TMessageBuf &mbReply)
//nBlock[in]      - ����� ����� �� ����� ���� (1,2,..)
//mbReply[in/out] - ����� ���������-������
{
  TCHAR chMess[CHARBUFSIZE];  //����� ��������� (TCHAR=wchar_t)
  WORD n = m_rTransacts.ItemCount();
  //ASSERT(0 < nBlock && nBlock < n);
  TTransact *ptRansact = m_rTransacts.GetAt(n - nBlock);
  _tcscpy_s(chMess, CHARBUFSIZE-1, _T("TBL "));
  memcpy(chMess + 4, ptRansact, sizeof(TTransact));
  mbReply.Put((BYTE *)chMess, sizeof(TCHAR) * 4 + sizeof(TTransact));
  return true;
} // CBlockChain::GetTransactionBlock()

// ���������� ��������� TAQ "������ ����������".
//� ����� ����������� ��������� TAR.
bool  CBlockChain::TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbReply)
//tCmd[out]       - ��������� �������, ��������� ��������� TAQ
//mbReply[in/out] - �����
{
  LPTSTR pszMsg = mbReply.Message();  DWORD n;
  swscanf_s((LPTSTR)tCmd._chHashFrom, _T("%02d"), &n);
  WORD nTrans = m_rTransacts.GetNewTransactOrd(),
       iUserFrom = (WORD)n;
  TBalance *ptBalnc = m_rBalances.GetAt(iUserFrom);
  double rAmount = ptBalnc->GetBalance();
  TCHAR chMess[CHARBUFSIZE];
  swprintf_s(chMess, CHARBUFSIZE, _T("TAR %08d%016.2f"), nTrans, rAmount);
  n = 4 + LONGNUMSIZE;
  memcpy(pszMsg, chMess, sizeof(TCHAR) * n);
  memcpy(pszMsg + n + _HASHSIZE, chMess + n, sizeof(TCHAR)*NAMESBUFSIZ);
  return true;
} // CBlockChain::TransActionreQuest()

// �������� � ������� ���������� ����� ���� (��� ������������).
bool  CBlockChain::AddTransactionBlock(TMessageBuf& mbDTB)
{
  return true;
} // CBlockChain::AddTransactionBlock()

// End of class CBalanceRegister definition ----------------------------
