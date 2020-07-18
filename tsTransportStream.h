#pragma once
#include "tsCommon.h"
#include <string>

/*
MPEG-TS packet:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |                             Header                            | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   4 |                  Adaptation field + Payload                   | `
`     |                                                               | `
` 184 |                                                               | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `


MPEG-TS packet header:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |       SB      |E|S|T|           PID           |TSC|AFC|   CC  | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `

Sync byte                    (SB ) :  8 bits
Transport error indicator    (E  ) :  1 bit
Payload unit start indicator (S  ) :  1 bit
Transport priority           (T  ) :  1 bit
Packet Identifier            (PID) : 13 bits
Transport scrambling control (TSC) :  2 bits
Adaptation field control     (AFC) :  2 bits
Continuity counter           (CC ) :  4 bits
*/


//=============================================================================================================================================================================

class xTS
{
public:
  static constexpr uint32_t TS_PacketLength = 188;
  static constexpr uint32_t TS_HeaderLength = 4;

  static constexpr uint32_t PES_HeaderLength = 6;

  static constexpr uint32_t BaseClockFrequency_Hz         =    90000; //Hz
  static constexpr uint32_t ExtendedClockFrequency_Hz     = 27000000; //Hz
  static constexpr uint32_t BaseClockFrequency_kHz        =       90; //kHz
  static constexpr uint32_t ExtendedClockFrequency_kHz    =    27000; //kHz
  static constexpr uint32_t BaseToExtendedClockMultiplier =      300;
};

//=============================================================================================================================================================================

class xTS_PacketHeader
{
public:
  enum class ePID : uint16_t
  {
    PAT  = 0x0000,
    CAT  = 0x0001,
    TSDT = 0x0002,
    IPMT = 0x0003,
    NIT  = 0x0010, //DVB specific PID
    SDT  = 0x0011, //DVB specific PID
    NuLL = 0x1FFF,
  };

protected:
  uint8_t SB = 0;
  uint8_t E = 0;
  uint8_t S = 0;
  uint8_t T = 0;
  uint16_t PID = 0;
  uint8_t TSC = 0;
  uint8_t AFC = 0;
  uint8_t CC = 0;

public:
  void     Reset();
  int32_t  Parse(const uint8_t* Input);
  void     Print() const;

public:
  uint8_t getSB() {return this -> SB;}
  uint8_t getE() {return this -> E;}
  uint8_t getS() {return this -> S;}
  uint8_t getT() {return this -> T;}
  uint8_t getPID() {return this -> PID;}
  uint8_t getTSC() {return this -> TSC;}
  uint8_t getAFC() {return this -> AFC;}
  uint8_t getCC() {return this -> CC;}

public:
  //TODO
  bool     hasAdaptationField() const { return (this -> AFC == 1) ? false : true; }
  bool     hasPayload        () const { return (this -> AFC > 1) ? true : false; }
};

//*********************************************************

class xTS_AdaptationField {
public:
    uint8_t AFL;
    uint8_t DC;
    uint8_t RA;
    uint8_t SP;
    uint8_t PCR;
    uint8_t OPCR;
    uint8_t SP2;
    uint8_t TP;
    uint8_t EX;

public:
    void Reset();
    int32_t Parse(const uint8_t* Input, uint8_t Control);
    void Print() const;

public:
    uint32_t getBytes() const { return this->AFL; };
};

//=============================================================================================================================================================================

class xPES_PacketHeader {
  public:
    enum eStreamId: uint8_t {
        eStreamId_program_stream_map = 0xBC,
        eStreamId_padding_stream = 0xBE,
        eStreamId_private_stream_2 = 0xBF,
        eStreamId_ECM = 0xF0,
        eStreamId_EMM = 0xF1,
        eStreamId_program_stream_directory = 0xFF,
        eStreamId_DSMCC_stream = 0xF2,
        eStreamId_ITUT_H222_1_type_E = 0xF8,
    };

  protected:
    uint32_t m_PacketStartCodePrefix;
    uint8_t m_StreamId;
    uint16_t m_PacketLength;
    uint8_t PES_header_data_length;

  public:
    void Reset();
    int32_t Parse(const uint8_t * Input);
    void Print() const;

  public:
    uint32_t getPacketStartCodePrefix() const {
      return m_PacketStartCodePrefix;
    }
    uint8_t getStreamId() const {
      return m_StreamId;
    }
    uint16_t getPacketLength() const {
      return m_PacketLength;
    }
    uint8_t get_PES_header_data_length() {
      return PES_header_data_length;
    }
};

//***************************************************************

class xPES_Assembler {
  public:
    enum class eResult: int32_t {
        UnexpectedPID = 1,
        StreamPackedLost,
        AssemblingStarted,
        AssemblingContinue,
        AssemblingFinished,
    };

  protected:
    int32_t m_PID;

    uint8_t * m_Buffer;
    uint32_t m_BufferSize;
    uint32_t m_DataOffset;

    int8_t m_LastContinuityCounter;
    bool m_Started;
    xPES_PacketHeader m_PESH;

    FILE * pFile;

  public:
    xPES_Assembler();
    ~xPES_Assembler();
    void Init(int32_t PID);
    eResult AbsorbPacket(
      const uint8_t * TransportStreamPacket,
      const xTS_PacketHeader * PacketHeader,
      const xTS_AdaptationField * AdaptationField
    );
    void PrintPESH() const {
      m_PESH.Print();
    }
    uint8_t * getPacket() {
      return m_Buffer;
    }
    int32_t getNumPacketBytes() const {
      return m_DataOffset;
    }
    xPES_PacketHeader get_m_PESH() {
      return m_PESH;
    }
  protected:
    void xBufferReset();
    void xBufferAppend(const uint8_t * Data, int32_t Size);
};
