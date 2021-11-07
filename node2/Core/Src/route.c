#include "route.h"
#include "main.h"
#include "ranging.h"
#include "stdint.h"

uint32_t My_addr;

int EntryNumber=0;
// int RTindex[64];
// RT_Entry* RouteTable[64]; 
//RT_Entry* RTTail;
RT_Entry* RTHead;
int WaitingNumber=0;
WaitingNode* Waitinglist;
uint32_t reply_ip;
uint16_t SEQ=1; //自己对包的编号，保证waiting无重复
extern DeviceStates_t DeviceState;
extern uint8_t RxDoneFlag;
extern uint8_t RxData[];
extern uint8_t RxSize;
//初始化等待列表，哨兵结点

void Resend_Waintinglist(){
	printf("Resend %d packages\n", WaitingNumber);
	WaitingNode* p=Waitinglist->next;
	WaitingNode* q=Waitinglist;
	while(p!=NULL){
		printf("resend p in waitinglist\n");
		p->resendNumber++;
		if(p->resendNumber>10){
			printf("Waiting to node%d with type %d can't be recieved\n", p->package->des_addr, p->package->type);
			if(p->required_addr!=My_addr){
				MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
				reply->type=4;
				reply->length=0;
				reply->des_addr=p->required_addr;
				reply->hop_addr=p->required_addr;
				reply->src_addr=My_addr;
				reply->ttl=0;
				reply->ack=p->pseq;
				reply->seq=SEQ++;
				reply->hops=0;
				Mesh_Send(reply);
				free(reply);
				if(p->package) free(p->package);
				if(p->packageTOsend) free(p->packageTOsend);
			}
			q->next=p->next;
			free(p);
			p=q->next;
			WaitingNumber--;
			printf("Resend_Waintinglist: WaitingNumber--, =%d\n", WaitingNumber);
		}
		else{
			if(p->package->type==3){
				printf("Repeat finding node%d for %d times: ",p->package->des_addr, p->resendNumber);
				Mesh_Send(p->package);
			}
			else if(p->package->type==2){
				printf("Repeat transmiting node%d for %d times: ",p->package->des_addr, p->resendNumber);
				Mesh_Send(p->package);
			}
			else{
				printf("Undefined p type = %d\n", p->package->type);
			}
			p=p->next;
			q=q->next;
		}
		
	}
}

void init_Waitinglist(){ 
	Waitinglist=(WaitingNode*)malloc(sizeof(WaitingNode));
	Waitinglist->next=NULL;
	Waitinglist->package=NULL;
	Waitinglist->packageTOsend=NULL;
	Waitinglist->pseq=0;
	Waitinglist->required_addr=0;
	Waitinglist->resendNumber=0;
	WaitingNumber=0;
}
//添加等待任务
WaitingNode* add_Waitinglist(uint16_t pseq, uint32_t required_addr, MeshPackage* psend, MeshPackage* pTOsend){
	printf("Add waiting list: listening to node%d\n", psend->des_addr);
	WaitingNode* p=(WaitingNode*)malloc(sizeof(WaitingNode));
	p->next=Waitinglist->next;
	Waitinglist->next=p;
	p->resendNumber=0;
	p->pseq=pseq;
	p->required_addr=required_addr;
	p->package=psend;
	p->packageTOsend=pTOsend;
	WaitingNumber++;
	printf("add_Waitinglist: WaitingNumber++, =%d\n", WaitingNumber);
	Waitinglist->next=p;
	return p;
}
//发送数据包
void Mesh_Send(MeshPackage* package){
	printf("In Mesh_send: send data=%s, legth=%d\n", ((char*)package+sizeof(MeshPackage)), package->length);
	LoRaSendData((uint8_t*)package, sizeof(MeshPackage)+package->length);
}
//查找路由
MeshPackage* findRoute_RT(uint32_t des_addr, uint32_t require_addr, uint16_t hops){ //发送路由查找包
	printf("Find route to node%d\n", des_addr);
	MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=3;
	p->length=0;
	p->hop_addr=My_addr;
	p->des_addr=des_addr;
	p->src_addr=require_addr; //请求路由的客户地址
	p->ttl=0;
	p->ack=0;
	p->seq=SEQ++; //each waiting package must have a unique seq
	p->hops=hops;
	Mesh_Send(p);
	return p;
}
// 初始化路由表
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
// 获取路由路径信息
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
// 删除路由路径
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
	if(p!=NULL){
		q->next=p->next;
		free(p);
			EntryNumber--;
	}
}
// 添加路由路径
void add_RT(uint32_t des, uint32_t nextHop, uint16_t numHops, uint16_t fresh){
	printf("Add Route: to des node%d, having %d hops, next_hop is node%d\n", des, numHops, nextHop);
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
		printf("Route table has %d entrys\n", EntryNumber);
	}
	else{
		if(p->Freshness<fresh){ // update
			p->num_hops=numHops;
			p->next_hop=nextHop;
			p->num_hops=numHops;
		}
	}
}
// 清除某一结点为中继的路由线路
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
// 清除路由表
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
// 获取路由路径数量
int numofRT_RT(){
	return EntryNumber;
}
// 回复加入网络申请，未完成
uint8_t Mesh_Reply_Join(MeshPackage* package){
	MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=1;
	p->length=0;
	p->hop_addr=package->hop_addr;
	p->src_addr=My_addr;
	p->ttl=0;
	p->ack=package->seq;
	p->seq=package->seq;
	p->hops=0;
	Mesh_Send(p);
}
// 处理回复请求
uint8_t Mesh_Handle_Reply(MeshPackage* package){
	WaitingNode* p=Waitinglist->next;
	WaitingNode* q=Waitinglist;
	while(p!=NULL){
		if(package->src_addr==p->package->des_addr && package->ack==p->package->seq) //正确应答包
		{
			if(p->package->type==2) //发送消息的应答包，证明对方收到
			{
				printf("Message to node%d has been received successfully\n", package->src_addr);
				q->next=p->next;
				free(p);
				return 1;
			}
			else if(p->package->type==3) //收到广播包回复
			{
				printf("BROADCAST to find node%d has been replied, add route path with %d hops\n", package->src_addr, package->hops);
				//添加路由
				add_RT(p->package->des_addr, package->hop_addr, package->hops, 0);
				if(p->required_addr!=My_addr) //有上家需要回复
				{
					printf("Need to reply to node%d: I find the node%d\n", p->package->des_addr, p->package->src_addr);
					//回复上家的广播
					MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
					reply->type=1;
					reply->length=0;
					reply->des_addr=p->required_addr;
					reply->hop_addr=p->required_addr;
					reply->src_addr=My_addr;
					reply->ttl=0;
					reply->ack=p->pseq;
					reply->seq=SEQ++;
					reply->hops=0;
					Mesh_Send(reply);
					free(reply);
				}
				if(p->packageTOsend!=NULL) //有包待转发
				{
					printf("Continue to transmit package to node%d\n", p->packageTOsend->des_addr);
					Mesh_transmit(p->packageTOsend);
					add_Waitinglist(p->packageTOsend->seq, My_addr, p->packageTOsend, NULL);
					if(q==Waitinglist) q=q->next;
				}
				q->next=p->next;
				free(p);
			}
			
			WaitingNumber--;
			printf("Mesh_Handle_Reply: WaitingNumber--, =%d\n", WaitingNumber);
			break;
		}
		else{
			p=p->next;
			q=q->next;
		}
		
	}
	return 0;
}
// 处理广播包
uint8_t Mesh_Handle_Broadcast(MeshPackage* package){
	if(package->des_addr==My_addr){
		printf("Broadcast from node%d, is finding me, reply\n", package->src_addr);
		//return package
		MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
		reply->type=1;
		reply->length=0;
		reply->des_addr=package->hop_addr;
		reply->hop_addr=My_addr;
		reply->src_addr=My_addr;
		reply->ttl=0;
		reply->ack=package->seq;
		reply->seq=SEQ;
		reply->hops=0;
		Mesh_Send(reply);
		free(reply);
	}
	else{
		RT_Entry* rt=get_RT(package->des_addr);
		if(rt!=NULL) //路由表中有信息，无需查找
		{
			printf("There is route to node%d in route table, next hop is node%d, has %d hops\n", rt->des_addr, rt->next_hop, rt->num_hops);
			//回复广播
			MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
			reply->type=1;
			reply->length=0;
			reply->des_addr=package->hop_addr;
			reply->hop_addr=package->hop_addr;
			reply->src_addr=My_addr;
			reply->ttl=0;
			reply->ack=package->seq;
			reply->seq=SEQ;
			reply->hops=rt->num_hops++;
			Mesh_Send(reply);
			free(reply);
		}
		else //需要查找
		{
			printf("No route to node%d, broadcast to find route\n", package->des_addr);
			MeshPackage* broadcastpackage=findRoute_RT(package->des_addr, package->src_addr, package->hops+1);
			// 添加等待列表
			add_Waitinglist(package->seq,package->src_addr,broadcastpackage, NULL);
		}
		
	}
	add_RT(package->hop_addr, package->hop_addr, 0, 0);
	add_RT(package->src_addr, package->hop_addr, package->hops, 0);
}

uint8_t Mesh_Reply(MeshPackage* package) //回复消息包
{
	printf("Reply to node%d for message arrival\n", package->src_addr);
	MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
	reply->type=1;
	reply->length=0;
	reply->des_addr=package->src_addr;
	reply->hop_addr=0; //在Mesh_transmit中会被更新为跳转地址
	reply->src_addr=My_addr;
	reply->ttl=0;
	reply->ack=package->seq;
	reply->seq=SEQ++;
	reply->hops=0;
	Mesh_transmit(reply);
	free(reply);
}
//转发包
uint8_t Mesh_transmit(MeshPackage* package) 
{
	RT_Entry*info= get_RT(package->des_addr);
	if(info==NULL){ //未找到路由，进行查找
		printf("Not find route to node%d, start find route\n", package->des_addr);
		MeshPackage* broadcastpackage=findRoute_RT(package->des_addr, package->src_addr, package->hops+1);
		add_Waitinglist(package->seq, package->src_addr, broadcastpackage, package);
	}
	else{ //直接转发
		printf("Transmit message from node%d to node%d\n", package->src_addr, package->des_addr);
		package->hop_addr=info->next_hop;
		package->hops++;
		Mesh_Send(package);
	}
}

uint8_t Mesh_sendMessage(uint32_t des_addr, char* message, int length){
	MeshPackage* package=(MeshPackage*)malloc(sizeof(MeshPackage)+length);
	package->type=2;
	package->length=length;
	package->hop_addr=0;
	package->src_addr=My_addr;
	package->des_addr=des_addr;
	package->ttl=0;
	package->ack=0;
	package->seq=SEQ;
	package->hops=0;
	memcpy((char*)package+sizeof(MeshPackage), message, length);
	RT_Entry*info= get_RT(des_addr);
	if(info==NULL){ //未找到路由，进行查找
		printf("Not find route to node%d, start find route\n", des_addr);
		MeshPackage* broadcastpackage=findRoute_RT(des_addr, My_addr, 0);
		add_Waitinglist(0, My_addr, broadcastpackage, package);
	}
	else{ //直接转发
		printf("Has route, send message to node%d\n", des_addr);
		package->hop_addr=info->next_hop;
		Mesh_Send(package);
		add_Waitinglist(0, My_addr, package, NULL);
	}
}	
// 初始化路由表
void init_Route(){
	My_addr=2;
	init_RT();
	init_Waitinglist();
}

