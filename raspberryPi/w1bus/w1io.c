//extern "C" {
  //#include <delay.h>
  #include <unistd.h>
//}
#include <string>
#include <gpiod.hpp>
#include "w1io.hpp"

Napi::Object W1io::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "W1io", { 
    InstanceMethod("cfg", &W1io::Cfg),
    InstanceMethod("reset", &W1io::Reset),
    InstanceMethod("bitIO", &W1io::BitIO),
    InstanceMethod("bitSearch", &W1io::BitSearch),
/*                   InstanceMethod("readBit", &W1io::ReadBit),
                   InstanceMethod("readByte", &W1io::ReadByte),
                   InstanceMethod("readBytes", &W1io::ReadBytes),
                   InstanceMethod("writeBit", &W1io::WriteBit),
                   InstanceMethod("writeByte", &W1io::WriteByte),
                   InstanceMethod("writeBytes", &W1io::WriteBytes),
                   InstanceMethod("search", &W1io::Search),
*/                   
    InstanceMethod("busy", &W1io::Busy)});

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("W1io", func);
  return exports;
}

W1io::W1io(const Napi::CallbackInfo& info) : Napi::ObjectWrap<W1io>(info) {
  Napi::Env env = info.Env();

  this->Cfg(info);

  try {
    this->chip = gpiod::chip(this->chipname);
  } catch(...) {
      Napi::Error::New(env, "Cannot open GPIO device: "+this->chipname).ThrowAsJavaScriptException();
      return;
  }
  try {
    this->io = this->chip.get_line(this->gpio);
  } catch(...) {
      Napi::Error::New(env, "Cannot open GPIO line: "+std::to_string(this->gpio)).ThrowAsJavaScriptException();
      return;
   }
  try {
    this->io.request({ this->consumer, AS_OUTPUT, OPEN_DRAIN_PULL_UP }, 1);
  } catch(...) {
      Napi::Error::New(env, "Cannot access GPIO line: "+std::to_string(this->gpio)).ThrowAsJavaScriptException();
      return;
  }

  this->ok = true;
}

// configure the oneWire bus ...
// takes gpio pin number and/or parameters object
Napi::Value W1io::Cfg(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Object obj = Napi::Object::New(env);
  int length = info.Length();

  // look for optional parameters object
  if (length == 1) {
    if (!info[0].IsObject()) {
       Napi::TypeError::New(env, "OneWire parameters object expected").ThrowAsJavaScriptException();
       return obj;
    } else {
      Napi::Object params = info[0].As<Napi::Object>();
      if (!this->ok) {
        // can only be changed from constructor call, not on the fly
        // optional bus gpio chipname and pin, default gpiochip0:4
        if (params.Has("chipname")) this->chipname = Napi::String(env, params.Get("chipname")).Utf8Value();
        if (params.Has("gpio")) this->gpio = Napi::Number(env, params.Get("gpio")).Int32Value();
        if (params.Has("consumer")) this->consumer = Napi::String(env, params.Get("consumer")).Utf8Value();
      }
      // optional timing parameters...
      if (params.Has("tslot")) this->t.slot = Napi::Number(env, params.Get("tslot")).Int32Value();
      if (params.Has("trec")) this->t.rec = Napi::Number(env, params.Get("trec")).Int32Value();
      if (params.Has("twr1")) this->t.wr1 = Napi::Number(env, params.Get("twr1")).Int32Value();
      if (params.Has("tspml")) this->t.smpl = Napi::Number(env, params.Get("tsmpl")).Int32Value();
      this->t.smplx = this->t.smpl - this->t.wr1;
      this->t.wr1slot = this->t.slot - this->t.smpl;
    }
  }

  Napi::Object timing = Napi::Object::New(env);
  obj.Set("chipname",this->chipname);
  obj.Set("gpio",this->gpio);
  obj.Set("consumer",this->consumer);
  timing.Set("slot",this->t.slot);
  timing.Set("rec",this->t.rec);
  timing.Set("smpl",this->t.smpl);
  timing.Set("wr1",this->t.wr1);
  timing.Set("smplx",this->t.smplx);
  timing.Set("wr1slot",this->t.wr1slot);
  obj.Set("timing",timing);
  obj.Set("busy",this->busBusy);
  obj.Set("ok",this->ok);
  Napi::Object flags = Napi::Object::New(env);
  //flags.Set("output",AS_OUTPUT);
  //flags.Set("mode",OPEN_DRAIN_PULL_UP);
  obj.Set("flags",flags);

  return obj;
}

// perform a oneWire bus reset...
Napi::Value W1io::Reset(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // perform a bus reset...
  this->io.set_flags(OPEN_DRAIN_PULL_UP);  // set to open drain to terminate any strong drive
  this->io.set_value(0);                   // set value to drive 0
  usleep(T_RESET);                        // delay T_RESET
  this->io.set_value(1);                   // set value to pullup 1
  // detect device presence at 1x, 2x, and 4x times for robust detection
  usleep(T_PRESENCE);                     // delay T_PRESENCE
  bool present = this->io.get_value();     // get value of pin (wired-OR), 0==found
  usleep(T_PRESENCE);                     // delay T_PRESENCE
  present &= this->io.get_value();         // get value of pin (wired-OR)
  usleep(T_PRESENCE<<1);                  // delay 2 * T_PRESENCE
  present &= this->io.get_value();         // get value of pin (wired-OR)
  usleep(T_RESET_WAIT);                   // delay reset recovery

  return Napi::Number::New(env, present); // return found==0
  
}

// performs single bit read/write operations
Napi::Value W1io::BitIO(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int length = info.Length();
  int bit;
  int strong = false;
  if (length==2 && info[1].IsBoolean()) {
    strong = info[1].As<Napi::Boolean>().Value();   // retrieve strong flag
  }
  if (length>=1 && info[0].IsNumber()) {
    bit = info[0].As<Napi::Number>().Int32Value();  // retrieve bit value
  } else {
    Napi::TypeError::New(env, "Bit value [and termination strength] expected").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  
  bit = this->bitBang(bit, strong);     // output bit and get response
  return Napi::Number::New(env, bit);
}

// performs single bit read/write operations in search mode
Napi::Value W1io::BitSearch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length()!=1 && !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Search bit write value expected").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  } 
  int bit = info[0].As<Napi::Number>().Int32Value();
  int t = this->bitBang(1,false);   // read true bit
  int c = this->bitBang(1,false);   // read complement bit
  // write bit logic...
  return Napi::Number::New(env, bit);
}

int W1io::bitBang(int bit, bool strong) {
  int rd;
  if (bit) {
    // write 1 (and read bus)
    this->io.set_value(0);
    usleep(this->t.wr1);
    this->io.set_value(1);
    usleep(this->t.smplx);
    rd = this->io.get_value();
    usleep(this->t.wr1slot);
  } else {
    // write 0 
    this->io.set_value(0);
    usleep(this->t.slot);
    this->io.set_value(1);
    if (strong) this->io.set_flags(0);   // set output driven
    rd = 0;
  }
  usleep(this->t.rec);
  return rd;
}

// get or set bus busy flag...
Napi::Value W1io::Busy(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int length = info.Length();

  if (length == 1 && info[0].IsBoolean()) {
    this->busBusy = info[0].As<Napi::Boolean>().Value();
  }

  return Napi::Boolean::New(env, this->busBusy);
}
