// BlkChain.cpp                                            (c) DIR, 2019
//
//              Реализация классов модели блокчейн
//
// CBlockChain - Модель блокчейн
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif


//      Реализация класса CBlockChain
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//

// Конструктор и деструктор
CBlockChain::CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode)
           : m_aCrypt(KEYCONTNAME),
             m_rUsers(&m_aCrypt),
             m_rBalances(&m_aCrypt),
             m_rTransacts(&m_aCrypt, &m_rUsers)
{
  m_pMainWin = pMainWin;    //главное окно программы
  m_paNode = paNode;        //узел одноранговой сети
  CancelAuthorization();    //очистить данные авторизации
  m_nActNodes = 0;
  m_iActNode = 0xFFFF;
}
CBlockChain::~CBlockChain(void)
{
} // CBlockChain::~CBlockChain()

//public:
// Внешние методы
// -------------------------------------------------------------------
// Перегруженный метод. Проверить регистрационные данные и
//идентифицировать пользователя
WORD  CBlockChain::AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd)
{
  WORD iUser;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b)  b = m_rUsers.AuthorizationCheck(pszLogin, pszPasswd, iUser);
  else {
    // Ошибка загрузки регистра пользователей
    ; //TraceW(m_rUsers.ErrorMessage());
  }
  return b ? iUser : UNAUTHORIZED;
}
WORD  CBlockChain::AuthorizationCheck(CString sLogin, CString sPasswd)
{
  return AuthorizationCheck(sLogin.GetBuffer(), sPasswd.GetBuffer());
} // CBlockChain::AuthorizationCheck()

// Авторизовать пользователя
bool  CBlockChain::Authorize(WORD iUser)
{
  // Инициализация объекта криптографии
  if (!m_aCrypt.IsInitialized())  m_aCrypt.Initialize();
  bool b = m_rUsers.IsLoaded();
  if (b) {  //здесь регистр пользователей m_rUsers уже должен быть загружен!
    m_rUsers.SetOwner(iUser);
    // Загрузить регистр текущих остатков
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    // Загрузить регистр транзакций
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Authorize()

// Отменить авторизацию пользователя
void  CBlockChain::CancelAuthorization()
{
  m_rUsers.ClearOwner();
  m_rBalances.ClearOwner();
  m_rTransacts.ClearOwner();
  memset(m_tVoteRes, 0xFF, sizeof(m_tVoteRes));
  memset(m_iNodes, 0xFF, sizeof(m_iNodes));
  memset(m_iActNodes, 0, sizeof(m_iActNodes));
} // CBlockChain::CancelAuthorization()

// Связать пользователя и узел сети (после авторизации и запуска узла)
void  CBlockChain::TieUpUserAndNode(WORD iUser, WORD iNode)
{
  ASSERT(0 <= iUser && iUser < NODECOUNT && 0 <= iNode && iNode < NODECOUNT);
  m_iNodes[iUser] = iNode;
  m_tVoteRes[iNode].Init(iUser);
} // CBlockChain::TieUpUserAndNode()

// Отвязать пользователя от узла сети
void  CBlockChain::UntieUserAndNode(WORD iNode)
{
  if (0 <= iNode && iNode < NODECOUNT) //0 <= iUser && iUser < NODECOUNT && 
  {
    WORD iUser = m_tVoteRes[iNode]._iUser;
    memset(m_tVoteRes+iNode, 0xFF, sizeof(TVotingRes));
    m_iNodes[iUser] = 0xFFFF;
  }
} // CBlockChain::UntieUserAndNode()

// Сформировать ответ на сообщение DTB, полученное серверным каналом
bool  CBlockChain::GetReplyOnDTB(TMessageBuf &mbDTB, LPTSTR pszMessg)
//mbDTB    - буфер сообщения (должен содержать только сообщ DTB)
//pszMessg - указатель выходного буфера для ответа
{
  // Скопировать подкоманду в буфер для изменения
  TMessageBuf  tMessBuf(mbDTB.MessBuffer(), mbDTB.MessageBytes());
  //TCHAR chMessg[CHARBUFSIZE];  //буфер сообщения (TCHAR=wchar_t)
  WORD iNodeFrom;
  // Извлечь подкоманду из DTB, определить iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;  bool b;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
case ScC_ATR:   //Add in Transaction Register
    // Актуализировать в узле (tCmd._szNode) (tCmd._szBlock) блоков 
    //регистра транзакций
    b = UpdateTransactionRegister(tCmd.GetNode(), tCmd.GetBlockNo());
    goto NoWrap;
case ScC_GTB:   //Get Transaction Register Block
    b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf);   //ответ - TBL
    break;
case ScC_TAQ:   //TransAction reQuest
    b = TransActionreQuest(tCmd, tMessBuf);     //ответ - TAR
    break;
case ScC_TRA:   //execute TRansAction
    b = ExecuteMyTransact(tMessBuf);
case 0:         //это не команда - ответ ACK
NoWrap:
    swprintf_s(pszMessg, CHARBUFSIZE, _T("ACK %02d%02d"),
               m_paNode->m_iOwnNode, iNodeFrom);
    return true;
  } //switch
  m_paNode->WrapData(iNodeFrom, tMessBuf);      //упаковать сообщ в DTB
  // Копирование сообщения во внешний буфер
  memcpy(pszMessg, tMessBuf.MessBuffer(), tMessBuf.MessageBytes());
  pszMessg[tMessBuf.MessageBytes()] = _T('\0'); //на всякий случай
  return true;
} // CBlockChain::GetReplyOnDTB()

// Обработать сообщение DTB из очереди (полученное серверным каналом)
bool  CBlockChain::ProcessDTB()
{
  TMessageBuf *pmbDTB, *pmbReply;
  bool b = m_paNode->m_oDTBQueue.Get(pmbDTB);   //дать DTB
  if (!b)  return b;
  // Скопировать сообщение DTB в буфер для изменения
  TMessageBuf  tMessBuf(pmbDTB->MessBuffer(), pmbDTB->MessageBytes());
  WORD iNodeFrom;
  // Извлечь подкоманду из DTB, определить iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
  case ScC_ATR:   //Add in Transaction Register
      // Актуализировать в узле (tCmd._szNode) (tCmd._szBlock) блоков 
      //регистра транзакций
      b = UpdateTransactionRegister(tCmd.GetNode(), tCmd.GetBlockNo());
      break;
  case ScC_GTB:   //Get Transaction Register Block
      b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf); //ответ - TBL
      pmbReply = RequestNoReply(iNodeFrom, tMessBuf);
      break;
  case ScC_TAQ:   //TransAction reQuest
      b = TransActionreQuest(tCmd, tMessBuf);              //ответ - TAR
      pmbReply = RequestNoReply(iNodeFrom, tMessBuf);
      break;
  case ScC_TAR:   //TransAction request Replay
      b = AddInVotingTable(iNodeFrom, &tMessBuf);   //занести в таблицу голос
      break;
  case ScC_TBL:   //Transaction BLock
      b = AddTransactionBlock(tMessBuf);    //добавить в регистр транзакций
      break;
  case ScC_TRA:   //execute TRansAction
      b = ExecuteMyTransact(tMessBuf);
  case 0:         //это не команда - ответ ACK
      break;
/*  NoWrap:
      swprintf_s(pszMessg, CHARBUFSIZE, _T("ACK %02d%02d"),
          m_paNode->m_iOwnNode, iNodeFrom);
      return true; */
  } //switch
/*  m_paNode->WrapData(iNodeFrom, tMessBuf);      //упаковать сообщ в DTB
  // Копирование сообщения во внешний буфер
  memcpy(pszMessg, tMessBuf.MessBuffer(), tMessBuf.MessageBytes());
  pszMessg[tMessBuf.MessageBytes()] = _T('\0'); //на всякий случай */
  return b;
} // CBlockChain::ProcessDTB()

// Инициировать транзакцию
bool  CBlockChain::Transaction(WORD iUserTo, double rAmount)
//iUserTo - индекс пользователя "кому"
//rAmount - сумма транзакции
{
  // Проверить параметры транзакции
  ASSERT(0 <= iUserTo && iUserTo < NODECOUNT);
  TBalance *ptBalance = m_rBalances.GetAt(iUserTo);
  double rBalance = ptBalance->GetBalance();
  if (rAmount > rBalance) {
    m_rBalances.SetError(7);    //сумма транзакции превышает остаток
    return false;
  }
  if (!m_paNode->IsQuorumPresent())  return false;
  // Сформировать подкоманду TAQ "Запросить транзакцию"
  WORD nTrans = m_rTransacts.GetNewTransactOrd(); //номер новой транзакции
  WORD iUserFrom = m_rUsers.OwnerIndex();         //индекс владельца узла
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CreateTransactSubcommand(szMessBuf, _T("TAQ"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  TMessageBuf mbRequest, *pmbTRA = NULL;
  WORD iNodeTo;  bool b;
  // Цикл голосования - разослать запросы TAQ всем узлам сети (включая свой)
  for (b = m_paNode->GetFirstNode(iNodeTo, true);
       b; b = m_paNode->GetNextNode(iNodeTo, true)) {
    mbRequest.PutMessage(szMessBuf);    //занести в буфер
    if (iNodeTo == m_paNode->m_iOwnNode) {
      pmbTRA = EmulateReply(mbRequest);
      b = AddInVotingTable(iNodeTo, pmbTRA);
    }
    else
      pmbTRA = RequestNoReply(iNodeTo, mbRequest); //портит mbRequest!
    if (pmbTRA) {
      // Ответ получен - занести в таблицу голосования
	  delete pmbTRA;  pmbTRA = NULL;
    }
  }
  return true;
} // CBlockChain::Transaction()

// Проанализировать результаты голосования
bool  CBlockChain::AnalyzeVoting()
{
  return true;
} // CBlockChain::AnalyzeVoting()

// Дополнить регистр недостающими транзакциями из актуальных узлов
void  CBlockChain::ActualizeTransactReg()
{
  //return true;
} // CBlockChain::ActualizeTransactReg()

// Перегруженный метод. Провести одобренную транзакцию
void  CBlockChain::ExecuteTransaction(TMessageBuf &mbTAQ)
//mbTAQ - буфер с подкомандой TAQ
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  // Скопировать подкоманду в буфер для изменения
  int len = min(_tcslen(mbTAQ.Message()), ERMES_BUFSIZE-1);
  memcpy(szMessBuf, mbTAQ.Message(), len * sizeof(TCHAR));
  memset(szMessBuf+len, 0, (ERMES_BUFSIZE-len) * sizeof(TCHAR));
  // Преобразовать на месте подкоманду TAQ "Запросить транзакцию" в 
  //подкоманду TRA "Выполнить транзакцию"
  memcpy(szMessBuf, _T("TRA"), 3 * sizeof(TCHAR));
  bool b;  WORD iNodeTo;  TMessageBuf mbRequest, *pmbReply;
  // Провести транзакцию на удалённых узлах сети
  for (b = m_paNode->GetFirstNode(iNodeTo);
       b; b = m_paNode->GetNextNode(iNodeTo)) {
    mbRequest.PutMessage(szMessBuf);    //занести в буфер
    pmbReply = RequestNoReply(iNodeTo, mbRequest);
    if (pmbReply) {
      // Ответ получен - проверить
      b = (pmbReply->Compare(_T("ACK"), 3) == 0);   //ответ д.б. ACK
      delete pmbReply;  pmbReply = NULL;
    }
  }
  // Провести транзакцию на своём узле
  mbRequest.PutMessage(szMessBuf);  //занести в буфер
  b = ExecuteMyTransact(mbRequest);
} // CBlockChain::ExecuteTransaction()

// Провести транзакцию на своём узле
bool  CBlockChain::ExecuteMyTransact(TMessageBuf &mbTRA)
//iUserFrom - индекс пользователя-отправителя
//iUserTo   - индекс пользователя-получателя
//rAmount   - сумма транзакции
{
  TSubcmd tCmd;
  if (ParseMessage(mbTRA, tCmd) != ScC_TRA)  return false;
  // Извлечь параметры транзакции
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
  m_rTransacts.Add(ptRansact);   //регистр транзакций
  m_pMainWin->AddInTransactTable(ptRansact);
  return true;
} // CBlockChain::ExecuteMyTransact()

//private:
// Внутренние методы
// -------------------------------------------------------------------
/* Инициализировать все реестры в памяти
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

// Сформировать подкоманду запроса, ответа или исполнения транзакции
void  CBlockChain::CreateTransactSubcommand(
  LPTSTR pszMsg, LPTSTR pszSbcmd, WORD nTrans,
  WORD iUserFrom, WORD iUserTo, double rAmount)
{
  // Сформировать подкоманду транзакции ОТЛАДОЧНЫЙ ВАРИАНТ
  //                                    ==================
  swprintf_s(pszMsg, ERMES_BUFSIZE, _T("%s %08d%02d%02d%016.2f"),
             pszSbcmd, nTrans, iUserFrom, iUserTo, rAmount); //сформировать
} // CBlockChain::CreateTransactSubcommand()

// Послать блок данных серверу заданного именованного канала и получить ответ
TMessageBuf * CBlockChain::RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest)
//iNode     - индекс узла-адресата
//mbRequest - буфер запроса
{
  TMessageBuf *pmbReply = NULL;
  WORD iNodeFrom;
  m_paNode->WrapData(iNodeTo, mbRequest);   //поместить подкоманду в DTB
  // Возможно, потребуется несколько попыток для обмена сообщениями
  //for (WORD i = 0; i < 3; i++) { //max number of attempts = 3
    pmbReply = m_paNode->RequestAndReply(iNodeTo, mbRequest);
    if (pmbReply) {   //ответ получен! д.б. ACK
      m_paNode->UnwrapData(iNodeFrom, pmbReply); //извлечь подкоманду из DTB
      ASSERT(iNodeFrom == iNodeTo);
      delete pmbReply;
    }
  //}
  return NULL;
} // RequestNoReply()

// Эмулировать ответ своего узла
TMessageBuf * CBlockChain::EmulateReply(TMessageBuf &mbRequest)
{
  // Сформировать подкоманду TAR "Выполнить транзакцию" на месте 
  //подкоманды TAQ
  TMessageBuf *pmbReply = new TMessageBuf(mbRequest.Message());
  memcpy(pmbReply->Message(), _T("TAR"), 3 * sizeof(TCHAR));
  return pmbReply;
} // CBlockChain::EmulateReply()

// Разобрать сообщение. Метод возвращает код подкоманды и описатель сообщ.
ESubcmdCode  CBlockChain::ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd)
//mbMess[in] - сообщение
//tCmd[out]  - результат разбора, описатель сообщения
{
  static LPTSTR sCmds[] = {
//Индекс             Код подкоманды
/* 0 */ _T("ATR"),  //ScC_ATR = 11, Add in Transact Reg - Добавить в журнал та
/* 1 */ _T("GTB"),  //ScC_GTB,      Get Trans log Block - Дать блок журнала та
/* 2 */ _T("TAQ"),  //ScC_TAQ,      TransAction reQuest - Запрос транзакции
/* 3 */ _T("TAR"),  //ScC_TAR,      TransAction Reply   - Ответ на запрос тран
/* 4 */ _T("TBL"),  //ScC_TBL,      Transact log BLock  - Блок транзакции
/* 5 */ _T("TRA"),  //ScC_TRA,      execute TRansAction - Выполнить транзакцию
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
  // Подкоманда определена, сформировть описатель
  switch (tCmd._eCode) {
case ScC_ATR:   //Add in Transaction Register
    // Номер (индекс) узла
    tCmd.SetNode(mbMess.Message() + CMDFIELDSIZ);
    // Количество блоков
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ+SHORTNUMSIZ);
    break;
case ScC_GTB:   //Get Transaction Register Block
    // Номер блока от конца цепи
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ);
    break;
case ScC_TAQ:   //TransAction reQuest
case ScC_TAR:   //TransAction Reply
case ScC_TRA:   //execute TRansAction
    // Номер транзакции
    tCmd.SetTransNo(mbMess.Message() + CMDFIELDSIZ);
    // Хеш-код отправителя
    tCmd.SetHashFrom(mbMess.MessBuffer() +
                     (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR));
    // Хеш-код получателя
    tCmd.SetHashTo(mbMess.MessBuffer() +
                   (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+_HASHSIZE-2);
    // Сумма транзакции
    tCmd.SetAmount(mbMess.Message() +
                   CMDFIELDSIZ+LONGNUMSIZE+2*(_HASHSIZE/sizeof(TCHAR)-1));
    break;
case ScC_TBL:   //Transact register BLock
    // Хеш-код предыдущего блока транзакции
    tCmd.SetHashPrev(mbMess.MessBuffer() + CMDFIELDSIZ*sizeof(TCHAR));
    // Номер транзакции
    tCmd.SetTransNo(mbMess.Message() + CMDFIELDSIZ+_HASHSIZE/sizeof(TCHAR)-1);
    // Хеш-код отправителя
    tCmd.SetHashFrom(mbMess.MessBuffer() +
                     (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+_HASHSIZE-2);
    // Хеш-код получателя
    tCmd.SetHashTo(mbMess.MessBuffer() +
                   (CMDFIELDSIZ+LONGNUMSIZE)*sizeof(TCHAR)+2*_HASHSIZE-4);
    // Сумма транзакции
    tCmd.SetAmount(mbMess.Message() +
                   CMDFIELDSIZ+LONGNUMSIZE+3*(_HASHSIZE/sizeof(TCHAR)-1));
    break;
  } //switch
  return tCmd._eCode;   //подкоманда не определена
} // CBlockChain::ParseMessage()

// Поместить ответ в таблицу голосования
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
  sNode.Format(_T("Узел #%d"), iNodeFrom);
  pszFields[0] = sNode.GetBuffer();     //номер узла
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUser, szOwner);
  pszFields[1] = szOwner;               //имя владельца узла
  pszFields[2] = tCmd._szTrans;         //номер транзакции
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserFrom, szUserFrom);
  pszFields[3] = szUserFrom;            //имя пользователя-отправителя
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserTo, szUserTo);
  pszFields[4] = szUserTo;              //имя пользователя-получателя
  sAmount.Format(_T("%.2lf"), m_tVoteRes[iNodeFrom]._rSum);
  pszFields[5] = sAmount.GetBuffer();   //сумма
  bool b = m_pMainWin->AddInVotingTable(pszFields);
  if (b) {
    m_nVote++;
    if (IsTheVoteOver()) {
      // Проанализировать результаты голосования
      if (!AnalyzeVoting()) {
        // Транзакция отклонена: номер транзакции неверен - дополнить наш
        //регистр недостающими транзакциями и пересчитать текущий остаток
        ActualizeTransactReg();
      }
      // Провести одобренную транзакцию
      //mbRequest.PutMessage(szMessBuf);      //занести в буфер
      ExecuteTransaction(*pmbTRA);
    }
  }
  return b;
} // CBlockChain::AddInVotingTable()

// Актуализировать свой регистр транзакций
bool  CBlockChain::UpdateTransactionRegister(WORD iNodeFrom, WORD nBlocks)
//iNodeFrom - номер (индекс) узла, у которого нужно запросить блоки транзакций
//nBlocks   - количество недостающих последних блоков транзакций
{
  return true;
} // CBlockChain::UpdateTransactionRegister()

// Обработать сообщение GTB "Дать блок регистра транзакций".
//В ответ формируется сообщение TBL.
bool  CBlockChain::GetTransactionBlock(WORD nBlock, TMessageBuf &mbReply)
//nBlock[in]      - номер блока от конца цепи (1,2,..)
//mbReply[in/out] - буфер сообщения-ответа
{
  TCHAR chMess[CHARBUFSIZE];  //буфер сообщения (TCHAR=wchar_t)
  WORD n = m_rTransacts.ItemCount();
  //ASSERT(0 < nBlock && nBlock < n);
  TTransact *ptRansact = m_rTransacts.GetAt(n - nBlock);
  _tcscpy_s(chMess, CHARBUFSIZE-1, _T("TBL "));
  memcpy(chMess + 4, ptRansact, sizeof(TTransact));
  mbReply.Put((BYTE *)chMess, sizeof(TCHAR) * 4 + sizeof(TTransact));
  return true;
} // CBlockChain::GetTransactionBlock()

// Обработать сообщение TAQ "Запрос транзакции".
//В ответ формируется сообщение TAR.
bool  CBlockChain::TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbReply)
//tCmd[out]       - результат разбора, описатель сообщения TAQ
//mbReply[in/out] - ответ
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

// Добавить в регистр транзакций новый блок (при актуализации).
bool  CBlockChain::AddTransactionBlock(TMessageBuf& mbDTB)
{
  return true;
} // CBlockChain::AddTransactionBlock()

// End of class CBalanceRegister definition ----------------------------
