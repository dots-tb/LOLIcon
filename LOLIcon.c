// LOLIcon
// by @dots_tb
// Thanks to:
// @CelesteBlue123 partner in crime, shh he doesn't know I added his namespace
// Our testing team, especially: @Yoyogames28 castelo
// @Cimmerian_Iter is also worth mentioning...

// @frangar for original oclockvita
// HENkaku wiki and its contributors (especially yifan lu for Pervasive RE)
// Team Molecule (because everyone does)
// Scorp for Button Swap
// Please read the readme for more credits

//MADE IN THE USA

#include <taihen.h>
#include <string.h>
#include <sys/syslimits.h>
#include <stdio.h>
#include "blit.h"
#include "utils.h"

#define LEFT_LABEL_X CENTER(24)
#define RIGHT_LABEL_X CENTER(0)
#define printf ksceDebugPrintf

static SceUID g_hooks[14];

static const char *ERRORS[5]={ 
	#define NO_ERROR 0
	"No error.", 
	#define SAVE_ERROR 1
	"There was a problem saving.", 
	#define SAVE_GOOD 2
	"Configuration saved.", 
	#define LOAD_ERROR 3
	"There was a problem loading.", 
	#define LOAD_GOOD 4
	"Configuration loaded."
};

int error_code = NO_ERROR;

#define CONFIG_PATH "ur0:LOLIcon/"

typedef struct titleid_config {
	int mode;
	int hideErrors;
	int showBat;
	int buttonSwap;
	int showFPS;
} titleid_config;

static char config_path[PATH_MAX];
static titleid_config current_config;

static char titleid[32];
uint32_t current_pid = 0, shell_pid = 0;

int showMenu = 0, pos = 0, isReseting = 0, forceReset = 0, isPspEmu = 0, isShell = 1;
int page = 0;
int willexit = 0;
static uint64_t ctrl_timestamp, msg_time = 0;

uint32_t *clock_speed;
unsigned int *clock_r1;
unsigned int *clock_r2;	

#define TIMER_SECOND         1000000 // 1 second
int fps;
long curTime = 0, lateTime = 0, fps_count = 0;

static int profile_default[] = {266, 166, 166, 111, 166};
static int profile_game[] = {444, 222, 222, 166, 222};
static int profile_max_performance[] = {444, 222, 222, 166, 333};
static int profile_holy_shit_performance[] = {500, 222, 222, 166, 333};
static int profile_max_battery[] = {111, 111, 111, 111, 111};
static int* profiles[5] = {profile_default,profile_game,profile_max_performance, profile_holy_shit_performance, profile_max_battery};


int (*_kscePowerGetGpuEs4ClockFrequency)(int*, int*);
int (*_kscePowerSetGpuEs4ClockFrequency)(int, int);
int (*_kscePowerGetGpuClockFrequency)(void);
int (*_kscePowerSetGpuClockFrequency)(int);
int (*_ksceKernelGetModuleInfo)(SceUID, SceUID, SceKernelModuleInfo *);
int (*_ksceKernelGetModuleList)(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);
int (*_ksceKernelExitProcess)(int);

#define ksceKernelExitProcess _ksceKernelExitProcess
#define ksceKernelGetModuleInfo _ksceKernelGetModuleInfo
#define ksceKernelGetModuleList _ksceKernelGetModuleList
#define kscePowerGetGpuEs4ClockFrequency _kscePowerGetGpuEs4ClockFrequency
#define kscePowerSetGpuEs4ClockFrequency _kscePowerSetGpuEs4ClockFrequency
#define kscePowerGetGpuClockFrequency _kscePowerGetGpuClockFrequency
#define kscePowerSetGpuClockFrequency _kscePowerSetGpuClockFrequency

void reset_config() {
	memset(&current_config, 0, sizeof(current_config));
}

int load_config() {
	snprintf(config_path, sizeof(config_path), CONFIG_PATH"%s/config.bin", titleid);
	printf("loaded %s\n", config_path);
	if(ReadFile(config_path, &current_config, sizeof(current_config))<0) {
		snprintf(config_path, sizeof(config_path), CONFIG_PATH"default.bin");
		if(ReadFile(config_path, &current_config, sizeof(current_config))<0) {
			reset_config();
			return -1;
		}
	}
	return 0;
}

int save_config() {
	snprintf(config_path, sizeof(config_path), CONFIG_PATH"%s", titleid);
	ksceIoMkdir(config_path, 6);
	snprintf(config_path, sizeof(config_path), CONFIG_PATH"%s/config.bin", titleid);
	if(WriteFile(config_path, &current_config, sizeof(current_config))<0)
		return -1;
	return 0;	
}

int save_default_config() {
	snprintf(config_path, sizeof(config_path), CONFIG_PATH"default.bin");
	if(WriteFile(config_path, &current_config, sizeof(current_config))<0)
		return -1;
	return 0;	
}

void refreshClocks() {
	isReseting = 1;
	kscePowerSetArmClockFrequency(profiles[current_config.mode][0]);
	kscePowerSetBusClockFrequency(profiles[current_config.mode][1]);
	kscePowerSetGpuEs4ClockFrequency(profiles[current_config.mode][2], profiles[current_config.mode][2]);
	kscePowerSetGpuXbarClockFrequency(profiles[current_config.mode][3]);
	kscePowerSetGpuClockFrequency(profiles[current_config.mode][4]);
	isReseting = 0;
}

void load_and_refresh() {
	error_code = LOAD_GOOD;
	if(load_config()<0) 
		error_code = LOAD_ERROR;			
	refreshClocks();
	printf("forcing reset\n");
}


// This function is from VitaJelly by DrakonPL and Rinne's framecounter
void doFps() {
	fps_count++;
	if ((curTime - lateTime) > TIMER_SECOND) {
		lateTime = curTime;
		fps = (int)fps_count;
		fps_count = 0;
	}
	blit_stringf(20, 15, "%d",  fps);
}

void drawErrors() {
	if(error_code > 0) {
		if(!curTime || (msg_time == 0 && !showMenu))
			msg_time = (curTime = ksceKernelGetProcessTimeWideCore()) + TIMER_SECOND * 2;
		if((!current_config.hideErrors && curTime < msg_time) || showMenu)
			blit_stringf(20, 0, "%s : %d",  ERRORS[error_code], error_code);
	}
}

int kscePowerSetClockFrequency_patched(tai_hook_ref_t ref_hook, int port, int freq){
	int ret = 0;
	if(!isReseting)
		profile_default[port] = freq;
	if(port==0) {
		if(freq == 500) {
			ret = TAI_CONTINUE(int, ref_hook, 444);
			ksceKernelDelayThread(10000);
			*clock_speed = profiles[current_config.mode][port];
			*clock_r1 = 0xF;
			*clock_r2 = 0x0;
			return ret;
		}
	} 
	if(port==2) {
		ret = TAI_CONTINUE(int, ref_hook, profiles[current_config.mode][port], profiles[current_config.mode][port]);
	} else
		ret = TAI_CONTINUE(int, ref_hook, profiles[current_config.mode][port]);
	return ret;
}

static tai_hook_ref_t power_hook1;
static int power_patched1(int freq) {
	return kscePowerSetClockFrequency_patched(power_hook1,0,freq);
}

static tai_hook_ref_t power_hook2;
static int power_patched2(int freq) {
	return kscePowerSetClockFrequency_patched(power_hook2,1,freq);
}

static tai_hook_ref_t power_hook3;
static int power_patched3(int freq) {
	return kscePowerSetClockFrequency_patched(power_hook3,2,freq);
}

static tai_hook_ref_t power_hook4;
static int power_patched4(int freq) {
	return kscePowerSetClockFrequency_patched(power_hook4,3,freq);
}

int checkButtons(int port, tai_hook_ref_t ref_hook, SceCtrlData *ctrl, int count) {
	int ret = 0, state;
	if (ref_hook == 0)
		ret = 1;
	else {
		ret = TAI_CONTINUE(int, ref_hook, port, ctrl, count);
		if(!showMenu){
			if (!isPspEmu && (ctrl->buttons & SCE_CTRL_UP)&&(ctrl->buttons & SCE_CTRL_SELECT))
				ctrl_timestamp = showMenu = 1;
			if (current_config.buttonSwap && 
				((isShell && shell_pid == ksceKernelGetProcessId())||(!isShell && current_pid == ksceKernelGetProcessId())) && 
				(ctrl->buttons & 0x6000) && ((ctrl->buttons & 0x6000) != 0x6000))
					ctrl->buttons = ctrl->buttons ^ 0x6000;
		} else {
			unsigned int buttons = ctrl->buttons;
			ctrl->buttons = 0;
			if(ctrl->timeStamp > ctrl_timestamp + 300*1000) {
				if( ksceKernelGetProcessId() == shell_pid) {
					if (buttons & SCE_CTRL_LEFT){
						switch(page) {
							case 1:
								if(current_config.mode > 0) {
									ctrl_timestamp = ctrl->timeStamp;
									current_config.mode--;
									refreshClocks();
								}
								break;
						}
					} else if ((buttons & SCE_CTRL_RIGHT)){
						switch(page) {
							case 1:
								if(current_config.mode <4) {
									ctrl_timestamp = ctrl->timeStamp;
									current_config.mode++;
									refreshClocks();
								}
								break;
						}
					} else if((buttons & SCE_CTRL_UP) && pos > 0) {
						ctrl_timestamp = ctrl->timeStamp;
						pos--;
					} else if (buttons & SCE_CTRL_CIRCLE)
						page = pos = 0;
					 else if (buttons & SCE_CTRL_CROSS) {
						 switch(page) {
							case 0:
								switch(pos) {
									case 0:
										error_code = SAVE_GOOD;
										if(save_config() < 0)
											error_code = SAVE_ERROR;
										break;
									case 1: {
										error_code = SAVE_GOOD;
										if(save_default_config() < 0)
											error_code = SAVE_ERROR;
										}
										break;
									case 2:
										reset_config();
										refreshClocks();
										break;
									case 3:
										page = 1;
										pos = 0;
										break;
									case 4:
										page = 2;
										pos = 0;
										break;
									case 5:
										page = 3;
										pos = 0;
										break;
									case 6:
										willexit = current_pid;
										break;
									case 7:
										kscePowerRequestSuspend();
										break;
									case 8:
										kscePowerRequestColdReset();
										break;
									case 9:
										kscePowerRequestStandby();
										break;
								}
								break;
							case 2:
								switch(pos) {
									case 0:
										current_config.showFPS = !current_config.showFPS;
										break;
									case 1:
										current_config.showBat = !current_config.showBat;
										break;		
									case 2:
										current_config.hideErrors = !current_config.hideErrors;
										break;
	
								}
								break;
							case 3:
								switch(pos) {
									case 0:
										current_config.buttonSwap = !current_config.buttonSwap;
										break;
								}
								break;								
						 }
						 ctrl_timestamp = ctrl->timeStamp;
					 }  else if (buttons & SCE_CTRL_DOWN) {
						pos++;
						ctrl_timestamp = ctrl->timeStamp;
					}
				}
				if((buttons & SCE_CTRL_SELECT)&&(buttons & SCE_CTRL_DOWN))
					error_code = showMenu = 0;
			}
		}
		if(KERNEL_PID!=ksceKernelGetProcessId()&& shell_pid!=ksceKernelGetProcessId()) {
			if(forceReset == 1) {
				if(current_pid==ksceKernelGetProcessId()) {
					if(ksceKernelGetProcessTitleId(current_pid, titleid, sizeof(titleid))==0 && titleid[0] != 0) 
						forceReset = 2;
				} else 
					current_pid=ksceKernelGetProcessId();
			}
			if(willexit == current_pid && current_pid == ksceKernelGetProcessId()) 
				ksceKernelExitProcess(0);
			else
				willexit = 0;
		} else if(forceReset == 2) {
			isShell = 0;
			load_and_refresh();
			msg_time = curTime = fps_count = lateTime = forceReset = 0;
		}
	}
	return ret;
}

static tai_hook_ref_t ref_hook1;
static int keys_patched1(int port, SceCtrlData *ctrl, int count) {
	int ret, state;
	if(isPspEmu) {
		ENTER_SYSCALL(state);
		ret = TAI_CONTINUE(int, ref_hook1, port, ctrl, count);
		EXIT_SYSCALL(state);
	} else 
		ret = checkButtons(port, ref_hook1, ctrl, count);
	return ret;
}   

static tai_hook_ref_t ref_hook2;
static int keys_patched2(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook2, ctrl, count);
}   

static tai_hook_ref_t ref_hook3;
static int keys_patched3(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook3, ctrl, count);
}  

static tai_hook_ref_t ref_hook4;
static int keys_patched4(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook4, ctrl, count);
}    

static tai_hook_ref_t ref_hook5;
static int keys_patched5(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook5, ctrl, count);
}    

static tai_hook_ref_t ref_hook6;
static int keys_patched6(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook6, ctrl, count);
}    

static tai_hook_ref_t ref_hook7;
static int keys_patched7(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook7, ctrl, count);
}    

static tai_hook_ref_t ref_hook8;
static int keys_patched8(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hook8, ctrl, count);
}    

void drawMenu() {
	int entries = 0;
	#define MENU_OPTION_F(TEXT,...)\
		blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
		blit_stringf(LEFT_LABEL_X, 120+16*entries++, (TEXT), __VA_ARGS__);	
	#define MENU_OPTION(TEXT,...)\
		blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
		blit_stringf(LEFT_LABEL_X, 120+16*entries++, (TEXT));	
	blit_set_color(0x00FFFFFF, 0x00FF0000);
	switch(page) {
		case 0:
			blit_stringf(LEFT_LABEL_X, 88, "LOLIcon by @dots_tb");
			MENU_OPTION_F("Save for %s", titleid);
			MENU_OPTION("Save as Default");
			MENU_OPTION("Clear settings");
			MENU_OPTION("Oclock Options");
			MENU_OPTION("OSD Options");
			MENU_OPTION("Ctrl Options");
			MENU_OPTION("Exit Game");
			MENU_OPTION("Suspend vita");
			MENU_OPTION("Restart vita");
			MENU_OPTION("Shutdown vita");
			break;
		case 1:
			blit_stringf(LEFT_LABEL_X, 88, "ACTUAL OVERCLOCK");		
			blit_stringf(LEFT_LABEL_X, 120, "PROFILE    ");
			switch(current_config.mode) {
				case 4: 
					blit_stringf(RIGHT_LABEL_X, 120, "Max Batt.");
					break;
				case 3: 
					blit_stringf(RIGHT_LABEL_X, 120, "Holy Shit.");
					break;
				case 2: 
					blit_stringf(RIGHT_LABEL_X, 120, "Max Perf.");
					break;
				case 1:
					blit_stringf(RIGHT_LABEL_X, 120, "Game Def.");
					break;
				case 0:
					blit_stringf(RIGHT_LABEL_X, 120, "Default  ");
					break;
				}	
			blit_stringf(LEFT_LABEL_X, 136, "CPU CLOCK  ");
			blit_stringf(RIGHT_LABEL_X, 136, "%-4d  MHz - %d:%d", kscePowerGetArmClockFrequency(), *clock_r1, *clock_r2);
			blit_stringf(LEFT_LABEL_X, 152, "BUS CLOCK  ");
			blit_stringf(RIGHT_LABEL_X, 152, "%-4d  MHz", kscePowerGetBusClockFrequency());
			blit_stringf(LEFT_LABEL_X, 168, "GPUes4CLK  ");
			
			int r1, r2;
			kscePowerGetGpuEs4ClockFrequency(&r1, &r2);
			blit_stringf(RIGHT_LABEL_X, 168, "%-d   MHz", r1);
			blit_stringf(LEFT_LABEL_X, 184, "XBAR  CLK  ");
			blit_stringf(RIGHT_LABEL_X, 184, "%-4d  MHz", kscePowerGetGpuXbarClockFrequency());
			blit_stringf(LEFT_LABEL_X, 200, "GPU CLOCK  ");
			blit_stringf(RIGHT_LABEL_X, 200, "%-4d  MHz", kscePowerGetGpuClockFrequency());
			break;
		case 2:
			blit_stringf(LEFT_LABEL_X, 88, "OSD");	
			MENU_OPTION_F("Show FPS %d",current_config.showFPS);
			MENU_OPTION_F("Show Battery %d",current_config.showBat);
			MENU_OPTION_F("Hide Errors %d",current_config.hideErrors);
			break;
		case 3:
			blit_stringf(LEFT_LABEL_X, 88, "CONTROL");	
			MENU_OPTION_F("BUTTON SWAP %d",current_config.buttonSwap);
			break;			
	}
	if(pos >= entries)
		pos = entries -1;	
}

static tai_hook_ref_t ref_hook0;
int _sceDisplaySetFrameBufInternalForDriver(int fb_id1, int fb_id2, const SceDisplayFrameBuf *pParam, int sync){
	if(!isPspEmu && fb_id1 && pParam) {
		if(!shell_pid && fb_id2) {//3.68 fix
			if(ksceKernelGetProcessTitleId(ksceKernelGetProcessId(), titleid, sizeof(titleid))==0 && titleid[0] != 0) {
				if(strncmp("main",titleid, sizeof(titleid))==0) {
					shell_pid = ksceKernelGetProcessId();
					load_and_refresh();
				}
			}
		}
		SceDisplayFrameBuf kfb;
		memset(&kfb,0,sizeof(kfb));
		memcpy(&kfb, pParam, sizeof(SceDisplayFrameBuf));
		blit_set_frame_buf(&kfb);
		if(showMenu) drawMenu();
		
		blit_set_color(0x0000FF00, 0xff000000);
		if((isShell && shell_pid == ksceKernelGetProcessId())||(!isShell && current_pid == ksceKernelGetProcessId())) {
			drawErrors();
			curTime = ksceKernelGetProcessTimeWideCore();
			if(current_config.showFPS) doFps();
			if(current_config.showBat) blit_stringf(20, 30, "%02d\%", kscePowerGetBatteryLifePercent());
		}
		
	}
	return TAI_CONTINUE(int, ref_hook0, fb_id1, fb_id2, pParam, sync);
}

int getFindModNameFromPID(int pid, char *mod_name, int size) {
	SceKernelModuleInfo sceinfo;
	sceinfo.size = sizeof(sceinfo);
	int ret;
	size_t count;
	SceUID modids[128];		
	if((ret = ksceKernelGetModuleList(pid, 0xff, 1, modids, &count)) == 0) {
		for(int i = 0; i < count; i++) {
			if((ret = ksceKernelGetModuleInfo(pid, modids[count - 1], &sceinfo))==0) {
				if(strncmp(mod_name, sceinfo.module_name, size)==0)
					return 1;
			}
		}
		return 0;
	}
	return ret;
}

static tai_hook_ref_t process_hook0;
int SceProcEventForDriver_414CC813(int pid, int id, int r3, int r4, int r5, int r6){
	SceKernelProcessInfo info;
	info.size = 0xE8;
	char module_name[28];
	if(strncmp("main",titleid, sizeof(titleid))==0) {
		switch(id) {
			case 0x1://startup
				if(!shell_pid && ksceKernelGetProcessInfo(pid, &info) ==0 ) {
					if(info.ppid == KERNEL_PID) {
						shell_pid = pid;
						strncpy(titleid, "main", sizeof("main"));
						load_and_refresh();
						break;
					}
				}
			case 0x5:
				isPspEmu = getFindModNameFromPID(pid, "adrenaline", sizeof("adrenaline"))||getFindModNameFromPID(pid, "ScePspemu", sizeof("ScePspemu"));
				current_pid = pid;
				if(!isPspEmu) 
					forceReset = 1;
				 else {
					ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
					showMenu = isShell = current_config.mode = 0;
				}
				break;
		}
	} else {
		if((id==0x4 || id == 0x3)&& (current_pid==pid||isPspEmu)) {
			msg_time = curTime = fps_count = lateTime = 0;
			isShell = 1;
			strncpy(titleid, "main", sizeof("main"));
			isPspEmu =0;
			load_and_refresh();
		}
	}
	return TAI_CONTINUE(int, process_hook0, pid, id, r3, r4, r5, r6);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	ksceIoMkdir(CONFIG_PATH,6);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x475BCC82, &_kscePowerGetGpuEs4ClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x264C24FC, &_kscePowerSetGpuEs4ClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x64641E6A, &_kscePowerGetGpuClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x621BD8FD , &_kscePowerSetGpuClockFrequency);

	tai_module_info_t tai_info;
	
	tai_info.size = sizeof(tai_module_info_t);

	clock_r1 = (unsigned int *)pa2va(0xE3103000);
	clock_r2 = (unsigned int *)pa2va(0xE3103004);	
	
	taiGetModuleInfoForKernel(KERNEL_PID, "ScePower", &tai_info);
	module_get_offset(KERNEL_PID, tai_info.modid, 1,  0x4124 + 0xA4, (uintptr_t)&clock_speed);	
	
	memset(&titleid, 0, sizeof(titleid));
	strncpy(titleid, "main", sizeof(titleid));
	reset_config();
	
	current_config.mode = 3;
	
	refreshClocks();
	
	if(module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0xD269F915 , &_ksceKernelGetModuleInfo))
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xDAA90093 , &_ksceKernelGetModuleInfo);
	if(module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x97CF7B4E , &_ksceKernelGetModuleList))
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xB72C75A4 , &_ksceKernelGetModuleList);
	if(module_get_export_func(KERNEL_PID, "SceProcessmgr", 0x7A69DE86, 0x4CA7DC42 , &_ksceKernelExitProcess))
		module_get_export_func(KERNEL_PID, "SceProcessmgr", 0xEB1F8EF7, 0x905621F9 , &_ksceKernelExitProcess);

	
	g_hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hook0, "SceDisplay",0x9FED47AC,0x16466675, _sceDisplaySetFrameBufInternalForDriver); 
	
	
	g_hooks[10] = taiHookFunctionExportForKernel(KERNEL_PID, &power_hook1, "ScePower", 0x1590166F, 0x74DB5AE5,power_patched1); // scePowerSetArmClockFrequency
	g_hooks[11] = taiHookFunctionExportForKernel(KERNEL_PID,	&power_hook2, "ScePower", 0x1590166F, 0xB8D7B3FB, power_patched2); // scePowerSetBusClockFrequency
	g_hooks[12] = taiHookFunctionExportForKernel(KERNEL_PID, &power_hook3, "ScePower", 0x1590166F, 0x264C24FC, power_patched3); // scePowerSetGpuClockFrequency
	g_hooks[13] = taiHookFunctionExportForKernel(KERNEL_PID, &power_hook4, "ScePower", 0x1590166F, 0xA7739DBE, power_patched4); // scePowerSetGpuXbarClockFrequency
	
	taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &tai_info);
	g_hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hook1, "SceCtrl", TAI_ANY_LIBRARY, 0xEA1D3A34, keys_patched1); // sceCtrlPeekBufferPositive
	g_hooks[2] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook2, tai_info.modid, 0, 0x3EF8, 1, keys_patched2); // sceCtrlPeekBufferPositive2
	g_hooks[3] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hook3, "SceCtrl", TAI_ANY_LIBRARY, 0x9B96A1AA, keys_patched3); // sceCtrlReadBufferPositive
	g_hooks[4] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook4, tai_info.modid, 0, 0x4E14, 1, keys_patched4); // sceCtrlReadBufferPositiveExt2
	g_hooks[5] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook5, tai_info.modid, 0, 0x4B48, 1, keys_patched5); // sceCtrlPeekBufferPositiveExt2
	g_hooks[6] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook6, tai_info.modid, 0, 0x3928, 1, keys_patched6); // sceCtrlPeekBufferPositiveExt
    g_hooks[7] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook7, tai_info.modid, 0, 0x449C, 1, keys_patched7); // sceCtrlReadBufferPositive2
    g_hooks[8] =  taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hook8, tai_info.modid, 0, 0x3BCC, 1, keys_patched8); // sceCtrlReadBufferPositiveExt
		  
	g_hooks[9] = taiHookFunctionImportForKernel(KERNEL_PID, &process_hook0, "SceProcessmgr", TAI_ANY_LIBRARY, 0x414CC813, SceProcEventForDriver_414CC813); 
		
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	// free hooks that didn't fail
	if (g_hooks[0] >= 0) taiHookReleaseForKernel(g_hooks[0], ref_hook0);
	if (g_hooks[1] >= 0) taiHookReleaseForKernel(g_hooks[1], ref_hook1);
	if (g_hooks[2] >= 0) taiHookReleaseForKernel(g_hooks[2], ref_hook2);
	if (g_hooks[3] >= 0) taiHookReleaseForKernel(g_hooks[3], ref_hook3);
	if (g_hooks[4] >= 0) taiHookReleaseForKernel(g_hooks[4], ref_hook4);
	if (g_hooks[5] >= 0) taiHookReleaseForKernel(g_hooks[5], ref_hook5);
	if (g_hooks[6] >= 0) taiHookReleaseForKernel(g_hooks[6], ref_hook6);
	if (g_hooks[7] >= 0) taiHookReleaseForKernel(g_hooks[7], ref_hook7);
	if (g_hooks[8] >= 0) taiHookReleaseForKernel(g_hooks[8], ref_hook8);
	if (g_hooks[9] >= 0) taiHookReleaseForKernel(g_hooks[9], process_hook0);
	if (g_hooks[10] >= 0) taiHookReleaseForKernel(g_hooks[10], power_hook1);
	if (g_hooks[11] >= 0) taiHookReleaseForKernel(g_hooks[11], power_hook2);
	if (g_hooks[12] >= 0) taiHookReleaseForKernel(g_hooks[12], power_hook3);
	if (g_hooks[13] >= 0) taiHookReleaseForKernel(g_hooks[13], power_hook4);

	return SCE_KERNEL_STOP_SUCCESS;
}
