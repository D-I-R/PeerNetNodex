// BlkChain.h                                              (c) DIR, 2019
//
//              Объявление классов модели блокчейн
//
// CBlockChain - Модель блокчейн
//
////////////////////////////////////////////////////////////////////////

#pragma once

//#include "afxtempl.h"
#include "FileBase.h"

#define  CMDFIELDSIZ  4     //размер поля команды
#define  SHORTNUMSIZ  2     //размер номера или количества блоков
#define  LONGNUMSIZE  8     //размер номера транзакции
#define  VOTECOLNUM   6     //количество столбцов в таблице голосования
#define  TRANCOLNUM   4     //количество столбцов в таблице транзакций
// Имя контейнера ключей шифрования (UNICODE):
#define  KEYCONTNAME  TEXT("Peer Net Key Container")

// Идентификаторы (коды) подкоманд команды DTB
enum ESubcmdCode {
  ScC_ATR = 11, //Add in Transaction Log - Нужно добавить в журнал транзакций
  ScC_GTB,      //Get Transaction log Block - Дать блок журнала транзакций
  ScC_TAQ,      //TransAction reQuest    - Запрос транзакции
  ScC_TAR,      //TransAction Reply      - Ответ на запрос транзакции
  ScC_TBL,      //Transaction log BLock  - Блок транзакции
  ScC_TRA,      //execute TRansAction    - Выполнить транзакцию
};
// Структурный тип "Описатель сообщения (подкоманды)"
struct TSubcmd {
  ESubcmdCode  _eCode;              //код подкоманды
  TCHAR  _szNode[SHORTNUMSIZ+1];    //номер узла (2)
  TCHAR  _szBlock[SHORTNUMSIZ+1];   //номер или количество блоков (2)
  TCHAR  _szTrans[LONGNUMSIZE+1];   //номер транзакции (8)
  BYTE   _chHashPrev[_HASHSIZE];     //хеш-код предыдущего блока
  BYTE   _chHashFrom[_HASHSIZE];     //хеш-код рег имени польз-отправителя
  BYTE   _chHashTo[_HASHSIZE];       //хеш-код рег имени польз-получателя
  TCHAR  _szAmount[NAMESBUFSIZ+1];  //сумма транзакции (или тек остатка)
  // Обнуление структуры
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

// Структурный тип "Запись таблицы результатов голосования"
struct TVotingRes {
  WORD    _iUser;       //владелец узла (индекс в реестре)
  WORD    _nTrans;      //номер новой транзакции
  WORD    _iUserFrom;   //пользователь "кто" (индекс в реестре)
  WORD    _iUserTo;     //пользователь "кому" (индекс в реестре)
  double  _rSum;        //сумма транзакции или текущий остаток счёта
  // Инициализировать структуру
  void  Init(WORD iUser) {
    memset(&_iUser, 0, sizeof(TVotingRes));
    _iUser = iUser;
  }
};

class CPeerNetDialog;
class CPeerNetNode;

// Объявление класса CBlockChain - Модель блокчейн
////////////////////////////////////////////////////////////////////////

class CBlockChain
{
  CPeerNetDialog  * m_pMainWin;     //главное окно программы
  CPeerNetNode    * m_paNode;       //узел одноранговой сети
  CCrypto           m_aCrypt;       //объект криптографии
  CUserRegister     m_rUsers;       //регистр пользователей
  CBalanceRegister  m_rBalances;    //регистр текущих остатков
  CTransRegister    m_rTransacts;   //регистр транзакций
  // Таблица соответствия iNode->iUser, а также таблица голосования узлов:
  TVotingRes  m_tVoteRes[NODECOUNT];
  WORD        m_nVote;              //количество проголосовавших узлов
  WORD        m_iNodes[NODECOUNT];      //таблица соответствия iUser->iNode
  WORD        m_iActNodes[NODECOUNT];   //список "актуальных" узлов сети
  WORD        m_nActNodes;      //количество "актуальных" узлов в списке
  WORD        m_iActNode;       //текущий элемент списка "актуальных" узлов
public:                         // (при переборе)
  // Конструктор и деструктор
  CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode);
  ~CBlockChain(void);
  // Внешние методы
  // -------------------------------------------------------------------
  // Дать указатель регистра пользователей
  CUserRegister    * UserRegister() { return &m_rUsers; }
  // Дать указатель регистра текущих остатков
  CBalanceRegister * BalanceRegister() { return &m_rBalances; }
  // Дать указатель регистра транзакций
  CTransRegister   * TransRegister() { return &m_rTransacts; }
  // Проверить регистрационные данные пользователя
  WORD  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd);
  WORD  AuthorizationCheck(CString sLogin, CString sPasswd);
  // Авторизовать пользователя
  bool  Authorize(WORD iUser);
  // Дать авторизованного пользователя (владельца узла)
  WORD  GetAuthorizedUser() { return m_rUsers.OwnerIndex(); }
  // Отменить авторизацию пользователя
  void  CancelAuthorization();
  // Связать пользователя с узлом сети
  void  TieUpUserAndNode(WORD iUser, WORD iNode);
  // Отвязать пользователя от узла сети
  void  UntieUserAndNode(WORD iNode);
  // Дать пользователя заданного узла
  WORD  GetNodeOwner(WORD iNode) { return m_tVoteRes[iNode]._iUser; }
  // Сформировать ответ на сообщение DTB, полученное серверным каналом
  bool  GetReplyOnDTB(TMessageBuf &mbDTB, LPTSTR pszMessg);
  // Обработать сообщение DTB из очереди (полученное серверным каналом)
  bool  ProcessDTB();
  // Запросить и провести транзакцию
  bool  Transaction(WORD iUserTo, double rAmount);
  // Голосование закончено?
  bool  IsTheVoteOver() { return (m_nVote == m_paNode->m_nNodes); }
  // Проанализировать результаты голосования
  bool  AnalyzeVoting();
  // Дополнить регистр недостающими транзакциями из актуальных узлов
  void  ActualizeTransactReg();
  // Провести одобренную транзакцию
  void  ExecuteTransaction(TMessageBuf &mbTAQ);
  // Провести транзакцию на своём узле
  bool  ExecuteMyTransact(TMessageBuf &mbTRA);
private:
  // Внутренние методы
  // -------------------------------------------------------------------
  // Инициализировать все реестры в памяти
  //bool  Initialize();
  // Сформировать подкоманду запроса или исполнения транзакции
  void  CreateTransactSubcommand(LPTSTR pszMsg, LPTSTR pszSbcmd,
                                 WORD nTrans, WORD iUserFrom,
                                 WORD iUserTo, double rAmount);
  // Послать блок данных серверу заданного именованного канала и получить ответ
  TMessageBuf * RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // Эмулировать ответ своего узла
  TMessageBuf * EmulateReply(TMessageBuf &mbRequest);
  // Разобрать сообщение. Метод возвращает код подкоманды и описатель сообщ.
  ESubcmdCode  ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd);
  // Поместить ответ в таблицу голосования
  bool  AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbReply);
  // Актуализировать свой регистр транзакций
  bool  UpdateTransactionRegister(WORD iNodeFrom, WORD nBlocks);
  // Обработать сообщение GTB "Дать блок регистра транзакций".
  //В ответ формируется сообщение TBL.
  bool  GetTransactionBlock(WORD nBlock, TMessageBuf &mbReply);
  // Обработать сообщение TAQ "Запрос транзакции".
  //В ответ формируется сообщение TAR.
  bool  TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbReply);
  // Добавить в регистр транзакций новый блок (при актуализации).
  bool  AddTransactionBlock(TMessageBuf &mbDTB);
};

// End of class CBlockChain declaration --------------------------------
