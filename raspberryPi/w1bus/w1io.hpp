#ifndef W1IO_H
#define W1IO_H

#include <string>
#include <gpiod.h>
//#include <gpiod.hpp>
#include <napi.h>

// wire one timing defaults
#define T_SLOT 65                           // driver bit period
#define T_WR1_LOW 4                         // write 1 low time
#define T_REC 10                            // onewire bus recovery time
#define T_SMPL 12                           // bus sample time after leading edge
#define T_RESET 480                         // bus reset low time
#define T_PRESENCE 30                       // bus presence detect sampling after T_RESET
#define T_RESET_WAIT T_RESET-(4*T_PRESENCE) // reset high recovery time

// Node Addon API wrapper for W1io...
class W1io : public Napi::ObjectWrap<W1io> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    W1io(const Napi::CallbackInfo& info);

  private:
    Napi::Value Cfg(const Napi::CallbackInfo& info);
    Napi::Value Reset(const Napi::CallbackInfo& info);
    Napi::Value BitIO(const Napi::CallbackInfo& info);
    Napi::Value BitSearch(const Napi::CallbackInfo& info);
    int bitBang(int bit, bool strong);
    int getValue();
    int setValue(int bit) {

//  Napi::Value ReadBit(const Napi::CallbackInfo& info);
//  Napi::Value ReadByte(const Napi::CallbackInfo& info);
//  Napi::Value ReadBytes(const Napi::CallbackInfo& info);
//  Napi::Value WriteBit(const Napi::CallbackInfo& info);
//  Napi::Value WriteByte(const Napi::CallbackInfo& info);
//  Napi::Value WriteBytes(const Napi::CallbackInfo& info);
//  Napi::Value SearchBit(const Napi::CallbackInfo& info);
//  Napi::Value Search(const Napi::CallbackInfo& info);  
    Napi::Value Busy(const Napi::CallbackInfo& info);

  std::string chipname = "gpiochip0";           // default gpiochip, RPi I/O header
//  gpiod::chip chip;                             // chip class instance for GPIO character device
  gpiod_chip chip;                             // chip class instance for GPIO character device
  unsigned int gpio = 4;                          // GPIO line number of bus
//  gpiod::line io;                               // line class instance of GPIO character device
  gpiod_line io;                               // line class instance of GPIO character device
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
