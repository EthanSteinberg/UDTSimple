#include <boost/cstdint.hpp>

struct t_Packet
{
   uint8_t type;

   t_Packet()
   {
      type = 0;
   }
};

struct t_askPacket : public t_Packet
{
   t_askPacket()
   {
      type = 1;
   }
};

struct t_wantPacket : public t_Packet
{ 
   char name[100];  
 
   uint64_t size;
   
   t_wantPacket()
   {
      type = 2;
   }
};

   
struct t_disconectPacket : public t_Packet
{ 
   t_disconectPacket()
   {
      type = 3;
   }
};
