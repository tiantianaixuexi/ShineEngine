#pragma once




namespace shine::util{


     template<typename T>
    T get_now_ms_platform();

     template<typename T>
    T now_ns();

    extern  template float get_now_ms_platform<float>();
    extern  template double get_now_ms_platform<double>();
    extern  template unsigned long long get_now_ms_platform<unsigned long long>();

    extern  template float now_ns<float>();
    extern  template double now_ns<double>();
    extern  template unsigned long long now_ns<unsigned long long>();

}
