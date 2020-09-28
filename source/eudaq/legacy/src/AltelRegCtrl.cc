#include "JadeRegCtrl.hh"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>



#define MAX_LINE_LENGTH 1024
#define MAX_PARAM_LENGTH 20
#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0

#define RBCP_DISP_MODE_NO 0
#define RBCP_DISP_MODE_INTERACTIVE 1
#define RBCP_DISP_MODE_DEBUG 2

struct rbcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <thread>
#include <algorithm>
#include <sstream>

using namespace std::chrono_literals;
 
//+++++++++++++++++++++++++++++++++++++++++
//AltelRegCtrl.hh

class AltelRegCtrl: public JadeRegCtrl{
public:
  AltelRegCtrl(const JadeOption &opt);
  ~AltelRegCtrl() override {};
  void Open() override;
  void Close() override;
  std::string SendCommand(const std::string &cmd, const std::string &para) override;
  void WriteByte(uint64_t addr, uint8_t val) override;
  uint8_t ReadByte(uint64_t addr) override;

  
  void ChipID(uint8_t id);
  void WriteReg(uint16_t addr, uint16_t val);
  uint16_t ReadReg(uint16_t addr);
  void Broadcast(uint8_t opcode);
  void SetFrameDuration(uint16_t len);
  void SetFramePhase(uint16_t len);
  void SetInTrigGap(uint16_t len);
  void SetFPGAMode(uint8_t mode);
  void SetFrameNumber(uint8_t n);
  void InitALPIDE();
  void StartPLL();
  void StartWorking(uint8_t trigmode);
  void ResetDAQ();
  
    
  int rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode);  
private:
  JadeOption m_opt;
  
  std::string m_ip_address;
  uint16_t m_ip_udp_port;
  uint8_t m_id ;
};

//+++++++++++++++++++++++++++++++++++++++++
//AltelRegCtrl.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(AltelRegCtrl)));
  auto _test_ = JadeFactory<JadeRegCtrl>::Register<AltelRegCtrl, const JadeOption&>(typeid(AltelRegCtrl));
}

AltelRegCtrl::AltelRegCtrl(const JadeOption &opt)
  :m_opt(opt), m_id(0), m_ip_udp_port(4660), JadeRegCtrl(opt){
  m_ip_address  = m_opt.GetStringValue("IP_ADDRESS");
  if(m_ip_address.empty()){
    std::cerr<<"altelregctrl: error ip address\n";
  }
  m_ip_udp_port = (uint16_t) m_opt.GetIntValue("IP_UDP_PORT");
}

void AltelRegCtrl::Open(){
  std::cout<<"sending INIT"<<std::endl;
  SendCommand("INIT", "");
  std::cout<<"sending INIT"<<std::endl;
  SendCommand("START", "");
  std::cout<<"open done"<<std::endl;
}

void AltelRegCtrl::Close(){
  SendCommand("STOP", "");
}


void AltelRegCtrl::WriteByte(uint64_t addr, uint8_t val){
  m_id++;
  rbcp_header sndHeader;
  sndHeader.type=RBCP_VER;
  sndHeader.command= RBCP_CMD_WR;
  sndHeader.id=m_id;
  sndHeader.length=1;
  sndHeader.address=htonl(addr); //TODO: check
  char rcvdBuf[2048];
  rbcp_com(m_ip_address.c_str(), m_ip_udp_port, &sndHeader, (char*) &val, rcvdBuf, RBCP_DISP_MODE_NO);
  //TODO: if failure
  
}

uint8_t AltelRegCtrl::ReadByte(uint64_t addr){
  m_id++;
  rbcp_header sndHeader;
  sndHeader.type=RBCP_VER;
  sndHeader.command= RBCP_CMD_RD;
  sndHeader.id=m_id;
  sndHeader.length=1;
  sndHeader.address=htonl(addr);
  char rcvdBuf[2048];
  int re = rbcp_com(m_ip_address.c_str(), m_ip_udp_port, &sndHeader, NULL, rcvdBuf, RBCP_DISP_MODE_NO);
  if (re!=1){
    std::cout<< "here error: "<<re<<std::endl;
  }
  return rcvdBuf[0];
}

void AltelRegCtrl::ChipID(uint8_t id){
  uint64_t addr = 0x10000001;
  WriteByte(addr, id);
}

void AltelRegCtrl::WriteReg(uint16_t addr, uint16_t val){
  uint64_t chip_addr_base_h = 0x10000002;
  uint64_t chip_addr_base_l = 0x10000003;
  uint64_t chip_data_h = 0x10000004;
  uint64_t chip_data_l = 0x10000005;
  WriteByte(chip_addr_base_h, (addr>>8)&0xff);
  WriteByte(chip_addr_base_l, addr&0xff);
  WriteByte(chip_data_h, (val>>8)&0xff);
  WriteByte(chip_data_l, val&0xff);
  WriteByte(0, 0x9c);
}

uint16_t AltelRegCtrl::ReadReg(uint16_t addr){
  uint64_t chip_addr_base_h = 0x10000002;
  uint64_t chip_addr_base_l = 0x10000003;
  uint64_t chip_data_c = 0x1000000d;
  uint64_t chip_data_h = 0x1000000e;
  uint64_t chip_data_l = 0x1000000f;
  WriteByte(chip_addr_base_h, (addr>>8)&0xff);
  WriteByte(chip_addr_base_l, addr&0xff);
  WriteByte(0, 0x4e);
  uint16_t val_c = ReadByte(chip_data_c);
  uint8_t val_h = ReadByte(chip_data_h);
  uint8_t val_l = ReadByte(chip_data_l);
  uint16_t val = val_h * 0xff + val_h;
  std::cout<<">>>>>>>>>>>>>read "<<val<< " count "<< val_c<<std::endl;
  
  return val;
}

void AltelRegCtrl::Broadcast(uint8_t opcode){
  uint64_t addr = 0x10000006;
  WriteByte(addr, opcode);
  WriteByte(0, 0x50);
}

void AltelRegCtrl::SetFrameDuration(uint16_t len){
  uint64_t addr_h = 0x10000007;
  uint64_t addr_l = 0x10000008;
  WriteByte(addr_h, (len>>8)&0xff);
  WriteByte(addr_l, len&0xff);
}

void AltelRegCtrl::SetFramePhase(uint16_t len){
  uint64_t addr_h = 0x10000009;
  uint64_t addr_l = 0x1000000a;
  WriteByte(addr_h, (len>>8)&0xff);
  WriteByte(addr_l, len&0xff);
}

void AltelRegCtrl::SetInTrigGap(uint16_t len){
  uint64_t addr_h = 0x1000000b;
  uint64_t addr_l = 0x1000000c;
  WriteByte(addr_h, (len>>8)&0xff);
  WriteByte(addr_l, len&0xff);
}

void AltelRegCtrl::SetFPGAMode(uint8_t mode){
  uint64_t addr = 0x10000010;
  WriteByte(addr, mode);
}

void AltelRegCtrl::SetFrameNumber(uint8_t n){
  uint64_t addr = 0x10000011;
  WriteByte(addr, n);
}



void AltelRegCtrl::InitALPIDE(){
  SetFPGAMode(0);
  //ResetDAQ();

  ChipID(0x10);

  // ReadReg(0x04);
  // ReadReg(0x05);
  

  Broadcast(0xD2);
  WriteReg(0x10,0x70);
  WriteReg(0x4,0x10);
  WriteReg(0x5,0x28);
  WriteReg(0x601,0x75);
  WriteReg(0x602,0x93);
  WriteReg(0x603,0x56);
  WriteReg(0x604,0x32);
  WriteReg(0x605,0xFF);
  WriteReg(0x606,0x0);
  WriteReg(0x607,0x39);
  WriteReg(0x608,0x0);
  WriteReg(0x609,0x0);
  WriteReg(0x60A,0x0);
  WriteReg(0x60B,0x32);
  WriteReg(0x60C,0x40);
  WriteReg(0x60D,0x40);
  WriteReg(0x60E,50); //empty 0x32; 0x12 data, not full. 
  WriteReg(0x701,0x400);
  WriteReg(0x487,0xFFFF);
  WriteReg(0x500,0x0);
  WriteReg(0x500,0x1);
  WriteReg(0x1,0x3C);
  Broadcast(0x63);
  StartPLL();
}

void AltelRegCtrl::StartPLL(){
  WriteReg(0x14,0x008d);
  WriteReg(0x15,0x0088);
  WriteReg(0x14,0x0085);
  WriteReg(0x14,0x0185);
  WriteReg(0x14,0x0085);
}

void AltelRegCtrl::StartWorking(uint8_t trigmode){
  WriteReg(0x487,0xFFFF);
  WriteReg(0x500,0x0);
  WriteReg(0x487,0xFFFF);
  WriteReg(0x500,0x1);
  WriteReg(0x4,0x10);
  WriteReg(0x5,156);   //3900ns    
  //WriteReg(0x5,1);   //25ns    
  WriteReg(0x1,0x3D);
  Broadcast(0x63);
  Broadcast(0xe4);
  //SetFrameDuration(16);   //400ns
  SetFrameDuration(100);   //400ns  //2500ns
  SetInTrigGap(20);
  SetFPGAMode(0x1);
}

void AltelRegCtrl::ResetDAQ(){
  WriteByte(0, 0xff);
}

std::string AltelRegCtrl::SendCommand(const std::string &cmd, const std::string &para){
  if(cmd=="INIT"){
    InitALPIDE();
  }

  if(cmd=="START"){
    StartWorking(1);//ext trigger
  }

  if(cmd=="STOP"){
    SetFPGAMode(0);
  }

  if(cmd=="RESET"){
    ResetDAQ();
  }

  return "";
}

int AltelRegCtrl::rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode){

  struct sockaddr_in sitcpAddr;
  
  int  sock;

  struct timeval timeout;
  fd_set setSelect;
  
  int sndDataLen;
  int cmdPckLen;

  char sndBuf[2048];
  int i, j = 0;
  int rcvdBytes;
  char rcvdBuf[2048];
  int numReTrans =0;

  /* Create a Socket */
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nCreate socket...\n");

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  sitcpAddr.sin_family      = AF_INET;
  sitcpAddr.sin_port        = htons(port);
  sitcpAddr.sin_addr.s_addr = inet_addr(ipAddr);

  sndDataLen = (int)sendHeader->length;

  if(dispMode==RBCP_DISP_MODE_DEBUG) printf(" Length = %i\n",sndDataLen);

  /* Copy header data */
  memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));

  if(sendHeader->command==RBCP_CMD_WR){
    memcpy(sndBuf+sizeof(struct rbcp_header),sendData,sndDataLen);
    cmdPckLen=sndDataLen + sizeof(struct rbcp_header);
  }else{
    cmdPckLen=sizeof(struct rbcp_header);
  }


  if(dispMode==RBCP_DISP_MODE_DEBUG){
    for(i=0; i< cmdPckLen;i++){
      if(j==0) {
	printf("\t[%.3x]:%.2x ",i,(unsigned char)sndBuf[i]);
	j++;
      }else if(j==3){
	printf("%.2x\n",(unsigned char)sndBuf[i]);
	j=0;
      }else{
	printf("%.2x ",(unsigned char)sndBuf[i]);
	j++;
      }
    }
    if(j!=3) printf("\n");
  }

  /* send a packet*/

  sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("The packet have been sent!\n");

  /* Receive packets*/
  
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nWait to receive the ACK packet...");


  while(numReTrans<3){

    FD_ZERO(&setSelect);
    FD_SET(sock, &setSelect);

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
 
    if(select(sock+1, &setSelect, NULL, NULL,&timeout)==0){
      /* time out */
      puts("\n***** Timeout ! *****");
      sendHeader->id++;
      memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));
      sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
      numReTrans++;
      FD_ZERO(&setSelect);
      FD_SET(sock, &setSelect);
    } else {
      /* receive packet */
      if(FD_ISSET(sock,&setSelect)){
	rcvdBytes=recvfrom(sock, rcvdBuf, 2048, 0, NULL, NULL);

	if(rcvdBytes<sizeof(struct rbcp_header)){
	  puts("ERROR: ACK packet is too short");
	  close(sock);
	  return -1;
	}

	if((0x0f & rcvdBuf[1])!=0x8){
	  puts("ERROR: Detected bus error");
	  close(sock);
	  return -1;
	}

	rcvdBuf[rcvdBytes]=0;

	if(RBCP_CMD_RD){
	  memcpy(recvData,rcvdBuf+sizeof(struct rbcp_header),rcvdBytes-sizeof(struct rbcp_header));
	}

	if(dispMode==RBCP_DISP_MODE_DEBUG){
	  puts("\n***** A pacekt is received ! *****.");
	  puts("Received data:");

	  j=0;

	  for(i=0; i<rcvdBytes; i++){
	    if(j==0) {
	      printf("\t[%.3x]:%.2x ",i, sendHeader,(unsigned char)rcvdBuf[i]);
	      j++;
	    }else if(j==3){
	      printf("%.2x\n",(unsigned char)rcvdBuf[i]);
	      j=0;
	    }else{
	      printf("%.2x ",(unsigned char)rcvdBuf[i]);
	      j++;
	    }
	    if(i==7) printf("\n Data:\n");
	  }
	  if(j!=3) puts(" ");
	}else if(dispMode==RBCP_DISP_MODE_INTERACTIVE){
	  if(sendHeader->command==RBCP_CMD_RD){
	    j=0;
	    puts(" ");

	    for(i=8; i<rcvdBytes; i++){
	      if(j==0) {
		printf(" [0x%.8x] %.2x ",ntohl(sendHeader->address)+i-8,(unsigned char)rcvdBuf[i]);
		j++;
	      }else if(j==7){
		printf("%.2x\n",(unsigned char)rcvdBuf[i]);
		j=0;
	      }else if(j==4){
		printf("- %.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }else{
		printf("%.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }
	    }
	    
	    if(j!=15) puts(" ");
	  }else{
	    printf(" 0x%x: OK\n",ntohl(sendHeader->address));
	  }
	}
	numReTrans = 4;
	close(sock);
	return(rcvdBytes);
      }
    }
  }
  close(sock);
  return -3;
}
