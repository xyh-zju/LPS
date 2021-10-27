#ifndef __ROUTE_H
#define __ROUTE_H

#include <stdint.h>

typedef enum
{
    DEVICE_MODE_IDLERX                              = 0x00,
    DEVICE_MODE_RANGING,                                                        
}DeviceStates_t;

typedef struct WAITING{
    uint8_t type; //1 for find route and return, 2 for recieved message, 3 for find route and transmit
    uint8_t resendNumber;
    uint16_t seq;
    uint32_t des_addr;
    uint32_t require_addr;
    MeshPackage* package;
    struct WAITING* next;
} WaitingNode;

typedef struct MESHPACKAGE{
    uint8_t type; //0 for join, 1 for apply, 2 for transmit, 3 for broadcasting find
    uint8_t ttl;
    uint16_t seq;
    uint16_t ack;
    uint16_t hops;
    uint32_t des_addr; 
    uint32_t final_addr; 
    uint32_t src; 
    uint32_t length;
    //uint8_t* data;
} MeshPackage;

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

uint8_t Mesh_Recieve(char* package, int length);
uint8_t Mesh_Reply_Join(uint32_t des);
uint8_t Mesh_Handle_Reply(MeshPackage* p);
uint8_t Mesh_transmit(MeshPackage* p);
//int Mesh_RouteFound(uint32_t destination);
//uint8_t Mesh_Construct(uint32_t destination);
//uint8_t Mesh_RouteMaintenance();
//uint8_t Mesh_Join();
void Mesh_Send(MeshPackage p, char* data, int dataLength);



#endif /* __MAIN_H */