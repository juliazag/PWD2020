#include "tsTransportStream.h"

//=============================================================================================================================================================================
// xTS_PacketHeader
//=============================================================================================================================================================================

void xTS_PacketHeader::Reset(){
    this -> SB = 0;
    this -> E = 0;
    this -> S = 0;
    this -> T = 0;
    this -> PID = 0;
    this -> TSC = 0;
    this -> AFC = 0;
    this -> CC = 0;
}

int32_t xTS_PacketHeader::Parse(const uint8_t *Input){
    uint32_t byte = 0;
    for(int i=0; i<4; i++) {
        byte <<= 8;
        byte = byte | *(Input+i);
    }

    this->sb  = ((byte & 0xff000000) >> 24);
    this->e   = ((byte & 0x800000)   >> 23);
    this->s   = ((byte & 0x400000)   >> 22);
    this->t   = ((byte & 0x200000)   >> 21);
    this->pid = ((byte & 0x1fff00)   >>  8);
    this->tsc = ((byte & 0xc0)       >>  6);
    this->afc = ((byte & 0x30)       >>  4);
    this->cc  = ((byte & 0xf)        >>  0);
}

void xTS_PacketHeader::Print(){
    printf("TS: ");
    printf("SB=%d ",this -> SB);
    printf("E=%d ",this -> E);
    printf("S=%d ",this -> S);
    printf("T=%d ",this -> T);
    printf("PID=%d ",this -> PID);
    printf("TSC=%d ",this -> TSC);
    printf("AFC=%d ",this -> AFC);
    printf("CC=%d ",this -> CC);
}

//******************************************************

void xTS_AdaptationField::Reset() {
    this->AFL  = 0;
    this->DC   = 0;
    this->RA   = 0;
    this->SP   = 0;
    this->PR  = 0;
    this->OR = 0;
    this->SP2  = 0;
    this->TP   = 0;
    this->EX   = 0;

    this->program_clock_reference_base = 0;
    this->program_clock_reference_extension = 0;

    this->original_program_clock_reference_base = 0;
    this->original_program_clock_reference_extension = 0;

    this->splice_countdown = 0;
    this->transport_private_data_length = 0;
    this->stuffing_byte_length = 0;
}

int32_t xTS_AdaptationField::Parse(const uint8_t* Input, uint8_t Control) {
    int AF_offset = 4;
    this->AFL = *(Input+AF_offset);
    AF_offset = 5;
    int i = 0;
    uint8_t flags = *(Input+AF_offset+(i++));
    this->DC = ((flags & 0x80) >> 7);
    this->RA = ((flags & 0x40) >> 6);
    this->SP = ((flags & 0x20) >> 5);
    this->PR = ((flags & 0x10) >> 4);
    this->OR = ((flags & 0x8)  >> 3);
    this->SP2 = ((flags & 0x4)  >> 2);
    this->TP = ((flags & 0x2)  >> 1);
    this->EX = ((flags & 0x1)  >> 0);
}

void xTS_AdaptationField::Print() const {
    printf("AF: ");
    printf("L=%3d ", this->getBytes());
    printf("DC=%d ", this->DC);
    printf("RA=%d ", this->RA);
    printf("SP=%d ", this->SP);
    printf("PR=%d ", this->PR);
    printf("OR=%d ", this->OR);
    printf("SP=%d ", this->SP2);
    printf("TP=%d ", this->TP);
    printf("EX=%d ", this->EX);
}

//=============================================================================================================================================================================

void xPES_PacketHeader::Reset() {
  this->m_PacketStartCodePrefix = 0;
  this->m_StreamId = 0;
  this->m_PacketLength = 0;
}

int32_t xPES_PacketHeader::Parse(const uint8_t* Input) {
  for(int i=0; i<3; i++) {
    this->m_PacketStartCodePrefix <<= 8;
    this->m_PacketStartCodePrefix = this->m_PacketStartCodePrefix | *(Input+i);
  }

  this->m_StreamId = *(Input+3);

  for(int i=0; i<2; i++) {
    this->m_PacketLength <<= 8;
    this->m_PacketLength = this->m_PacketLength | *(Input+4+i);
  }

  if(this->m_StreamId != eStreamId_program_stream_map &&
     this->m_StreamId != eStreamId_padding_stream &&
     this->m_StreamId != eStreamId_private_stream_2 &&
     this->m_StreamId != eStreamId_ECM &&
     this->m_StreamId != eStreamId_EMM &&
     this->m_StreamId != eStreamId_program_stream_directory &&
     this->m_StreamId != eStreamId_DSMCC_stream &&
     this->m_StreamId != eStreamId_ITUT_H222_1_type_E) {

        this->PES_header_data_length = *(Input+8)+6+2+1;
        uint32_t PTS = 0;
        uint32_t DTS = 0;
    }
}

void xPES_PacketHeader::Print() const {
  printf("PES: ");
  printf("PSCP=%d ", this->m_PacketStartCodePrefix);
  printf("SID=%d ", this->m_StreamId);
  printf("L=%d ", this->m_PacketLength);
}

//****************************************************************************************************

xPES_Assembler::xPES_Assembler() {
  this->m_PID = 0;

  this->m_Buffer = NULL;
  this->m_BufferSize = 0;
  this->m_DataOffset = 0;

  this->m_LastContinuityCounter = 0;
  this->m_Started = 0;
  this->m_PESH.Reset();

  this->pFile = NULL;
}

xPES_Assembler::~xPES_Assembler() {
  fclose(this->pFile);
}

void xPES_Assembler::Init(int32_t PID) {
  this->m_PID = PID;
  this->m_LastContinuityCounter = 15;
  this->pFile = fopen("PID136.mp2", "wb");
}

xPES_Assembler::eResult xPES_Assembler::AbsorbPacket (
  const uint8_t * TransportStreamPacket,
  const xTS_PacketHeader * PacketHeader,
  const xTS_AdaptationField * AdaptationField
) {
  this->m_Started = (*PacketHeader).getS();
  uint8_t length = 188;
  uint8_t offset = 4; //TS_HeaderLength
  if((*PacketHeader).hasAdaptationField()) {
    offset += 1; //xTS_AdaptationField adaptation_field_length
    offset += (*AdaptationField).getNumBytes();
  }

  length -= offset; //188B - TS_Packet bytes

  int cc = (*PacketHeader).getCC();
  if(cc != ((this->m_LastContinuityCounter+1)%16) ) {
    printf("ContinuityCounter not valid");
  }
  this->m_LastContinuityCounter = cc;

  xPES_Assembler::eResult res;

  if(this->m_Started) {
    this->m_PESH.Reset();
    this->m_PESH.Parse(TransportStreamPacket+offset);
    res = xPES_Assembler::eResult::AssemblingStarted;
  }
  else if(this->m_DataOffset + length == this->m_BufferSize) {
    res = xPES_Assembler::eResult::AssemblingFinished;
  }
  else {
    res = xPES_Assembler::eResult::AssemblingContinue;
  }

  this->xBufferAppend(TransportStreamPacket+offset, length);
  return res;
}

void xPES_Assembler::xBufferReset() {
  this->m_BufferSize = 0;
  this->m_DataOffset = 0;
}

void xPES_Assembler::xBufferAppend(const uint8_t * Data, int32_t Size) {
  if(this->m_Started == true) {
    this->xBufferReset();
    this->m_BufferSize = this->m_PESH.getPacketLength()-(this->get_m_PESH().get_PES_header_data_length()-6);

    if(!this->m_Buffer) {
      this->m_Buffer = (uint8_t*) malloc (sizeof(uint8_t)* this->m_BufferSize);
    }
    else {
      uint8_t* ptr = (uint8_t*) realloc (this->m_Buffer, sizeof(uint8_t)* this->m_BufferSize);
      if (ptr != NULL) {
        this->m_Buffer = ptr;
      }
      else {
        free(this->m_Buffer);
        puts("Reallocating memory");
        exit(1);
      }
    }
  }

  int i = (this->m_Started) ? this->get_m_PESH().get_PES_header_data_length() : 0;
  for(i; i<Size; i++) {
    *(this->m_Buffer+this->m_DataOffset) = *(Data+i);
    this->m_DataOffset++;
  }

  if(this->m_DataOffset == this->m_BufferSize) {
    fwrite(this->m_Buffer, 1, this->m_BufferSize, this->pFile);
  }
}
