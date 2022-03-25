/* 'extern "C"' is the holy grail! */
#ifdef __cpp
extern "C"{
#endif
    extern void ak_drv_ptz_open();
    extern void ak_drv_ptz_check_self(int);
    extern void ak_drv_ptz_set_degree(int,int);
    extern void ak_drv_ptz_set_angle_rate(int,int);
    extern void ak_drv_ptz_turn(int, int, int );
    extern void ak_drv_ptz_turn_to_pos(int,int);
    extern void ak_drv_ptz_set_speed(int,int, int, int);

#ifdef __cpp
}
#endif