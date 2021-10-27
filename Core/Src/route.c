#include "route.h"
#include "main.h"
#include "ranging.h"

uint32_t My_addr;

int EntryNumber=0;
// int RTindex[64];
// RT_Entry* RouteTable[64]; 
//RT_Entry* RTTail;
RT_Entry* RTHead;
int WaitingNumber=0;
WaitingNode* Waitinglist;
uint32_t reply_ip;
uint16_t SEQ=1;
extern DeviceStates_t DeviceState;
extern uint8_t RxDoneFlag;
extern uint8_t RxDoneFlag;
extern uint8_t RxData[];
extern uint8_t RxSize;

void init_Waitinglist(){
	Waitinglist=(WaitingNode*)malloc(sizeof(WaitingNode));
	Waitinglist->next=NULL;
	Waitinglist->seq=0;
	Waitinglist->type=0;
	Waitinglist->des_addr=0;
	Waitinglist->require_addr=0;
	Waitinglist->resendNumber=0;
	Waitinglist->package=NULL;
	WaitingNumber=0;
}

WaitingNode* add_Waitinglist(uint8_t type, uint16_t seq, uint32_t des_addr, uint32_t re_addr){
	WaitingNode* p=(WaitingNode*)malloc(sizeof(WaitingNode));
	p->next=Waitinglist->next;
	p->seq=seq;
	p->type=type;
	p->des_addr=des_addr;
	p->require_addr=re_addr;
	p->resendNumber=0;
	p->package=NULL;
	WaitingNumber++;
	Waitinglist->next=p;
	return p;
}

int getResponse_Waitinglist(MeshPackage* package){
	WaitingNode* p=Waitinglist->next;
	WaitingNode* q=Waitinglist;
	while(p!=NULL){
		if(p->seq==package->ack){
			q->next=p->next;
			if(p->type==1){
				reply_Waitinglist(p, package->hops); //only send reply to find node
				add_RT(p->des_addr, package->src, package->hops, 0); //fresh haven't handled
			}
			else if(p->type==3){
				transmit_Waitinglist(p);
			}
			if(p->package!=NULL) free(p->package);
			free(p);
			return 1;
		}
	}
	return 0;
}

void transmit_Waitinglist(WaitingNode* p){
	

}

void reply_Waitinglist(WaitingNode* pwaiting, uint16_t hops){
	MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=1;
	p->length=0;
	p->des_addr=pwaiting->required_addr;
	p->src=My_addr;
	p->ttl=0;
	p->ack=pwaiting->seq;
	p->seq=pwaiting->seq;
	p->hops=hops+1;
	Mesh_Send(*p, NULL, 0);
}

void Mesh_Send(MeshPackage head, char* data, int dataLength){
	uint8_t* p=(uint8_t)malloc(sizeof(head)+dataLength);
	*p=(uint8_t) head;
	if(data!=NULL){
		uint8_t* q=p+sizeof(head);
		*q=*data; //Assemble message package
	}
	LoRaSendData(p, sizeof(head)+dataLength);
	LoRaSetRx();
}

void findRoute_RT(uint32_t des_addr){
	RT_Entry* p=get_RT(des);
	if(p==NULL){ //broadcast to find the route
		MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
		p->type=3;
		p->length=0;
		p->des_addr=des_addr;
		p->src=My_addr;
		p->ttl=0;
		p->ack=0;
		p->seq=SEQ; //each waiting package must have a unique seq
		p->hops=0;
		Mesh_Send(*p, NULL, 0);
		//add to the waiting list
		//set a timer to resend the request
		//if 
	}
}

void init_RT(){
	RTHead=(RT_Entry*)malloc(sizeof(RT_Entry));
	RTHead->des_addr=0;
	RTHead->Freshness=0;
	RTHead->flag=0;
	RTHead->num_hops=0;
	RTHead->next_hop=0;
	RTHead->next=NULL;
	EntryNumber=0;
}

RT_Entry* get_RT(uint32_t des){
	RT_Entry* p=RTHead->next;
	while(p!=NULL){
		if(p->des_addr==des){
			break;
		}
		p=p->next;
	}
	return p; //not found return null, found return pointer
}

void delete_RT(uint32_t des){
	RT_Entry* p=RTHead->next;
	RT_Entry* q=RTHead;
	while(p!=NULL){
		if(p->des_addr==des){
			break;
		}
		q=p;
		p=p->next;
	}
	if(p!=null){
		q->next=p->next;
		free(p);
			EntryNumber--;
	}
}

void add_RT(uint32_t des, uint32_t nextHop, uint16_t numHops, uint16_t fresh){
	RT_Entry* p=get_RT(des);
	if(p==NULL){ // add
		RT_Entry* p=(RT_Entry*)malloc(sizeof(RT_Entry));
		p->des_addr=des;
		p->Freshness=fresh;
		p->flag=2;
		p->num_hops=numHops;
		p->next_hop=nextHop;
		p->next=RTHead->next;
		RTHead->next=p;
		EntryNumber++;
	}
	else{
		if(p->Freshness<fresh){ // update
			p->num_hops=numHops;
			p->next_hop=nextHop;
			p->num_hops=numHops;
		}
	}
}

int clearSubs_RT(uint32_t nextHop){
	RT_Entry* p=RTHead->next;
	RT_Entry* q=RTHead;
	int num=0;
	while(p!=NULL){
		if(p->next_hop==nextHop){
			q->next=p->next;
			free(p);
			EntryNumber--;
			p=q->next;
			num++;
		}
		else{
			p=p->next;
			q=q->next;
		}
	}
	return num;
}

int cleanRT_RT(){  //clean the invalid route
	RT_Entry* p=RTHead->next;
	RT_Entry* q=RTHead;
	int num=0;
	while(p!=NULL){
		if(p->flag==0){
			q->next=p->next;
			free(p);
			EntryNumber--;
			p=q->next;
			num++;
		}
		else{
			p=p->next;
			q=q->next;
		}
	}
	return num;
}

int numofRT_RT(){
	return EntryNumber;
}


//uint8_t Mesh_Send(uint32_t destination, char* message, int length){ 
//    if(Mesh_RouteFound(destination)){
//        //make the package and send
//        return 1;
//    }
//    else{
//        //send false, the destination is unreachable
//        return 0;
//    }
//}

uint8_t Mesh_Recieve(char* package, int length){
    //analysis the package
    MeshPackage* head = (MeshPackage*)package;
	
    switch (head->type)
    {
    case 0/* join */:
        /* add to the RT */
		add_RT(head->des_addr)
        break;
    case 1/* recieve hello */:
				printf("Success transmit the info!");
				reply_ip=head->dest;
    default:
        break;
    }






/*
    uint32_t des_addr=0;
    if(des_addr==My_addr){
        //recieved my message
        //read the message
        //return a success response?
    }
    else if(Mesh_RouteFound(des_addr)){
        //find the destination in the route table, transmit
        //send the package to the next hop
    }
    else {
        //the path is down, broadcast the error message to the network to cancel this path in the network
    }
    */
}

uint8_t Mesh_Reply_Join(MeshPackage* p){
	MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=1;
	p->length=0;
	p->des_addr=p->des_addr;
	p->src=My_addr;
	p->ttl=0;
	p->ack=p->seq;
	p->seq=p->seq;
	p->hops=0;
	Mesh_Send(*p, NULL, 0);
}

uint8_t Mesh_Handle_Reply(MeshPackage* p){
	getResponse_Waitinglist(p);
}

uint8_t Mesh_transmit(MeshPackage* p){
	RT_Entry*info= get_RT(p->des_addr);
	if(info==NULL){ //find first, transmit later
		findRoute_RT(p->des_addr);
		WaitingNode* wp = add_Waitinglist(1, SEQ, p->des_addr, My_addr);
		SEQ++;
		uint8_t* savedpackage=(uint8_t*)malloc(sizeof(MeshPackage)+p->length);
		*savedpackage = *(uint8_t)p;
		wp->package = savedpackage;
	}
	else{ //the route is in the route table, transmit
		Mesh_Send(*p, p+sizeof(MeshPackage), p->length);
		//transmit directly without waiting for the reply
	}
}

//int Mesh_RouteFound(uint32_t destination){ //construct the route and return the next hop's ip, false return -1
//    int flag_found=0;
//    for(int i=0; i<EntryNumber; i++){
//        if(RouteTable[i].des_addr==destination){
//            //the destination is in the route table
//            if(RouteTable[i].flag>0){
//                //transmit the message
//            }
//            else{
//                if(Mesh_RouteMaintenance(destination)){//upload the route
//                    //transmit the message
//                } 
//                else{
//                    return 0; //maintenance failed
//                }
//            }
//            flag_found=1;
//            return 1; //transmit success
//        }
//    }
//    if(flag_found==0){
//        //no route in the route table
//        if(Mesh_Construct(destination)){//construct success
//            //transmit the message
//            return 1;
//        }
//        else{
//            return 0;
//        }
//    }
//}

//uint8_t Mesh_Construct(uint32_t destination){
//    //
//}

//uint8_t Mesh_RouteMaintenance(){ //the old route is out of date, reconstruct a route
//    //
//}

//uint8_t Mesh_Join(){

//}

void init_Route(){
	init_RT();
	init_Waitinglist();
}

