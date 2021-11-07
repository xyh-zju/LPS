#ifndef __ROUTE_H
#define __ROUTE_H

#include <stdint.h>

typedef enum
{
    DEVICE_MODE_IDLERX                              = 0x00,
    DEVICE_MODE_RANGING,                                                        
}DeviceStates_t;

typedef struct MESHPACKAGE{
    uint8_t type; //0 for join, 1 for reply, 2 for hop-transmit, 3 for broadcasting find, 4 for failed
    uint8_t ttl;
    uint16_t seq;
    uint16_t ack;
    uint16_t hops;
    uint32_t hop_addr; //广播包为发送本包的地址，消息包为下一个中继的地址
    uint32_t des_addr; //目标地址
    uint32_t src_addr; //源地址
    uint32_t length;
    //uint8_t* data;
} MeshPackage;

typedef struct WAITING{
    uint8_t resendNumber;
    uint16_t pseq; //包在上一个结点中的序号
		uint32_t required_addr;
    MeshPackage* package;
		MeshPackage* packageTOsend;
		struct WAITING* next;
} WaitingNode;

typedef struct RT_ENTRY{
    uint32_t des_addr;
    uint16_t Freshness;
    uint8_t  flag; //0 for invalid, 1 for out of date, 2 for valid
    uint16_t num_hops;
    uint32_t next_hop;
    struct RT_ENTRY* next;
    //uint32_t *formerlist; //the former nodes use this route path
    //uint32_t lifttime;
} RT_Entry;

//uint8_t Mesh_Send(uint32_t destination, char* message, int length);

void init_Route();

uint8_t Mesh_Reply_Join(MeshPackage* package);
uint8_t Mesh_Reply(MeshPackage* package);
uint8_t Mesh_Handle_Reply(MeshPackage* package);
uint8_t Mesh_Handle_Broadcast(MeshPackage* package);
uint8_t Mesh_transmit(MeshPackage* package);
MeshPackage* findRoute_RT(uint32_t des_addr, uint32_t require_addr, uint16_t hops);
uint8_t Mesh_sendMessage(uint32_t des_addr, char* message, int length);
//int Mesh_RouteFound(uint32_t destination);
//uint8_t Mesh_Construct(uint32_t destination);
//uint8_t Mesh_RouteMaintenance();
//uint8_t Mesh_Join();
void Mesh_Send(MeshPackage* package);
void Resend_Waintinglist();


#endif /* __MAIN_H */