#include "global.h"
#include "stack.h"
#include "xstack.h"
#include "scheduler.h"
#include "bit_ops.h"
#include "timer0_isr.h"
#include "syscall.h"
#include "semaphore.h"
#include "events.h"
#include "seg_led.h"
#include "button.h"
#include "usbcom.h"
#include "rs485.h"
#include "random.h"
#include "ds1302.h"

#define TIMESLICE_MS	1
#define T12RL	(65536 - MAIN_Fosc*TIMESLICE_MS/12/1000)

void testproc0() large reentrant{
	while(1){
		proc_sleep(1000);
	}
}

void testproc(u16 param) large reentrant
{
	while(1)
	{
		if((timer0_cnt>>5) & BIT(param))
		{
			SETBIT(led_display_content, param);
		}
		else
		{
			CLEARBIT(led_display_content, param);
		}
	}
}

void testproc2(u16 param) large reentrant
{
	while(1)
	{
		proc_sleep(param);
		led_display_content = ~led_display_content;
	}
}

void testproc3(u16 param) large reentrant
{
	while(1)
	{
		proc_sleep(param);
		led_display_content ^= 0x0f;
	}
}

void testproc4() large reentrant
{
	while(1)
	{
		proc_sleep(500);
		sem_post(0);
		led_display_content |= 0x80;
		sem_wait(0);
		led_display_content &= ~0x80;
	}
}

void testproc5() large reentrant
{
	sem_init(0,0);
	while(1)
	{
		sem_wait(0);
		led_display_content |= 0x40;
		proc_sleep(500);
		sem_post(0);
		led_display_content &= ~0x40;
	}
}

void testproc6(u16 param) large reentrant
{
	while(1)
	{
		proc_wait_evts(EVT_BTN1_DN);
		seg_set_str("HELLO  ");
		usbcom_write("hello \0",0);
		proc_wait_evts(EVT_NAV_R);
		seg_set_str("WORLD  ");
		usbcom_write("world\r\n\0",0);
		proc_wait_evts(EVT_UART1_RECV);
		seg_set_str(usbcom_buf);
	}
}

void testproc7(u16 param) large reentrant
{
	while(1)
	{
		proc_wait_evts(EVT_UART2_RECV | EVT_BTN1_DN);
		if(MY_EVENTS & EVT_BTN1_DN)
		{
			*((u32 *)rs485_buf) = rand32();
			rs485_write(rs485_buf, 4);
			seg_set_number(*((u32 *)rs485_buf));
		}
		else
		{
			seg_set_number(*((u32 *)rs485_buf));
		}
	}
}
XDATA u8 debug_pid1[50];
XDATA volatile u8 debug_pid1_count = 0;
XDATA u8 debug_pid2[50];
XDATA volatile u8 debug_pid2_count = 0;



//蜂鸣器
sbit BZ = P3^4;

#define MAX_PLAYERS       4  //最大玩家数
#define HAND_SIZE         5  //每个人手牌数
#define BULLET_SLOTS      3  //手枪槽位
#define CARD_POOL_SIZE    20 //总牌堆大小
#define JOKER_CARD        'J' //小丑牌

//牌堆
CODE char CARD_POOL[CARD_POOL_SIZE] = {
	  'A', 'A', 'A', 'A', 'A', 'A',
    '2', '2', '2', '2', '2', '2',
	  '3', '3', '3', '3', '3', '3',
    JOKER_CARD, JOKER_CARD
};

#define PACKET_SIZE 9 // 9字节数据包
#define ID_BROADCAST 0xFF // 广播地址（发给所有人）
#define ID_MASTER    0x00 // 主机地址（固定为0）
/*
8位数据包格式如下
0：目标
1：源
2：命令
3~7：数据
*/


//数据包命令 Master为主机 Slave为从机 D1-D5为具体数据
/* 改为手动分配
#define CMD_I_AM_MASTER     0x01 // 选举主机
#define CMD_ASSIGN_ID       0x02 // (M->S) 主机分配ID
*/
#define CMD_DEAL_HAND       0x03 // (M->S) 主机发D1-D5手牌
#define CMD_NEW_ROUND       0x04 // (M->S) 主机宣布回合开始 叫牌D1
#define CMD_SET_TURN        0x05 // (M->S) 主机宣布轮到D1玩家
#define CMD_PUBLIC_CLAIM    0x06 // (M->S) 主机公布D1玩家叫牌D2张
#define CMD_CLAIM           0x10 // (S->M) 玩家剩余D1张牌 D2-D4为实际打出的牌
#define CMD_CHALLENGE       0x11 // (S->M) 玩家质疑
#define CMD_SHOWDOWN        0x12 // (M->S) 主机展示D1结果（0假1真） 公布输家D2 真实ANS D3
#define CMD_PUNISHMENT      0x13 // (M->S) 主机命令玩家D1开枪
#define CMD_FIRE_WEAPON     0x14 // (S->M) 玩家D1开枪
#define CMD_PUNISH_RESULT   0x15 // (M->S) 主机公布D1是否命中（1命中） 输家D2 剩余D3子弹
#define CMD_PLAYER_DEAD     0x16 // (M->S) 主机公布玩家D1死亡
#define CMD_GAME_WIN        0x17 // (M->S) 主机公布D1胜利
#define CMD_GAME_STUCK      0x18 // (S->M) 从机没牌了 请求新的一轮
//大厅命令
#define CMD_PLAYER_READY    0x20 // (S->M) 从机通知主机：我D1已经准备
#define CMD_PLAYER_CANCEL   0x21 // (S->M) 从机通知主机：我D1取消准备

//按键状态 由于按键较少 不同按键在不同状态承担的功能不同
enum {
	STATE_ID_SELECT,        // 0 选择ID
	STATE_IDLE_WAIT_TURN,   // 1 空闲
	STATE_MY_TURN_CHALLENGE,// 2 质疑上家
	STATE_MY_TURN_SELECT,   // 3 选择出牌
	STATE_WAIT_RESULT,      // 4 等待结果
	STATE_PUNISHMENT,       // 5 等待开枪
	STATE_PLAYER_DEAD,      // 6 玩家死亡
	STATE_GAME_OVER,				// 7 游戏结束
	STATE_NEW_ROUND_ANNOUNCE,   // 8 新回合通知
	STATE_PUBLIC_CLAIM_ANNOUNCE,// 9 玩家出牌通知
	STATE_SHOW_CARDS_ANNOUNCE,  // 10 摊牌 宣布结果
	STATE_SHOW_LOSER_ANNOUNCE,  // 11 摊牌 宣布输家
	STATE_SURVIVE_ANNOUNCE,			// 12 开枪后宣布存活
	STATE_DEAD_ANNOUNCE,				// 13 开枪后广播死亡
	//大厅新增状态
	STATE_LOBBY_IDLE,					// 14 选好ID，等待按K1准备
	STATE_LOBBY_WAIT_SLAVE,		// 15 从机已准备，等待主机开始
	STATE_LOBBY_WAIT_MASTER,	// 16 主机已准备，等待其他玩家
};

//全局变量
XDATA u8 my_id = 0xFF;              			     // 玩家ID 0为Master
XDATA u8 current_state = STATE_ID_SELECT;			 // 游戏状态
XDATA char my_hand[HAND_SIZE];         				 // 玩家手上的牌
XDATA u8 my_hand_count = 0;				 	    			 // 当前手牌数
XDATA u8 player_bullet_counts[MAX_PLAYERS];    // 所有玩家剩余子弹数
XDATA char round_card = ' ';          			   // 主机公布的本回合叫牌
XDATA u8 last_claim_pid = 0;            			 // 上一个玩家叫牌玩家ID
XDATA u8 last_claim_count = 0;								 // 上一个玩家叫牌数量
XDATA u8 display_mode = 0;          			     // 模式切换 0为隐藏模式 1为显示模式
XDATA u8 cursor_pos = 0;             			     // 光标 用于标明当前选牌
XDATA u8 selected_cards = 0;        			     // 选牌位图
XDATA u8 player_alive = 0xFF;      			       // 存活玩家位图
XDATA u8 current_turn_id = 0;      			       // 当前轮到谁
XDATA char display_buffer[9];       			     // 数码管显示缓冲
XDATA u8 packet_buffer[PACKET_SIZE]; 			     // 485发送缓冲
XDATA u8 display_param_global = 0;						 //专门给显示进程传递数据
XDATA u8 master_fatal_shot[MAX_PLAYERS];			 //致命子弹

//仅主机使用变量 存储所有人的手牌用于裁决
XDATA char master_all_hands[MAX_PLAYERS][HAND_SIZE]; //所有人手牌
XDATA char master_played_cards[MAX_PLAYERS][HAND_SIZE];// 上一个玩家打出的牌
XDATA u8 last_challenger_id = 0xFF; //记录谁发起的质疑
XDATA u8 game_round_count = 0; //游戏轮次（主机）
XDATA u8 current_round_display = 0; //用于显示的游戏轮次（所有玩家）
XDATA char showdown_cards[3]; //摊牌的卡片
XDATA u8 showdown_loser_id; //摊牌时的输家id
XDATA u8 dead_player_id; //刚刚死去的玩家
XDATA u8 game_winner_id = 0xFF; //游戏胜利者ID


void game_handle_packet() large reentrant;
void game_handle_buttons() large reentrant;
void master_start_new_round() large reentrant;
void master_handle_claim(u8 source_id, u8 d1, u8 d2, u8 d3, u8 d4, u8 d5) large reentrant;
void master_resolve_challenge(u8 challenger_id) large reentrant;
void master_resolve_punishment(u8 loser_id) large reentrant;
void game_send_claim() large reentrant;
u8 count_set_bits(u8 mask);


//发送数据包
void game_send_packet(u8 to_id, u8 cmd, u8 d1, u8 d2, u8 d3, u8 d4, u8 d5){
	XDATA u8 i;
  XDATA u8 checksum = 0;
	
	packet_buffer[0] = to_id;    // 发给谁
	packet_buffer[1] = my_id;    // 谁发的
	packet_buffer[2] = cmd;      // 命令
	packet_buffer[3] = d1;       // 数据
	packet_buffer[4] = d2;
	packet_buffer[5] = d3;
	packet_buffer[6] = d4;
	packet_buffer[7] = d5;
	
	for(i = 0; i < (PACKET_SIZE - 1); i++)
    checksum ^= packet_buffer[i];
	
	packet_buffer[PACKET_SIZE - 1] = checksum;
	
	rs485_write(packet_buffer, PACKET_SIZE);
	proc_sleep(5);
}

//蜂鸣器响
void beep_os(u16 duration_ms) large reentrant{
	u16 i;
	//推挽
	P3M1 &= 0xEF; // P3.4 M1=0
	P3M0 |= 0x10; // P3.4 M0=1
   
	for (i = 0; i < (duration_ms / 2); i++){
		//BZ = 1;
		proc_sleep(1);
		//BZ = 0;
		proc_sleep(1);
	}
	BZ = 0;
}

//用于统计位图中1的数量 计算存活玩家
u8 count_set_bits(u8 mask) {
    u8 count = 0;
    u8 i;
    for (i = 0; i < 8; i++) {
        if (GETBIT(mask, i)) {
            count++;
        }
    }
    return count;
}

//打包并发送叫牌信息
void game_send_claim() large reentrant {
	u8 claim_count = count_set_bits(selected_cards);
	u8 d_data[5];
	u8 i;
	u8 d_index = 1; // D1为剩余手牌数目 D2-4为牌

	XDATA char new_hand[HAND_SIZE];
	u8 new_count = 0;

	d_data[1] = ' ';
	d_data[2] = ' ';
	d_data[3] = ' ';
	d_data[4] = ' ';

	// 分离打出的牌和剩下的牌
	for (i = 0; i < my_hand_count; i++) {
		if (GETBIT(selected_cards, i)) {
			// 这个牌被选中了
			if(d_index < 4) {
				d_data[d_index] = my_hand[i];
				d_index++;
			}
		}
		else {
			// 没被选中
			if(new_count < HAND_SIZE) {
				new_hand[new_count] = my_hand[i];
				new_count++;
			}
		}
	}

	// 更新自己的手排
	my_hand_count = new_count;
	for(i=0; i < my_hand_count; i++)
			my_hand[i] = new_hand[i];
	// 清空剩余部分 让数码管不亮
	for(i=my_hand_count; i < HAND_SIZE; i++)
			my_hand[i] = ' ';

	// D1剩余手牌数
	d_data[0] = my_hand_count;

	//数据包 剩余手牌数+打出了哪些牌
	if (my_id == ID_MASTER)
		master_handle_claim(my_id, d_data[0], d_data[1], d_data[2], d_data[3], 0);
	else{
		game_send_packet(ID_MASTER, CMD_CLAIM, 
				d_data[0], d_data[1], d_data[2], d_data[3], 0);
	}
		
	// 重置为等待状态
	selected_cards = 0;
	cursor_pos = 0;
	current_state = STATE_IDLE_WAIT_TURN;
}

//按键处理
void game_handle_buttons() large reentrant {
	u8 selected_count;
	u8 i;

	// K2专门非大厅模式转换显示模式
	if (MY_EVENTS & EVT_BTN2_DN){
		if ((current_state != STATE_LOBBY_IDLE) &&
				(current_state != STATE_LOBBY_WAIT_SLAVE) &&
				(current_state != STATE_LOBBY_WAIT_MASTER))
			display_mode = !display_mode;
	}

	// 根据当前状态处理按键
	switch (current_state) {
		case STATE_MY_TURN_CHALLENGE:
			// K1 质疑
			if (MY_EVENTS & EVT_BTN1_DN) {
				if(my_id == ID_MASTER)
					// Master本地处理，不发送网络包
					master_resolve_challenge(my_id);
				else{// Slave发送质疑包
					game_send_packet(ID_MASTER, CMD_CHALLENGE, 0, 0, 0, 0, 0);
					current_state = STATE_WAIT_RESULT;
				}
			}
			// 遥感按下放弃质疑 进入选牌
			if (MY_EVENTS & EVT_NAV_PUSH) {
				if (my_hand_count == 0) {
					//没牌了
					if (my_id == ID_MASTER) {
						proc_sleep(500);
						master_start_new_round();
					}
					else{
						//从机请求
						game_send_packet(ID_MASTER, CMD_GAME_STUCK, 0, 0, 0, 0, 0);
						current_state = STATE_IDLE_WAIT_TURN;
					}
				}
				else{
					selected_cards = 0;
					cursor_pos = 0;
					current_state = STATE_MY_TURN_SELECT;
				}
			}
			break;

		case STATE_MY_TURN_SELECT:
			selected_count = count_set_bits(selected_cards);
			//左边遥感
			if (MY_EVENTS & EVT_NAV_L)
					if (cursor_pos > 0) cursor_pos--;
			//右边遥感
			if (MY_EVENTS & EVT_NAV_R)
					if (cursor_pos < (my_hand_count - 1)) cursor_pos++;
			//上遥感 选择
			if (MY_EVENTS & EVT_NAV_U) {
				//最多选3张
				if (selected_count < 3){
					//选中光标指向数
					SETBIT(selected_cards, cursor_pos);
				}
				else{
					//已达上限
					beep_os(50);
				}
			}
			// 下遥感 取消选择
			if (MY_EVENTS & EVT_NAV_D)
					CLEARBIT(selected_cards, cursor_pos);

			// K1 确认出牌
			if (MY_EVENTS & EVT_BTN1_DN) {
				selected_count = count_set_bits(selected_cards);
				if (selected_count == 0)
					// 至少要选一张牌
					beep_os(50);
				else if(selected_count > 3)
					// 理论上不会发生
					beep_os(50);
				else
					// 叫牌
					game_send_claim();
			}
			break;

		case STATE_PUNISHMENT:
			// K1 开枪
			if (MY_EVENTS & (EVT_BTN1_DN)) {
				if (my_id == ID_MASTER) {//主机输了
					current_state = STATE_WAIT_RESULT;
					master_resolve_punishment(my_id);
				}
				else{
					game_send_packet(ID_MASTER, CMD_FIRE_WEAPON, my_id, 0, 0, 0, 0);
					current_state = STATE_WAIT_RESULT;
				}
			}
			break;
			
		//大厅逻辑
		case STATE_LOBBY_IDLE: // 当前未准备
			if (MY_EVENTS & EVT_BTN1_DN) {
				if (my_id == ID_MASTER) {
					// 主机按下准备
					current_state = STATE_LOBBY_WAIT_MASTER;
					player_alive = BIT(my_id); // 主机进入准备列表
				}
				else {
					// 从机按下准备，发包给主机
					game_send_packet(ID_MASTER, CMD_PLAYER_READY, my_id, 0, 0, 0, 0);
					current_state = STATE_LOBBY_WAIT_SLAVE;
				}
				beep_os(50);
			}
			// K2 返回ID选择
			if (MY_EVENTS & EVT_BTN2_DN) {
				my_id = 0xFF; // 重置ID
				current_state = STATE_ID_SELECT; // 返回选择界面
			}
			break;

		case STATE_LOBBY_WAIT_SLAVE: // 从机已准备
			if (MY_EVENTS & EVT_BTN1_DN) {
				// 从机取消准备
				game_send_packet(ID_MASTER, CMD_PLAYER_CANCEL, my_id, 0, 0, 0, 0);
				current_state = STATE_LOBBY_IDLE;
				beep_os(50);
			}
			break;
		
		case STATE_LOBBY_WAIT_MASTER: // 主机已准备
			if (MY_EVENTS & EVT_BTN1_DN) {
				// 主机按K1 根据人数决定开始或取消
				selected_count = count_set_bits(player_alive);
				if (selected_count >= 2) {
					// 人数够，开始游戏
					// 初始化所有玩家子弹 以及致命子弹
					for(i=0; i<MAX_PLAYERS; i++) {
						if(GETBIT(player_alive, i)) { // 只初始化准备了的玩家
							player_bullet_counts[i] = BULLET_SLOTS;
							master_fatal_shot[i] = rand32() % BULLET_SLOTS;
						}
					}
					//初始化轮数
					game_round_count = 0;
					//新一轮开始 (master_start_new_round 将使用 player_alive 来发牌)
					master_start_new_round();
				}
				else {
					// 人数不够，主机取消准备
					player_alive = 0;
					current_state = STATE_LOBBY_IDLE;
					beep_os(50);
				}
			}
			break;
	}
}



// 处理485总线上的数据包
void game_handle_packet() large reentrant {
	XDATA u8 i;
  XDATA u8 checksum = 0;
	
	u8 target_id = rs485_buf[0];
	u8 source_id = rs485_buf[1];
	u8 cmd = rs485_buf[2];
	u8 d1 = rs485_buf[3];
	u8 d2 = rs485_buf[4];
	u8 d3 = rs485_buf[5];
	u8 d4 = rs485_buf[6];
	u8 d5 = rs485_buf[7];
	
	for(i = 0; i < (PACKET_SIZE - 1); i++){
    checksum ^= rs485_buf[i];
	}
	
	if (checksum != rs485_buf[PACKET_SIZE - 1]){
		return;
	}
	// 不是发给本机的数据包
	if (target_id != my_id && target_id != ID_BROADCAST)
		return;
	
	//主机处理
	if (my_id == ID_MASTER) {
		// 主机只处理来自从机的请求
		if (source_id != ID_MASTER) {
			// 如果游戏已经结束了 不要处理来自从机的任何请求
			if (current_state == STATE_GAME_OVER)
				return;
			switch (cmd) {
				case CMD_CLAIM: // 从机出牌
					master_handle_claim(source_id, d1, d2, d3, d4, d5);
					return;
				case CMD_CHALLENGE: // 从机质疑
					master_resolve_challenge(source_id);
					return;
				case CMD_FIRE_WEAPON: // 从机开枪
					master_resolve_punishment(source_id);
					return;
				case CMD_GAME_STUCK:
					proc_sleep(500);
					master_start_new_round();
					return;
				//大厅逻辑
				case CMD_PLAYER_READY: // 从机准备
					// 只有主机在等待状态才接收
					if (current_state == STATE_LOBBY_WAIT_MASTER) {
						SETBIT(player_alive, d1); // d1 里存的是从机ID
						beep_os(20);
					}
					return;
				
				case CMD_PLAYER_CANCEL: // 从机取消准备
					if (current_state == STATE_LOBBY_WAIT_MASTER) {
						CLEARBIT(player_alive, d1);
						beep_os(20);
					}
					return;
			}
		}
	}
	// 只处理来自主机的命令 包括主机自己发给自己的
	if (source_id != ID_MASTER)
		return;
	//通用广播处理
	switch (cmd) {
		case CMD_DEAL_HAND: // 主机发牌
			if (my_id == ID_MASTER)//主机不需要给自己发牌
				break;
			// 只在等待 刚活下来 轮到我选牌 大厅中 接收手牌
			if(current_state == STATE_IDLE_WAIT_TURN ||
					current_state == STATE_SURVIVE_ANNOUNCE ||
					current_state == STATE_MY_TURN_SELECT ||
					current_state == STATE_LOBBY_WAIT_SLAVE ) {
				my_hand[0] = d1;
				my_hand[1] = d2;
				my_hand[2] = d3;
				my_hand[3] = d4;
				my_hand[4] = d5;
				my_hand_count = HAND_SIZE;
			}
			break;

		case CMD_NEW_ROUND: // 主机宣布新的回合
			// 已死不可复活
			if (current_state == STATE_PLAYER_DEAD)
				break;
		
			round_card = d1;
			last_claim_count = 0;
			last_claim_pid = 0xFF;
			current_round_display = d3;
		
			// 顶替当前状态，设置为NEW RND
			current_state = STATE_NEW_ROUND_ANNOUNCE;
		
			if (current_state != STATE_PLAYER_DEAD){
				//设置状态N RND xx
				current_state = STATE_NEW_ROUND_ANNOUNCE;
				proc_sleep(1000);
				if (d2 == my_id) {//我的回合
					current_state = STATE_MY_TURN_SELECT;
					selected_cards = 0;
					cursor_pos = 0;
				}
				else
					current_state = STATE_IDLE_WAIT_TURN;
			}
			break;

		/*
		case CMD_SET_TURN: // 主机宣布轮到谁 只用于第一回合
			current_turn_id = d1;
			if (current_turn_id == my_id && current_state != STATE_PLAYER_DEAD) {
				// 轮到本机
				current_state = STATE_MY_TURN_CHALLENGE;
			}
			break;

		case CMD_PUBLIC_CLAIM: // 主机叫牌
			last_claim_pid = d1;
			last_claim_count = d2;
			break;
		*/
		case CMD_PUBLIC_CLAIM:
			last_claim_pid = d1;
			last_claim_count = d2;
			if (current_state != STATE_PLAYER_DEAD) {
				current_state = STATE_PUBLIC_CLAIM_ANNOUNCE;
				proc_sleep(1500);
				//显示出牌信息后 再根据是否轮到自己做调整
				if (d4 == my_id)
					current_state = STATE_MY_TURN_CHALLENGE;
				else
					current_state = STATE_IDLE_WAIT_TURN;
			}
			break;

		case CMD_SHOWDOWN: // 公布翻牌结果
			if (current_state == STATE_PLAYER_DEAD)
				break;
			// d1输家id d234为卡牌
			showdown_loser_id = d1;
			showdown_cards[0] = d2;
			showdown_cards[1] = d3;
			showdown_cards[2] = d4;
			
			//公布结果
			current_state = STATE_SHOW_CARDS_ANNOUNCE;
			proc_sleep(2000);
		
			//公布输家
			current_state = STATE_SHOW_LOSER_ANNOUNCE;
			proc_sleep(2000);
		
			if (showdown_loser_id == my_id) {
				// 本机输了
				current_state = STATE_PUNISHMENT;
				beep_os(200);
			}
			else
				// 别人输了
				current_state = STATE_IDLE_WAIT_TURN;
			break;
			
		case CMD_PUNISH_RESULT:
			player_bullet_counts[d2] = d3;
			//只有开枪的人才可能接受存活反馈
			if (d2 == my_id){
				//只处理从机 主机已经在本地处理了
				if (my_id != ID_MASTER) {
					if (d1 == 1){
						// 死了 等待广播
						beep_os(1000);
						current_state = STATE_IDLE_WAIT_TURN;
					}
					else{
						//没死 存活反馈
						beep_os(100);
						current_state = STATE_SURVIVE_ANNOUNCE;
					}
				}
			}
			break;
		/*
		case CMD_PUNISH_RESULT: // 宣布开枪结果
			// d1结果（是否命中） d2输家 d3剩余开火次数
			if (d2 == my_id){
				// 本机结果
				player_bullet_counts[my_id] = d3;//更新剩余子弹
				if (d1 == 1){ 
					// 命中
					current_state = STATE_PLAYER_DEAD;
					beep_os(1000); // DEAD
				}
				else{
					// 存活
					current_state = STATE_IDLE_WAIT_TURN;
					beep_os(100); // LIVE
				}
			}
			else
				// 更新别人的子弹数
				player_bullet_counts[d2] = d3;
			break;
		*/

		case CMD_PLAYER_DEAD: // 主机宣布玩家死亡
			player_alive &= ~BIT(d1); // 更新存活列表
			dead_player_id = d1; //更新死亡玩家
		
			//仅仅死亡时播放
			if (current_state != STATE_PLAYER_DEAD) {
				// 广播死亡2s
				current_state = STATE_DEAD_ANNOUNCE;
				proc_sleep(2000);
			
				if (d1 == my_id)
					// 死的是你
					current_state = STATE_PLAYER_DEAD;
				else
					current_state = STATE_IDLE_WAIT_TURN;
			}
			break;
		
		case CMD_GAME_WIN: // 主机宣布游戏结果 包括死的人也会被宣布
			game_winner_id = d1;
			current_state = STATE_GAME_OVER;
			break;
	}
}

//只能由主机调用 负责洗牌分发
//将牌复制到XDATA数组中 然后随机打乱数组 最后发到master_all_hands
void master_shuffle_deck(){
	u8 i, r;
	char temp;
	//在XDATA创建堆副本
	XDATA char xdata_card_pool[CARD_POOL_SIZE];
	//复制牌堆
	for(i=0; i < CARD_POOL_SIZE; i++)
		xdata_card_pool[i] = CARD_POOL[i];
	
	//洗牌
	for (i = (CARD_POOL_SIZE - 1); i > 0; i--){
		r = rand32() % (i + 1);
		//创造随机数 然后交换
		temp = xdata_card_pool[i];
		xdata_card_pool[i] = xdata_card_pool[r];
		xdata_card_pool[r] = temp;
	}
	
	//将洗好后的牌分配到缓冲
	for (i = 0; i < MAX_PLAYERS; i++){
		u8 p;
		for (p = 0; p < HAND_SIZE; p++){
			//没有考虑玩家死亡 全部都分配手牌
			master_all_hands[i][p] = xdata_card_pool[i * HAND_SIZE + p];
		}
	}
}


// 只能由主机调用 负责开始新的回合
void master_start_new_round() large reentrant {
  u8 i;
	XDATA u8 start_pid;
	
	//轮次加一
	game_round_count++;
	
	current_round_display = game_round_count;
	
	if (game_round_count == 1)
		start_pid = ID_MASTER;
	else{//随机选择
		start_pid = rand32() % MAX_PLAYERS;
		while (!GETBIT(player_alive, start_pid))
			start_pid = (start_pid + 1) % MAX_PLAYERS;
	}
		
	
	//洗牌
	master_shuffle_deck();
	
	// 选一个非JOKER的牌
	round_card = CARD_POOL[rand32() % CARD_POOL_SIZE];
	while(round_card == JOKER_CARD)
		round_card = CARD_POOL[rand32() % CARD_POOL_SIZE];
	
	// 如果主机还活着 给自己发牌
	if (GETBIT(player_alive, ID_MASTER)) {
		my_hand_count = HAND_SIZE;
		for(i=0; i<HAND_SIZE; i++)
			my_hand[i] = master_all_hands[0][i];
	}
	else
		my_hand_count = 0;

	// 主机给从机发牌
	for (i = 1; i < MAX_PLAYERS; i++){
		if (GETBIT(player_alive, i)){ //只给活着的发
			game_send_packet(i, CMD_DEAL_HAND, 
				master_all_hands[i][0], master_all_hands[i][1], 
				master_all_hands[i][2], master_all_hands[i][3], master_all_hands[i][4]);
			
			// 防止拥塞
			proc_sleep(100);
		}
	}
	
	// 主机重置本地游戏状态
	last_claim_count = 0;
	last_claim_pid = 0xFF;
  last_challenger_id = 0xFF;
	current_turn_id = start_pid;
	
	game_send_packet(ID_BROADCAST, CMD_NEW_ROUND, round_card, start_pid, game_round_count, 0, 0);
    
	if (current_state != STATE_PLAYER_DEAD){
		//设置状态为 N RND xx
		current_state = STATE_NEW_ROUND_ANNOUNCE;
		//休眠1秒
		proc_sleep(1000);
		if (start_pid == ID_MASTER) {
			//主机
			current_state = STATE_MY_TURN_SELECT;
			selected_cards = 0;
			cursor_pos = 0;
		}
		else{
			current_state = STATE_IDLE_WAIT_TURN;
		}
	}
}

// 只能由主机调用 负责处理出牌
void master_handle_claim(u8 source_id, u8 d1, u8 d2, u8 d3, u8 d4, u8 d5) large reentrant {
	XDATA u8 i;
	XDATA u8 claim_count = 0; // 实际打出牌的数量
	XDATA u8 next_pid;
	
	// 清空上一次记录
	for(i=0; i < HAND_SIZE; i++)
    master_played_cards[source_id][i] = ' ';
	
	// 读取牌
	master_played_cards[source_id][0] = d2;
	master_played_cards[source_id][1] = d3;
	master_played_cards[source_id][2] = d4;
	
	// 计算玩家打了几牌
	if (d2 != ' ') claim_count++;
	if (d3 != ' ') claim_count++;
	if (d4 != ' ') claim_count++;
	
	//玩家id和牌数
	last_claim_pid = source_id;
  last_claim_count = claim_count;
	
	proc_sleep(100);
	
	//查找下一个活着的玩家
	next_pid = source_id;
	do{
		next_pid = (next_pid + 1) % MAX_PLAYERS;
		if (GETBIT(player_alive, next_pid))
      break;
	}while (next_pid != source_id);
	
	current_turn_id = next_pid;
	
	//玩家出了几张牌
	game_send_packet(ID_BROADCAST, CMD_PUBLIC_CLAIM, source_id, claim_count, round_card, next_pid, 0);
	
	//如果主机还活着
	if (current_state != STATE_PLAYER_DEAD) {
		//玩家除了几张牌
		current_state = STATE_PUBLIC_CLAIM_ANNOUNCE;
		proc_sleep(1500);
		if (next_pid == ID_MASTER)
			//轮到自己
			current_state = STATE_MY_TURN_CHALLENGE;
		else
			current_state = STATE_IDLE_WAIT_TURN;
	}
	/*
	if (next_pid == ID_MASTER) {
		//主机宣布手牌
		current_state = STATE_PUBLIC_CLAIM_ANNOUNCE;
		
		proc_sleep(1500);
		
		if (next_pid == ID_MASTER)
			current_state = STATE_MY_TURN_CHALLENGE;
		else
			current_state = STATE_IDLE_WAIT_TURN;
		if (last_claim_pid != 0xFF) 
		    current_state = STATE_MY_TURN_CHALLENGE;
		else
			current_state = STATE_MY_TURN_SELECT;
	}
	else
		game_send_packet(next_pid, CMD_SET_TURN, next_pid,0, 0, 0, 0);
	*/
	
}

// 只能由主机调用 负责处理玩家的质疑
void master_resolve_challenge(u8 challenger_id) large reentrant {
	XDATA u8 i;
  XDATA u8 is_bluff = 0;      // 上家是否在吹牛
  XDATA u8 loser_id;          // 输家ID
  XDATA u8 true_count = 0;    // 真实牌数
	XDATA char real_cards[3];
	
	//初始化real_cards
	for(i = 0; i < 3; i++)
		real_cards[i] = ' ';
	
	//谁发起的质疑
	last_challenger_id = challenger_id;
	
	//检查上家
	for (i = 0; i < 3; i++) {
		XDATA char card = master_played_cards[last_claim_pid][i];
		real_cards[i] = card;
		if (card == ' ' || card == 0)
			continue;
		//检查是不是万能牌或者所叫牌
		if (card != round_card && card != JOKER_CARD)
			is_bluff = 1;
		else
			true_count++;
	}
	
	//质疑成功
	if (is_bluff == 1)
		loser_id = last_claim_pid;
	//质疑失败
	else
		loser_id = challenger_id;
	
	proc_sleep(50);
	
	//广播结果
	game_send_packet(ID_BROADCAST, CMD_SHOWDOWN,loser_id,real_cards[0],real_cards[1],real_cards[2], 0);
	
	// 如果主机还活着 处理自己的情况
	if (current_state != STATE_PLAYER_DEAD) {

		showdown_loser_id = loser_id;
		showdown_cards[0] = real_cards[0];
		showdown_cards[1] = real_cards[1];
		showdown_cards[2] = real_cards[2];
		
		//公布结果
		current_state = STATE_SHOW_CARDS_ANNOUNCE;
		proc_sleep(2000);
		
		//显示输家
		current_state = STATE_SHOW_LOSER_ANNOUNCE;
		proc_sleep(2000);
		
		if (loser_id == my_id) {
			current_state = STATE_PUNISHMENT;
			beep_os(200);
		}
		else
			current_state = STATE_IDLE_WAIT_TURN;
	}
}

// 只能由主机调用 负责惩罚玩家
void master_resolve_punishment(u8 loser_id) large reentrant {
  XDATA u8 is_hit = 0;	//是否命中
	XDATA u8 winner_id;		//如果游戏结束 保存胜利者id
	XDATA u8 i;
	XDATA u8 remaining_bullets; //剩余子弹
	
	//扣减子弹
	if (player_bullet_counts[loser_id] > 0)
    player_bullet_counts[loser_id]--;
	remaining_bullets = player_bullet_counts[loser_id];
	
	//判断是否命中
	if (remaining_bullets == master_fatal_shot[loser_id])
    is_hit = 1;
	
	//WAIT
	proc_sleep(50);
	
	//广播结果
	game_send_packet(ID_BROADCAST, CMD_PUNISH_RESULT,is_hit,loser_id,remaining_bullets,0, 0);
	
	//主机更新本地状态
	if (current_state != STATE_PLAYER_DEAD) {
		if (loser_id == my_id) {
			if (is_hit == 1) {
				//死了 长鸣 等待广播
				beep_os(1000);
				current_state = STATE_IDLE_WAIT_TURN;
			}
			else{
				//没死 显示存活2s
				beep_os(100);
				current_state = STATE_SURVIVE_ANNOUNCE;
			}
		}
		//等待惩罚
		else
			current_state = STATE_IDLE_WAIT_TURN;
	}
	
	//处理死亡玩家
	if (is_hit == 1) {
		proc_sleep(1000);
		
		CLEARBIT(player_alive, loser_id);
		// 广播谁死了
		game_send_packet(ID_BROADCAST, CMD_PLAYER_DEAD, loser_id, 0, 0, 0, 0);
		
		//如果主机存活 主机本地处理谁死了
		if (current_state != STATE_PLAYER_DEAD) {
			dead_player_id = loser_id;
			current_state = STATE_DEAD_ANNOUNCE;
			proc_sleep(2000);
			
			// 如果死的是自己
			if (loser_id == my_id)
				current_state = STATE_PLAYER_DEAD;
			else
				current_state = STATE_IDLE_WAIT_TURN;
		}
	}
		
	//检查游戏是否结束
	if (count_set_bits(player_alive) <= 1) {
		if (is_hit == 1)
			proc_sleep(1500);
			//proc_sleep(500);
		
		proc_sleep(1000);
		
		winner_id = 0xFF;
		if (count_set_bits(player_alive) == 1) {
			// 找到唯一胜利者
			for(i = 0; i < MAX_PLAYERS; i++) {
				if (GETBIT(player_alive, i)) {
						winner_id = i;
						break;
				}
			}
		}
		//宣布胜利者
		game_winner_id = winner_id;
		game_send_packet(ID_BROADCAST, CMD_GAME_WIN, winner_id, 0, 0, 0, 0);
    current_state = STATE_GAME_OVER;
	}
	//开启新的一轮
	else{
		//有人死了 短暂等待即可
		if (is_hit == 1){
			proc_sleep(500);
		}
		//没有人死 需要让开枪者有时间看到LIVE
		else{
			proc_sleep(2000);
		}
		master_start_new_round();
	}
	//如果游戏还没结束 在广播dead后进入下一轮
}







//OS数码管驱动内容
extern XDATA u8 seg_display_content[8];
extern CODE u8 seg_decoder[128];
//OS时钟
extern DATA u32 timer0_cnt;

//更新显示
void game_update_display(u8 param){
	u8 i;
	u8 cursor_real_pos; //光标位置

	//清空缓存
	for (i = 0; i < 8; i++)
		display_buffer[i] = ' ';
	
	switch (current_state){
		case STATE_LOBBY_IDLE: // Pi FREE
			display_buffer[0] = 'P'; display_buffer[1] = my_id + '0'; display_buffer[3] = 'F';
			display_buffer[4] = 'R'; display_buffer[5] = 'E'; display_buffer[6] = 'E';
			break;

		case STATE_LOBBY_WAIT_SLAVE: // Pi RDY
			display_buffer[0] = 'P';
			display_buffer[1] = my_id + '0';
			display_buffer[3] = 'R';
			display_buffer[4] = 'D';
			display_buffer[5] = 'Y';
			break;

		case STATE_LOBBY_WAIT_MASTER: //打印准备玩家01234567
			{
				u8 i;
				u8 display_idx = 0;
				for (i = 0; i < MAX_PLAYERS; i++) {
					if (GETBIT(player_alive, i)) {
						if (display_idx < 8) {
							display_buffer[display_idx] = i + '0';
							display_idx++;
						}
					}
				}
			}
			break;
		//大厅显示结束
		
		
		case STATE_SURVIVE_ANNOUNCE: //LIVE
			display_buffer[0] = 'L'; display_buffer[1] = 'I';
			display_buffer[2] = 'V'; display_buffer[3] = 'E';
			break;
		
		case STATE_DEAD_ANNOUNCE://Pi DEAD
			display_buffer[0] = 'P'; display_buffer[1] = dead_player_id + '0';
			display_buffer[3] = 'D'; display_buffer[4] = 'E';
			display_buffer[5] = 'A'; display_buffer[6] = 'D';
			break;
		
		case STATE_SHOW_CARDS_ANNOUNCE://Pi 3Card
			display_buffer[0] = 'P'; display_buffer[1] = last_claim_pid + '0';
			display_buffer[3] = showdown_cards[0];
			display_buffer[4] = showdown_cards[1];
			display_buffer[5] = showdown_cards[2];
			break;
		
		case STATE_SHOW_LOSER_ANNOUNCE://Pi LOSE
			display_buffer[0] = 'P';display_buffer[1] = showdown_loser_id + '0';
			display_buffer[3] = 'L';display_buffer[4] = 'O';display_buffer[5] = 'S';
			display_buffer[6] = 'E';
			break;
		
		case STATE_PUBLIC_CLAIM_ANNOUNCE://Pi CL t Crad
			display_buffer[0] = 'P';
			display_buffer[1] = last_claim_pid + '0';
			display_buffer[3] = 'C';display_buffer[4] = 'L';
			display_buffer[5] = last_claim_count + '0';
			display_buffer[7] = round_card;
			break;
		case STATE_ID_SELECT://P SEL  id
			display_buffer[0] = 'P';display_buffer[2] = 'S';
			display_buffer[3] = 'E'; display_buffer[4] = 'L';
			display_buffer[7] = param + '0';
			break;
		
		case STATE_GAME_OVER://Pi WIN
			display_buffer[0] = 'P';
			if (game_winner_id != 0xFF)
				 display_buffer[1] = game_winner_id + '0';
			display_buffer[3] = 'W'; display_buffer[4] = 'I'; display_buffer[5] = 'N';
			break;
				
		case STATE_PLAYER_DEAD://DEAD
			display_buffer[0] = 'D'; display_buffer[1] = 'E'; display_buffer[2] = 'A';
			display_buffer[3] = 'D';
			break;

		case STATE_MY_TURN_CHALLENGE://CHA Px y
			//上家x出了y张 是否质疑
			display_buffer[0] = 'C'; display_buffer[1] = 'H'; display_buffer[2] = 'A';
			display_buffer[4] = 'P'; display_buffer[5] = last_claim_pid + '0';
			display_buffer[7] = last_claim_count + '0';
			break;
				
		case STATE_WAIT_RESULT://WAIT
			display_buffer[0] = 'W'; display_buffer[1] = 'A'; display_buffer[2] = 'I';
			display_buffer[3] = 'T';
			break;

		case STATE_PUNISHMENT:
			//FIRE NOW
			display_buffer[0] = 'F'; display_buffer[1] = 'I'; display_buffer[2] = 'R';
			display_buffer[3] = 'E';
			display_buffer[5] = 'N'; display_buffer[6] = 'O'; display_buffer[7] = 'W';
			break;
		
		case STATE_NEW_ROUND_ANNOUNCE://新的回合 N RND i
		{
			u8 tens = current_round_display / 10;
			u8 units = current_round_display % 10;
			display_buffer[0] = 'N'; display_buffer[2] = 'R';
			display_buffer[4] = 'N'; display_buffer[5] = 'D'; display_buffer[7] = units + '0';
			if(tens != 0)
				display_buffer[6] = tens + '0';
			break;
		}
				
		//默认状态 显示手牌
		case STATE_MY_TURN_SELECT:
		default:
			//0为隐藏 1为显示
			if (display_mode == 0){
				//隐藏状态时 显示00000 剩余开火次数
				for (i = 0; i < HAND_SIZE; i++)
					display_buffer[i] = '0';
				display_buffer[7] = player_bullet_counts[my_id] + '0';
			}
			else{
				//显示状态时 显示手牌和本轮叫牌
				for (i = 0; i < my_hand_count; i++){
					// (HAND_SIZE - my_hand_count)右对齐
					display_buffer[ (HAND_SIZE - my_hand_count) + i ] = my_hand[i];
				}
				display_buffer[7] = round_card;
			}
			break;
	}
	
	//原子操作
	ATOMIC_START();
	
	//字符转数码管段码
	for (i = 0; i < 8; i++)
		seg_display_content[i] = seg_decoder[ display_buffer[i] ];
	
	//只在选牌模式和显示模式下 控制光标
	if (current_state == STATE_MY_TURN_SELECT && display_mode == 1){
		//计算光标位置
		cursor_real_pos = (HAND_SIZE - my_hand_count) + cursor_pos;
		if (cursor_real_pos < 8) {
			//点亮小数点
			seg_display_content[cursor_real_pos] |= 0x80;
		}
		//闪烁已选中的牌
		if ((timer0_cnt / 200) % 2 == 0) { 
			for(i=0; i<my_hand_count; i++) {
				if (GETBIT(selected_cards, i)) {
					u8 real_pos = (HAND_SIZE - my_hand_count) + i;
					if(real_pos < 8)
						seg_display_content[real_pos] = 0x00;
				}
			}
		}
	}
	
	ATOMIC_END();
}

//专门用来处理显示
void display_proc() large reentrant{
    while(1){
			game_update_display(display_param_global);
			proc_sleep(20);
    }
}


//游戏主进程
void communication_proc() large reentrant {
	XDATA u8 temp_id_select = 0; //临时ID
	
	// 蜂鸣器推挽
	P3M1 &= 0xEF; // P3.4 M1=0
	P3M0 |= 0x10; // P3.4 M0=1
	BZ = 0;
	
	// 初始化状态
	current_state = STATE_ID_SELECT;
	my_id = 0xFF;
	display_param_global = 0; // 初始化ID选择的显示
	
	// 游戏主循环
	while(1) {
		// 等待事件
		proc_wait_evts(
			EVT_UART2_RECV | //485
			EVT_BTN1_DN | EVT_BTN2_DN | //按键 
			EVT_NAV_L | EVT_NAV_R | EVT_NAV_U | EVT_NAV_D | EVT_NAV_PUSH //遥感
		);
		if (current_state == STATE_ID_SELECT){
			display_param_global = temp_id_select;
			// 上
			if (MY_EVENTS & EVT_NAV_U)
				temp_id_select = (temp_id_select + 1) % MAX_PLAYERS;
			// 下
			if (MY_EVENTS & EVT_NAV_D)
				temp_id_select = (temp_id_select + MAX_PLAYERS - 1) % MAX_PLAYERS;
			
			display_param_global = temp_id_select;
			
			// K1或遥感按下
			if (MY_EVENTS & (EVT_BTN1_DN | EVT_NAV_PUSH)) {
				my_id = temp_id_select;
				
				// 选好了ID
				srand();//随机性 修改了随机由时钟决定
				beep_os(100);
				display_param_global = 0;
				
				// 进入大厅
				current_state = STATE_LOBBY_IDLE;
				player_alive = 0; // 初始化准备列表
			}
		}
		// 其他状态
		else {
			// 485
			if (MY_EVENTS & EVT_UART2_RECV)
				game_handle_packet();
			
			// 按键
			if (MY_EVENTS & ~EVT_UART2_RECV)
				game_handle_buttons();
		}
	}
}

/*
void communication_proc() large reentrant {
	XDATA u8 temp_id_select = 0; //临时的ID
	
	// 蜂鸣器引脚推挽
	P3M1 &= 0xEF; // P3.4 M1=0
	P3M0 |= 0x10; // P3.4 M0=1
	BZ = 0;
	
	// ID 选择
	current_state = STATE_ID_SELECT;

	while(my_id == 0xFF) {
		game_update_display(temp_id_select); 
		
		// 等待按键被按下
		proc_wait_evts(EVT_NAV_U | EVT_NAV_D | EVT_BTN1_DN | EVT_NAV_PUSH);
		// 上
		if (MY_EVENTS & EVT_NAV_U)
			temp_id_select = (temp_id_select + 1) % MAX_PLAYERS;
		// 下
		if (MY_EVENTS & EVT_NAV_D)
			temp_id_select = (temp_id_select + MAX_PLAYERS - 1) % MAX_PLAYERS;
		// K1或遥感按下
		if (MY_EVENTS & (EVT_BTN1_DN | EVT_NAV_PUSH))
			my_id = temp_id_select;
	}
	
	// 选好了ID
	beep_os(100);
	
	// 根据是否为主机设置初始状态
	if (my_id == ID_MASTER)
		//主机按下K1开始游戏
		current_state = STATE_IDLE_WAIT_TURN;
	else
		//从机等待主机
		current_state = STATE_IDLE_WAIT_TURN;
	
	// 游戏主循环
	while(1) {
		// 刷新数码管
		//game_update_display(0);

		// 等待事件
		proc_wait_evts(
			EVT_UART2_RECV | // RS485
			EVT_BTN1_DN | EVT_BTN2_DN |  //按键
			EVT_NAV_L | EVT_NAV_R | EVT_NAV_U | EVT_NAV_D | EVT_NAV_PUSH //导航
		);

		// 485
		if (MY_EVENTS & EVT_UART2_RECV)
			game_handle_packet();
		// 按键
		if (MY_EVENTS & ~EVT_UART2_RECV)
			game_handle_buttons();
	}
}
*/

/*

//只能由主机调用 负责开始新回合
//负责洗牌 给定叫牌 给自己发牌 给别人发牌 轮到自己出牌
void master_start_new_round(){
	u8 i;
	//洗牌
	master_shuffle_deck(); 
	
	//叫牌
	round_card = CARD_POOL[rand32() % CARD_POOL_SIZE];
	while(round_card == JOKER_CARD) { //不能叫JOKER
		round_card = CARD_POOL[rand32() % CARD_POOL_SIZE];
	}
	
	//主机的手牌
	my_hand_count = HAND_SIZE;
	for(i=0; i<HAND_SIZE; i++)
		my_hand[i] = master_all_hands[0][i];

	//主机给从机发牌
	for (i = 1; i < MAX_PLAYERS; i++){
		if (GETBIT(player_alive, i)){ //只给存活玩家发牌
			//发包
			game_send_packet(i, CMD_DEAL_HAND, master_all_hands[i][0], master_all_hands[i][1], master_all_hands[i][2], 
				master_all_hands[i][3], master_all_hands[i][4]);
			
			//为防止忙碌 发包后短暂休眠
			proc_sleep(100);
		}
	}
	
	//主机宣布新回合
	game_send_packet(ID_BROADCAST, CMD_NEW_ROUND, round_card, 0, 0, 0, 0);
	
	//主机回合
	last_claim = 0;//清零叫牌
	current_turn_id = 0;
	current_state = STATE_MY_TURN_SELECT; // 主机选牌
}

//只能由主机调用 检查质疑结果


/*
XDATA u8 rs485_message[] = "BEEP";

void beep_os(u16 duration_ms) large reentrant
{
	u16 i;
	for (i = 0; i < (duration_ms / 2); i++)
	{
		BZ = 1;
		proc_sleep(1);
		BZ = 0;
		proc_sleep(1);
	}
	BZ = 0;
}

void communication_proc() large reentrant
{
	P3M1 &= 0xEF;
	P3M0 |= 0x10;
	BZ = 0;

	while(1)
	{
		proc_wait_evts(EVT_BTN2_DN | EVT_UART2_RECV);
		if (MY_EVENTS & EVT_BTN2_DN)
		{
			rs485_write(rs485_message, 4); 
		}
		if (MY_EVENTS & EVT_UART2_RECV)
		{
			beep_os(500); 
		}
	}
}
*/

int main()
{
	//initialize kernel stack and xstack pointer
	XDATA u8 i;
	
	SP = kernel_stack;
	setxbp(kernel_xstack + KERNEL_XSTACKSIZE);
	
	//set process stacks and swap stacks owner
	process_stack[0][PROCESS_STACKSIZE-1] = 0;
	process_stack[1][PROCESS_STACKSIZE-1] = 1;
	process_stack[2][PROCESS_STACKSIZE-1] = 2;
	process_stack[3][PROCESS_STACKSIZE-1] = 3;
	process_stack[4][PROCESS_STACKSIZE-1] = 4;
	process_stack_swap[0][PROCESS_STACKSIZE-1] = 5;
	process_stack_swap[1][PROCESS_STACKSIZE-1] = 6;
	process_stack_swap[2][PROCESS_STACKSIZE-1] = 7;
	
	//initialize LED pins
	P0M1 &= 0x00;
	P0M0 |= 0xff;
	P2M1 &= 0xf0;
	P2M0 |= 0x0f;
	//select LED, set all off
	P23 = 1;
	P0 = 0;

	//initialize buttons
	buttons_init();
	
	//initialize serial ports
	usbcom_init(115200);
	rs485_init(115200);
		
	//start process

		
		
	//initialize PCA2 interrupt (as syscall interrupt)
	//clear CCF2
	CCON &= ~0x04;
	//disable PCA2 module and set ECCF2
	CCAPM2 = 1;
	//low priority interrupt
	PPCA = 0;
	
	//初始化子弹数
	for(i=0; i<MAX_PLAYERS; i++) {
		player_bullet_counts[i] = BULLET_SLOTS;
	}
	//启动进程
	start_process((u16)display_proc, 1, 0);
	start_process((u16)communication_proc, 0, 0);
	
	//start_process((u16)display_test_process, 0, 0);
	
	
	//start main timer
	TR0 = 0;														//stop timer
	TMOD &= 0xF0;												//timer mode, 16b autoreload
	AUXR &= 0x7F;												//12T mode
	TL0 = T12RL & 0xff;							//set reload value
	TH0 = (T12RL & 0xff00) >> 8;
	ET0 = EA = 1;												//set interrupt enable
	PT0 = 0;														//set priority to low
	TR0 = 1;														//start timer
	
	
	//spin
	while(1);
}
