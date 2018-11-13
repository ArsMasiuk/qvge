/* $Id: CoinMessageHandler.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinMessageHandler.hpp"
#include "CoinHelperFunctions.hpp"
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <map>

/* Default constructor. */
CoinOneMessage::CoinOneMessage()
{
  externalNumber_=-1;
  message_[0]='\0';
  severity_='I';
  detail_=0;
}
/* Destructor */
CoinOneMessage::~CoinOneMessage()
{
}
/* The copy constructor */
CoinOneMessage::CoinOneMessage(const CoinOneMessage & rhs)
{
  externalNumber_=rhs.externalNumber_;
  strcpy(message_,rhs.message_);
  severity_=rhs.severity_;
  detail_=rhs.detail_;
}
/* assignment operator. */
CoinOneMessage&
CoinOneMessage::operator=(const CoinOneMessage & rhs)
{
  if (this != &rhs) {
    externalNumber_=rhs.externalNumber_;
    strcpy(message_,rhs.message_);
    severity_=rhs.severity_;
    detail_=rhs.detail_;
  }
  return *this;
}
/* Normal constructor */
CoinOneMessage::CoinOneMessage(int externalNumber, char detail,
		const char * message)
{
  externalNumber_=externalNumber;
  strcpy(message_,message);
  if (externalNumber<3000)
    severity_='I';
  else if (externalNumber<6000)
    severity_='W';
  else if (externalNumber<9000)
    severity_='E';
  else
    severity_='S';
  detail_=detail;
}
// Replaces messages (i.e. a different language)
void
CoinOneMessage::replaceMessage( const char * message)
{
  strcpy(message_,message);
}


/* Constructor with number of messages. */
CoinMessages::CoinMessages(int numberMessages)
{
  numberMessages_=numberMessages;
  language_=us_en;
  strcpy(source_,"Unk");
  class_=1;
  lengthMessages_=-1;
  if (numberMessages_) {
    message_ = new CoinOneMessage * [numberMessages_];
    int i;
    for (i=0;i<numberMessages_;i++)
      message_[i]=NULL;
  } else {
    message_=NULL;
  }
}
/* Destructor */
CoinMessages::~CoinMessages()
{
  int i;
  if (lengthMessages_<0) {
    for (i=0;i<numberMessages_;i++)
      delete message_[i];
  }
  delete [] message_;
}
/* The copy constructor */
CoinMessages::CoinMessages(const CoinMessages & rhs)
{
  numberMessages_=rhs.numberMessages_;
  language_=rhs.language_;
  strcpy(source_,rhs.source_);
  class_=rhs.class_;
  lengthMessages_=rhs.lengthMessages_;
  if (lengthMessages_<0) {
    if (numberMessages_) {
      message_ = new CoinOneMessage * [numberMessages_];
      int i;
      for (i=0;i<numberMessages_;i++)
	if (rhs.message_[i])
	  message_[i]=new CoinOneMessage(*(rhs.message_[i]));
	else
	  message_[i] = NULL;
    } else {
      message_=NULL;
    }
  } else {
    char * temp = CoinCopyOfArray(reinterpret_cast<char *> (rhs.message_),lengthMessages_);
    message_ = reinterpret_cast<CoinOneMessage **> (temp);
    std::ptrdiff_t offset = temp - reinterpret_cast<char *> (rhs.message_);
    int i;
    //printf("new address %x(%x), rhs %x - length %d\n",message_,temp,rhs.message_,lengthMessages_);
    for (i=0;i<numberMessages_;i++) {
      if (message_[i]) {
	char * newAddress = (reinterpret_cast<char *> (message_[i])) + offset;
	assert (newAddress-temp<lengthMessages_);
	message_[i] = reinterpret_cast<CoinOneMessage *> (newAddress);
	//printf("message %d at %x is %s\n",i,message_[i],message_[i]->message());
	//printf("message %d at %x wass %s\n",i,rhs.message_[i],rhs.message_[i]->message());
      }
    }
  }
}
/* assignment operator. */
CoinMessages&
CoinMessages::operator=(const CoinMessages & rhs)
{
  if (this != &rhs) {
    language_=rhs.language_;
    strcpy(source_,rhs.source_);
    class_=rhs.class_;
    if (lengthMessages_<0) {
      int i;
      for (i=0;i<numberMessages_;i++)
	delete message_[i];
    }
    delete [] message_;
    numberMessages_=rhs.numberMessages_;
    lengthMessages_=rhs.lengthMessages_;
    if (lengthMessages_<0) {
      if (numberMessages_) {
	message_ = new CoinOneMessage * [numberMessages_];
	int i;
	for (i=0;i<numberMessages_;i++)
	  if (rhs.message_[i])
	    message_[i]=new CoinOneMessage(*(rhs.message_[i]));
	  else
	    message_[i] = NULL;
      } else {
	message_=NULL;
      }
    } else {
      char * temp = CoinCopyOfArray(reinterpret_cast<char *> (rhs.message_),lengthMessages_);
      message_ = reinterpret_cast<CoinOneMessage **> (temp);
      std::ptrdiff_t offset = temp - reinterpret_cast<char *> (rhs.message_);
      int i;
      //printf("new address %x(%x), rhs %x - length %d\n",message_,temp,rhs.message_,lengthMessages_);
      for (i=0;i<numberMessages_;i++) {
	if (message_[i]) {
	  char * newAddress = (reinterpret_cast<char *> (message_[i])) + offset;
	  assert (newAddress-temp<lengthMessages_);
	  message_[i] = reinterpret_cast<CoinOneMessage *> (newAddress);
	  //printf("message %d at %x is %s\n",i,message_[i],message_[i]->message());
	  //printf("message %d at %x wass %s\n",i,rhs.message_[i],rhs.message_[i]->message());
	}
      }
    }
  }
  return *this;
}
// Puts message in correct place
void
CoinMessages::addMessage(int messageNumber, const CoinOneMessage & message)
{
  if (messageNumber>=numberMessages_) {
    // should not happen but allow for it
    CoinOneMessage ** temp = new CoinOneMessage * [messageNumber+1];
    int i;
    for (i=0;i<numberMessages_;i++)
      temp[i] = message_[i];
    for (;i<=messageNumber;i++)
      temp[i] = NULL;
    delete [] message_;
    message_ = temp;
  }
  if (lengthMessages_>=0)
    fromCompact();
  delete message_[messageNumber];
  message_[messageNumber]=new CoinOneMessage(message);
}
// Replaces messages (i.e. a different language)
void
CoinMessages::replaceMessage(int messageNumber,
			     const char * message)
{
  if (lengthMessages_>=0)
    fromCompact();
  assert(messageNumber<numberMessages_);
  message_[messageNumber]->replaceMessage(message);
}
// Changes detail level for one message
void
CoinMessages::setDetailMessage(int newLevel, int messageNumber)
{
  int i;
  // Last message is null (corresponds to DUMMY)
  for (i=0;i<numberMessages_-1;i++) {
    if (message_[i]->externalNumber()==messageNumber) {
      message_[i]->setDetail(newLevel);
      break;
    }
  }
}
// Changes detail level for several messages
void
CoinMessages::setDetailMessages(int newLevel, int numberMessages,
			       int * messageNumbers)
{
  int i;
  if (numberMessages<3&&messageNumbers) {
    // do one by one
    int j;
    for (j=0;j<numberMessages;j++) {
      int messageNumber = messageNumbers[j];
      for (i=0;i<numberMessages_;i++) {
	if (message_[i]->externalNumber()==messageNumber) {
	  message_[i]->setDetail(newLevel);
	  break;
	}
      }
    }
  } else if (numberMessages<10000&&messageNumbers) {
    // do backward mapping
    int backward[10000];
    for (i=0;i<10000;i++)
      backward[i]=-1;
    for (i=0;i<numberMessages_;i++)
      backward[message_[i]->externalNumber()]=i;
    for (i=0;i<numberMessages;i++) {
      int iback = backward[messageNumbers[i]];
      if (iback>=0)
	message_[iback]->setDetail(newLevel);
    }
  } else {
    // do all (except for dummy end)
    for (i=0;i<numberMessages_-1;i++) {
      message_[i]->setDetail(newLevel);
    }
  }
}

// Changes detail level for all messages >= low and < high
void
CoinMessages::setDetailMessages(int newLevel, int low, int high)
{
  // do all (except for dummy end) if in range
  for (int i=0;i<numberMessages_-1;i++) {
    int iNumber = message_[i]->externalNumber();
    if (iNumber>=low&&iNumber<high)
      message_[i]->setDetail(newLevel);
  }
}
/*
  Moves to compact format

  Compact format is an initial array of CoinOneMessage pointers, followed by a
  bulk store that holds compressed CoinOneMessage objects, where the
  message_ array is truncated to be just as large as necessary.
*/
void
CoinMessages::toCompact()
{
  if (numberMessages_&&lengthMessages_<0) {
    lengthMessages_=numberMessages_*CoinSizeofAsInt(CoinOneMessage *);
    int i;
    for (i=0;i<numberMessages_;i++) {
      if (message_[i]) {
	int length = static_cast<int>(strlen(message_[i]->message()));
	length = static_cast<int>((message_[i]->message()+length+1)-
				      reinterpret_cast<char *> (message_[i]));
	assert (length<COIN_MESSAGE_HANDLER_MAX_BUFFER_SIZE);
	int leftOver = length %8;
	if (leftOver)
	  length += 8-leftOver;
	lengthMessages_+=length;
      }
    }
    // space
    char * temp = new char [lengthMessages_];
    CoinOneMessage ** newMessage = reinterpret_cast<CoinOneMessage **> (temp);
    temp += numberMessages_*CoinSizeofAsInt(CoinOneMessage *);
    CoinOneMessage message;
    //printf("new address %x(%x) - length %d\n",newMessage,temp,lengthMessages_);
    lengthMessages_=numberMessages_*CoinSizeofAsInt(CoinOneMessage *);
    for (i=0;i<numberMessages_;i++) {
      if (message_[i]) {
	message = *message_[i];
	int length = static_cast<int>(strlen(message.message()));
	length = static_cast<int>((message.message()+length+1)-
				  reinterpret_cast<char *> (&message));
	assert (length<COIN_MESSAGE_HANDLER_MAX_BUFFER_SIZE);
	int leftOver = length %8;
	memcpy(temp,&message,length);
	newMessage[i]=reinterpret_cast<CoinOneMessage *> (temp);
	//printf("message %d at %x is %s\n",i,newMessage[i],newMessage[i]->message());
	if (leftOver)
	  length += 8-leftOver;
	temp += length;
	lengthMessages_+=length;
      } else {
	// null message
	newMessage[i]=NULL;
      }
    }
    for (i=0;i<numberMessages_;i++)
      delete message_[i];
    delete [] message_;
    message_ = newMessage;
  }
}
// Moves from compact format
void
CoinMessages::fromCompact()
{
  if (numberMessages_&&lengthMessages_>=0) {
    CoinOneMessage ** temp = new CoinOneMessage * [numberMessages_];
    int i;
    for (i=0;i<numberMessages_;i++) {
      if (message_[i])
	temp[i]=new CoinOneMessage(*(message_[i]));
      else
	temp[i]=NULL;
    }
    delete [] message_;
    message_ = temp;
  }
  lengthMessages_=-1;
}

// Clean, print message and check severity, return 0 normally
int
CoinMessageHandler::internalPrint()
{
  int returnCode=0;
  if (messageOut_>messageBuffer_) {
    *messageOut_=0;
    //take off trailing spaces and commas
    messageOut_--;
    while (messageOut_>=messageBuffer_) {
      if (*messageOut_==' '||*messageOut_==',') {
	*messageOut_=0;
	messageOut_--;
      } else {
	break;
      }
    }
    // Now do print which can be overridden
    returnCode=print();
    // See what to do on error
    checkSeverity();
  }
  return returnCode;
}
// Print message, return 0 normally
int
CoinMessageHandler::print()
{
#ifndef NDEBUG
  fprintf(fp_,"%s\n",messageBuffer_);
#endif
  return 0;
}
// Check severity
void
CoinMessageHandler::checkSeverity()
{
  if (currentMessage_.severity_=='S') {
    fprintf(fp_,"Stopping due to previous errors.\n");
    //Should do walkback
    abort();
  }
}
/* Amount of print out:
   0 - none
   1 - minimal
   2 - normal low
   3 - normal high
   4 - verbose
   above that 8,16,32 etc just for selective debug and are for
   printf messages in code
*/
void
CoinMessageHandler::setLogLevel(int value)
{
  if (value>=-1)
    logLevel_=value;
}
void
CoinMessageHandler::setLogLevel(int which,int value)
{
  if (which>=0&&which<COIN_NUM_LOG) {
    if (value>=-1)
      logLevels_[which]=value;
  }
}
void
CoinMessageHandler::setPrecision(unsigned int new_precision) {

  char new_string[8] = {'%','.','8','f','\0','\0','\0','\0'};

  //we assume that the precision is smaller than one thousand
  new_precision = std::min<unsigned int>(999,new_precision);
  if (new_precision == 0)
    new_precision = 1;
  g_precision_ = new_precision ;
  int idx = 2;
  int base = 100;
  bool print = false;
  while (base > 0) {

    char c = static_cast<char>(new_precision / base);
    new_precision = new_precision % base;
    if (c != 0)
      print = true;
    if (print) {
      new_string[idx] = static_cast<char>(c +  '0');
      idx++;
    }
    base /= 10;
  }
  new_string[idx] = 'g';
  strcpy(g_format_,new_string);
}
void
CoinMessageHandler::setPrefix(bool value)
{
  if (value)
    prefix_ = 255;
  else
    prefix_ =0;
}
bool
CoinMessageHandler::prefix() const
{
  return (prefix_!=0);
}
// Constructor
CoinMessageHandler::CoinMessageHandler() :
  logLevel_(1),
  prefix_(255),
  currentMessage_(),
  internalNumber_(0),
  format_(NULL),
  printStatus_(0),
  highestNumber_(-1),
  fp_(stdout)
{
  const char* g_default = "%.8g";

  strcpy(g_format_,g_default);
  g_precision_ = 8 ;

  for (int i=0;i<COIN_NUM_LOG;i++)
    logLevels_[i]=-1000;
  messageBuffer_[0]='\0';
  messageOut_ = messageBuffer_;
  source_="Unk";
}
// Constructor
CoinMessageHandler::CoinMessageHandler(FILE * fp) :
  logLevel_(1),
  prefix_(255),
  currentMessage_(),
  internalNumber_(0),
  format_(NULL),
  printStatus_(0),
  highestNumber_(-1),
  fp_(fp)
{
  const char* g_default = "%.8g";

  strcpy(g_format_,g_default);
  g_precision_ = 8 ;

  for (int i=0;i<COIN_NUM_LOG;i++)
    logLevels_[i]=-1000;
  messageBuffer_[0]='\0';
  messageOut_ = messageBuffer_;
  source_="Unk";
}
/* Destructor */
CoinMessageHandler::~CoinMessageHandler()
{
}

void
CoinMessageHandler::gutsOfCopy(const CoinMessageHandler& rhs)
{
  logLevel_=rhs.logLevel_;
  prefix_ = rhs.prefix_;
  if (rhs.format_ && *rhs.format_ == '\0')
  { *rhs.format_ = '%' ;
    currentMessage_=rhs.currentMessage_;
    *rhs.format_ = '\0' ; }
  else
  { currentMessage_=rhs.currentMessage_; }
  internalNumber_=rhs.internalNumber_;
  int i;
  for ( i=0;i<COIN_NUM_LOG;i++)
    logLevels_[i]=rhs.logLevels_[i];
  doubleValue_=rhs.doubleValue_;
  longValue_=rhs.longValue_;
  charValue_=rhs.charValue_;
  stringValue_=rhs.stringValue_;
  std::ptrdiff_t offset ;
  if (rhs.format_)
  { offset = rhs.format_ - rhs.currentMessage_.message();
    format_ = currentMessage_.message()+offset; }
  else
  { format_ = NULL ; }
  std::memcpy(messageBuffer_,rhs.messageBuffer_,
	      COIN_MESSAGE_HANDLER_MAX_BUFFER_SIZE);
  offset = rhs.messageOut_-rhs.messageBuffer_;
  messageOut_= messageBuffer_+offset;
  printStatus_= rhs.printStatus_;
  highestNumber_= rhs.highestNumber_;
  fp_ = rhs.fp_;
  source_ = rhs.source_;
  strcpy(g_format_,rhs.g_format_);
  g_precision_ = rhs.g_precision_ ;
}
/* The copy constructor */
CoinMessageHandler::CoinMessageHandler(const CoinMessageHandler& rhs)
{
  gutsOfCopy(rhs);
}
/* assignment operator. */
CoinMessageHandler &
CoinMessageHandler::operator=(const CoinMessageHandler& rhs)
{
  if (this != &rhs) {
    gutsOfCopy(rhs);
  }
  return *this;
}
// Clone
CoinMessageHandler *
CoinMessageHandler::clone() const
{
  return new CoinMessageHandler(*this);
}
// Start a message
CoinMessageHandler &
CoinMessageHandler::message(int messageNumber,
			   const CoinMessages &normalMessage)
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    internalPrint();
  }
  internalNumber_=messageNumber;
  currentMessage_= *(normalMessage.message_[messageNumber]);
  source_ = normalMessage.source_;
  format_ = currentMessage_.message_;
  messageBuffer_[0]='\0';
  messageOut_=messageBuffer_;
  highestNumber_ = CoinMax(highestNumber_,currentMessage_.externalNumber_);
  // do we print
  int detail = currentMessage_.detail_;
  printStatus_=0;
  if (logLevels_[0]==-1000) {
    if (detail>=8&&logLevel_>=0) {
      // bit setting - debug
      if ((detail&logLevel_)==0)
	printStatus_ = 3;
    } else if (logLevel_<detail) {
      printStatus_ = 3;
    }
  } else if (logLevels_[normalMessage.class_]<detail) {
    printStatus_ = 3;
  }
  if (!printStatus_) {
    if (prefix_) {
      sprintf(messageOut_,"%s%4.4d%c ",source_.c_str(),
	      currentMessage_.externalNumber_,
	      currentMessage_.severity_);
      messageOut_ += strlen(messageOut_);
    }
    format_ = nextPerCent(format_,true);
  }
  return *this;
}
/* The following is to help existing codes interface
   Starts message giving number and complete text
*/
CoinMessageHandler &
CoinMessageHandler::message(int externalNumber,const char * source,
			   const char * msg, char severity)
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    internalPrint();
  }
  internalNumber_=externalNumber;
  currentMessage_= CoinOneMessage();
  currentMessage_.setExternalNumber(externalNumber);
  source_ = source;
  // mark so will not update buffer
  printStatus_=2;
  highestNumber_ = CoinMax(highestNumber_,externalNumber);
  // If we get here we always print
  if (prefix_) {
    sprintf(messageOut_,"%s%4.4d%c ",source_.c_str(),
	    externalNumber,
	    severity);
  }
  strcat(messageBuffer_,msg);
  messageOut_=messageBuffer_+strlen(messageBuffer_);
  return *this;
}
  /* Allows for skipping printing of part of message,
      but putting in data */
  CoinMessageHandler &
CoinMessageHandler::printing(bool onOff)
{
  // has no effect if skipping or whole message in
  if (printStatus_<2) {
    assert(format_[1]=='?');
    *format_ = '%' ;
    if (onOff)
      printStatus_=0;
    else
      printStatus_=1;
    format_ = nextPerCent(format_+2,true);
  }
  return *this;
}
/* Stop (and print)
*/
int
CoinMessageHandler::finish()
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    internalPrint();
  }
  internalNumber_=-1;
  format_ = NULL;
  messageBuffer_[0]='\0';
  messageOut_=messageBuffer_;
  printStatus_=0;
  doubleValue_.clear();
  longValue_.clear();
  charValue_.clear();
  stringValue_.clear();
  return 0;
}
/* Gets position of next field in format
   If we're scanning the initial portion of the string (prior to the first
   `%' code) the prefix will be copied to the output buffer. Normally, the
   text from the current position up to and including a % code is is processed
   by the relevant operator<< method.
*/
char *
CoinMessageHandler::nextPerCent(char * start , const bool initial)
{
  if (start) {
    bool foundNext=false;
    while (!foundNext) {
      char * nextPerCent = strchr(start,'%');
      if (nextPerCent) {
	if (initial&&!printStatus_) {
	  int numberToCopy=static_cast<int>(nextPerCent-start);
	  strncpy(messageOut_,start,numberToCopy);
	  messageOut_+=numberToCopy;
	}
	// %? is skipped over as it is just a separator
	if (nextPerCent[1]!='?') {
	  start=nextPerCent;
	  if (start[1]!='%') {
	    foundNext=true;
	    if (!initial)
	      *start='\0'; //zap
	  } else {
	    start+=2;
	    if (initial) {
	      *messageOut_='%';
	      messageOut_++;
	    }
	  }
	} else {
	  foundNext=true;
	  // skip to % and zap
	  start=nextPerCent;
	  *start='\0';
	}
      } else {
        if (initial&&!printStatus_) {
          strcpy(messageOut_,start);
          messageOut_+=strlen(messageOut_);
        }
        start=0;
        foundNext=true;
      }
    }
  }
  return start;
}
// Adds into message
CoinMessageHandler &
CoinMessageHandler::operator<< (int intvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_.push_back(intvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but may be changed to null)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,intvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %d",intvalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
CoinMessageHandler &
CoinMessageHandler::operator<< (double doublevalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  doubleValue_.push_back(doublevalue);

  if (printStatus_<2) {
    if (format_) {
      //format is at \0 (but changed to %)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	if (format_[1] == '.' && format_[2] >= '0' && format_[2] <= '9') {
	  // an explicitly specified precision currently overrides the
	  // precision of the message handler
	  sprintf(messageOut_,format_,doublevalue);
	}
	else {
	  sprintf(messageOut_,g_format_,doublevalue);
	  if (next != format_+2) {
	    messageOut_+=strlen(messageOut_);
	    sprintf(messageOut_,format_+2);
	  }
	}
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," ");
      messageOut_ += 1;
      sprintf(messageOut_,g_format_,doublevalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
#if COIN_BIG_INDEX==1
CoinMessageHandler &
CoinMessageHandler::operator<< (long longvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_.push_back(longvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but may be changed to null)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,longvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %ld",longvalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
#endif
#if COIN_BIG_INDEX==2
CoinMessageHandler &
CoinMessageHandler::operator<< (long long longvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_.push_back(longvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but may be changed to null)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,longvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %ld",longvalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
#endif
CoinMessageHandler &
CoinMessageHandler::operator<< (const std::string& stringvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  stringValue_.push_back(stringvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,stringvalue.c_str());
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %s",stringvalue.c_str());
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
CoinMessageHandler &
CoinMessageHandler::operator<< (char charvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  charValue_.push_back(charvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,charvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %c",charvalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
CoinMessageHandler &
CoinMessageHandler::operator<< (const char *stringvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  stringValue_.push_back(stringvalue);
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,stringvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %s",stringvalue);
      messageOut_+=strlen(messageOut_);
    }
  }
  return *this;
}
CoinMessageHandler &
CoinMessageHandler::operator<< (CoinMessageMarker marker)
{
  if (printStatus_!=3) {
    switch (marker) {
    case CoinMessageEol:
      finish();
      break;
    case CoinMessageNewline:
      strcat(messageOut_,"\n");
      messageOut_++;
      break;
    }
  } else {
    // skipping - tidy up
    format_ = NULL;
  }
  return *this;
}

// returns current
CoinMessageHandler &
CoinMessageHandler::message()
{
  return * this;
}
