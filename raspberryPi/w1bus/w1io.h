#ifndef W1IO_H
#define W1IO_H

#include <string>
#include <gpiod.hpp>
//#include <napi.h>
#include <node_api.h>


// wire one timing defaults
#define T_SLOT 65                           // driver bit period
#define T_WR1_LOW 4                         // write 1 low time
#define T_REC 10                            // onewire bus recovery time
#define T_SMPL 12                           // bus sample time after leading edge
#define T_RESET 480                         // bus reset low time
#define T_PRESENCE 30                       // bus presence detect sampling after T_RESET
#define T_RESET_WAIT T_RESET-(4*T_PRESENCE) // reset high recovery time
//#define AS_OUTPUT gpiod::line_request::DIRECTION_OUTPUT
//#define AS_OUTPUT GPIOD_LINE_REQUEST_DIRECTION_OUTPUT
//#define OPEN_DRAIN_PULL_UP GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN | GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP
//#define OPEN_DRAIN_PULL_UP gpiod::line_request::FLAG_OPEN_DRAIN


// Node Addon API wrapper for W1io...
class W1io {
  public:
    static napi_value Init(napi_env env, napi_value exports);
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    //W1io(napi_env env, napi_callback_info info);

  private:
    explicit W1io(napi_env env, napi_callback_info info)
    ~W1io();
    
    static napi_value Cfg(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value BitIO(napi_env env, napi_callback_info info);
    static napi_value BitSearch(napi_env env, napi_callback_info info);
    static int bitBang(int bit, bool strong);

//  static napi_value ReadBit(napi_env env, napi_callback_info info);
//  static napi_value ReadByte(napi_env env, napi_callback_info info);
//  static napi_value ReadBytes(napi_env env, napi_callback_info info);
//  static napi_value WriteBit(napi_env env, napi_callback_info info);
//  static napi_value WriteByte(napi_env env, napi_callback_info info);
//  static napi_value WriteBytes(napi_env env, napi_callback_info info);
//  static napi_value SearchBit(napi_env env, napi_callback_info info);
//  static napi_value Search(napi_env env, napi_callback_info info);  
    static napi_value Busy(napi_env env, napi_callback_info info);

  std::string chipname = "gpiochip0";           // default gpiochip, RPi I/O header
  gpiod::chip chip;                             // chip class instance for GPIO character device
  int gpio = 4;                                 // GPIO line number of bus
  gpiod::line io;                               // line class instance of GPIO character device
  std::string consumer = "w1io";                // user space consumer
  struct timing {                               // default timing parameters
    int slot;
    int rec;
    int smpl;
    int wr1;
    int smplx;
    int wr1slot;
  } t = { T_SLOT, T_REC, T_SMPL, T_WR1_LOW, T_SMPL-T_WR1_LOW, T_SLOT-T_SMPL };
  bool busBusy = false;
  bool ok = false;
};

#endif
