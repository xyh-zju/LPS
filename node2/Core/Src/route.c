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
void init_Waitinglist(){ 
	Waitinglist=(WaitingNode*)malloc(sizeof(WaitingNode));
	Waitinglist->next=NULL;
	Waitinglist->seq=0;
	Waitinglist->pseq=0;
	Waitinglist->type=0;
	Waitinglist->des_addr=0;
	Waitinglist->require_addr=0;
	Waitinglist->resendNumber=0;
	Waitinglist->package_stream=NULL;
	WaitingNumber=0;
}
//添加等待任务
WaitingNode* add_Waitinglist(uint8_t type, uint16_t seq, uint16_t pseq, uint32_t des_addr, uint32_t require_addr, uint8_t* package_stream){
	WaitingNode* p=(WaitingNode*)malloc(sizeof(WaitingNode));
	p->next=Waitinglist->next;
	p->seq=seq;
	p->pseq=pseq;
	p->type=type;
	p->des_addr=des_addr;
	p->require_addr=require_addr;
	p->resendNumber=0;
	p->package_stream=package_stream;
	WaitingNumber++;
	Waitinglist->next=p;
	return p;
}
//发送数据包
void Mesh_Send(MeshPackage* head, char* data, int dataLength){
	printf("In Mesh_send:\n");
	uint8_t* p=(uint8_t*)malloc(sizeof(head)+dataLength);
	*p=*(uint8_t*)head;
	if(data!=NULL){
		uint8_t* q=p+sizeof(head);
		*q=*data; //Assemble message package
	}
	LoRaSendData(p, sizeof(head)+dataLength);
	LoRaSetRx();
}
//查找路由
void findRoute_RT(uint32_t des_addr, uint32_t require_addr, uint16_t hops){ //发送路由查找包
	MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=3;
	p->length=0;
	p->hop_addr=My_addr;
	p->des_addr=des_addr;
	p->src_addr=require_addr; //请求路由的客户地址
	p->ttl=0;
	p->ack=0;
	p->seq=SEQ; //each waiting package must have a unique seq
	p->hops=hops;
	Mesh_Send(p, NULL, 0);
	free(p);
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
	Mesh_Send(p, NULL, 0);
}
// 处理回复请求
uint8_t Mesh_Handle_Reply(MeshPackage* package){
	WaitingNode* p=Waitinglist->next;
	WaitingNode* q=Waitinglist;
	while(p!=NULL){
		if(package->ack==p->seq) //正确应答包
		{
			if(p->type==2) //发送消息的应答包，证明对方收到
			{
				q->next=p->next;
				free(p);
				printf("Message received successfully\n");
				return 1;
			}
			else if(p->type==1) //收到广播包回复
			{
				//添加路由
				add_RT(p->des_addr, package->src_addr, package->hops, 0);
				if(p->require_addr!=My_addr) //有上家需要回复
				{
					//回复上家的广播
					MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
					reply->type=1;
					reply->length=0;
					reply->des_addr=p->require_addr;
					reply->hop_addr=p->require_addr;
					reply->src_addr=My_addr;
					reply->ttl=0;
					reply->ack=p->pseq;
					reply->seq=p->seq;
					reply->hops=0;
					Mesh_Send(reply, NULL, reply->length);
					free(reply);
				}
				if(p->package_stream!=NULL) //有包待转发
				{
					Mesh_transmit((MeshPackage*)p->package_stream);
					free(p->package_stream);
				}
			}
		}
	}
	return 0;
}
// 处理广播包
uint8_t Mesh_Handle_Broadcast(MeshPackage* package){
	RT_Entry* rt=get_RT(package->des_addr);
	if(rt!=NULL) //路由表中有信息，无需查找
	{
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
		Mesh_Send(reply, NULL, reply->length);
		free(reply);
	}
	else //需要查找
	{
		findRoute_RT(package->des_addr, package->src_addr, package->hops+1);
		uint8_t* savedpackage=(uint8_t*)malloc(sizeof(MeshPackage)+package->length); //创建数据包流
		*savedpackage = *(uint8_t*)package;
		// 添加等待列表
		add_Waitinglist(1, SEQ, package->seq, 0, package->src_addr, savedpackage);
	}
}

uint8_t Mesh_Reply(MeshPackage* package) //回复消息包
{
	MeshPackage* reply=(MeshPackage*)malloc(sizeof(MeshPackage));
	reply->type=1;
	reply->length=0;
	reply->des_addr=package->src_addr;
	reply->hop_addr=0; //在Mesh_transmit中会被更新为跳转地址
	reply->src_addr=My_addr;
	reply->ttl=0;
	reply->ack=package->seq;
	reply->seq=SEQ;
	reply->hops=0;
	Mesh_transmit(reply);
	free(reply);
}
//转发包
uint8_t Mesh_transmit(MeshPackage* package) 
{
	RT_Entry*info= get_RT(package->des_addr);
	if(info==NULL){ //未找到路由，进行查找
		findRoute_RT(package->des_addr, package->src_addr, package->hops+1);
		uint8_t* savedpackage=(uint8_t*)malloc(sizeof(MeshPackage)+package->length); //创建数据包流
		*savedpackage = *(uint8_t*)package;
		add_Waitinglist(1, SEQ, package->seq, 0, package->src_addr, savedpackage);
	}
	else{ //直接转发
		package->hop_addr=info->next_hop;
		package->hops++;
		Mesh_Send(package, (char* )(package+sizeof(MeshPackage)), package->length);
	}
}
// 初始化路由表
void init_Route(){
	My_addr=2;
	init_RT();
	init_Waitinglist();
}

