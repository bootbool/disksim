#ifndef DISKSIM_POWER_H
#define DISKSIM_POWER_H

#include "disksim_global.h"

/* Power event ranges */
#define POWER_MIN_EVENT             170
#define POWER_SPINUP_EVENT          170
#define POWER_SPINDOWN_EVENT        171 
#define POWER_MAX_EVENT				171

#define POWER_STATE_IDLE			0
#define POWER_STATE_ACTIVE			1
#define POWER_STATE_STANDBY			2
#define POWER_STATE_SPINNING_UP		3
#define POWER_STATE_SPINNING_DOWN	4


typedef struct power_ev {
  double time;
  int type;
  struct power_ev *next;
  struct power_ev *prev;
  int devno;
} power_event;

typedef struct power_param{
	double active_power;      /*the power when active, watt*/
	double idle_power;        /*the power when idle, watt*/   
	double standby_power;     /*the power when standby, watt*/
	double spindown_threshold; /*the time waiting in the idle state before a spindown, millisec*/
	double spindown_energy;   /*the energy during a spindown, joule*/
	double spindown_delay;    /*the latency during a spindown, millisec*/
	double spinup_energy;     /*the energy during a up, joule*/
	double spinup_delay;      /*the latency during a up, millisec*/
	double post_spinup_incr;  /*an incremental inteval for the events arriving during the disk spins up, millisec*/
}power_param_t;

typedef struct power_stat{
    int disk_hda_is_busy;             /* hda is being used or not */
    int disk_hda_spining_up;
    double disk_last_hda_off_time;   /* hda is released, maybe is still rotating */
    double disk_last_hda_on_time;    /* hda is inited to seek */
    double disk_last_service_time;   /* last request arrive time */
	int	   current_state;
	double post_spinup_time;
	
	double total_time_active;
	double total_time_idle;
	double total_time_standby;
    double total_runtime;
	double num_spindowns;
	double num_spinups;
}power_stat_t;


extern void power_internal_event(power_event *curr);
extern int power_waitfor_spinup(ioreq_event* curr);
extern void power_read_params(char* powerfile_name);
extern int power_manage_spindown(int devno, double time);
extern void power_stat_reset();
extern void power_initialization();
extern void power_set_idle(int devno);
extern void power_set_active(int devno);
void power_stat_show();
void power_cleanup();
extern void power_manage_release_hda(int devno, double time);
extern void power_manage_initiate_hda(int devno, double time);

#endif

