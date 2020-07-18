#include "tsCommon.h"
#include "tsTransportStream.h"
#include <cstdio>

using namespace std;

int main( int argc, char *argv[ ], char *envp[ ])
{
  FILE * stream = fopen("example_new.ts", "rb");
  int TS_Size = 188;
  size_t open_Stream;

  xTS_PacketHeader    TS_PacketHeader;
  xTS_AdaptationField TS_AdaptationField;
  xPES_Assembler PES_Assembler;
  PES_Assembler.Init(136);

  uint8_t * TS_PacketBuffer;
  TS_PacketBuffer = (uint8_t*) malloc (sizeof(uint8_t)*TS_Size);

  int32_t TS_PacketId = 0;
  while(!feof(stream))
  {
    open_Stream = fread(TS_PacketBuffer,1,TS_Size,stream);

    TS_PacketHeader.Reset();
    TS_PacketHeader.Parse(TS_PacketBuffer);

    printf("%010d ", TS_PacketId);
    TS_PacketHeader.Print();

    if(TS_PacketHeader.hasAdaptationField()) {
      TS_AdaptationField.Reset();
      TS_AdaptationField.Parse(TS_PacketBuffer, TS_PacketHeader.getAFC());
      printf("\n           ");
      TS_AdaptationField.Print();
    }

    if (TS_PacketHeader.getPID() == 136) {
        xPES_Assembler::eResult Result = PES_Assembler.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_AdaptationField);
        switch (Result) {

            case xPES_Assembler::eResult::StreamPackedLost:
                printf("PackedLost");
                break;

            case xPES_Assembler::eResult::AssemblingStarted:
                printf("\n");
                printf("Started");
                printf("\n");
                PES_Assembler.PrintPESH();
                break;

            case xPES_Assembler::eResult::AssemblingContinue:
                printf("Continue");
                break;

            case xPES_Assembler::eResult::AssemblingFinished:
                printf("\n");
                printf("Finished");
                printf("\n");
                printf("PES: Len=%d HeaderLen=%d DataLen=%d",
                PES_Assembler.get_m_PESH().getPacketLength()+6,
                PES_Assembler.get_m_PESH().get_PES_header_data_length(),
                PES_Assembler.getNumPacketBytes());
                break;

            default:
                break;
      }
    }

    printf("\n");

    TS_PacketId++;
  }
  fclose (stream);
  free (TS_PacketBuffer);
}
