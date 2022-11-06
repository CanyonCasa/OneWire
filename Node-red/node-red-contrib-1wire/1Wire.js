/**
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

let sleep = async function(ms) { return new Promise(resolve => setTimeout(resolve,ms)) };

module.exports = function(RED) {

  function oneWireNode(cfg) {
    RED.nodes.createNode(this,cfg);
    var node = this;
    this.identifier = cfg.identifier.trim();
    this.format = cfg.format;
    var fs = require("fs").promises;
    let logError = (this.context().global).get('myLib').logError;
    let delay = cfg['Read After Write Delay'] || 1000;  /// not presently implemented
    let fv = function firstValid() { return Array.from(arguments).find(e => e!==undefined); };

    node.on("input", function(msg,send,done) {
      (async () => {
        const RETRY = 3;
        let retry = RETRY;
        let sn = (this.identifier || msg.topic || '').toLowerCase();  // identifier overrides topic
        let topic = msg.label || sn;  // output topic defaults to sn if no label defined
        let family = sn.substring(0,2);
        let filebase = "/sys/bus/w1/devices/" + sn;
        let showStatus = (f,s,t) => node.status({fill:f||'red',style:s||'ring',text:t||sn+': NA'});

        // check family code to address different supported device types...
        if (['10','22','28','3b','42'].includes(family)) {
          // DS18B20 or other temperature sensor...
          retry = RETRY;
          while (retry) {
            try {
              let temp = await fs.readFile(filebase+'/temperature',"utf8");
              if (temp===undefined) throw new Error("Bad CRC/temperature detected!");
              // format and send good data only...
              let celcius = this.format==1 || !msg.farenheit;
              temp = celcius ? temp/1000 : (temp/1000 * 9/5) + 32; // C or F
              showStatus("blue","ring",sn+': '+temp.toFixed(3));
              send({topic: topic, payload: temp, sn: sn, time: +(new Date()),
                format: celcius?'Celcius':'Farenheit', units: celcius?'C':'F'});
              break;
            } catch(e) {
              node.warn("Wirenode Temp Error: "+e.toString());
              logError('OneWire',sn,e.toString());
              showStatus();
              retry--;
            };
          };
          done();

        } else if (['3a'].includes(family)) {
          // DS2413 2-bit I/O port...
          // DS2413 I/O port... write: payload => {a:<0|1>, b:<0|1>} or [a,b] OR read: payload => timestamp
          // Output msg also includes port object with decoded bit, hex, and state info.
	        // Special mode where A is an accuator output and B is the feedback status...
          //   input payload => off|OFF|on|ON|STATUS; output payload => OFF|ON
          // 
          let bit = (v) => v ? 1 : 0;
          let parse = (p) => ({ port: p, latchB: bit(p&0x8), pioB: bit(p&0x4), latchA: bit(p&0x2), pioA: bit(p&0x1),
            reg: bit(p&0x8)>>2+bit(p&0x2)>>1, hex: '0x'+p.toString(16).toUpperCase(), state: p&0x4?'ON':'OFF' });
          let port;
          retry = RETRY;
          while (retry) {
            try {
              // first get current port state, parse, and check
              port = parse((await fs.readFile(filebase+"/state"))[0]&0xFF);  // input as Buffer[1 byte]
              if (((~port.port&0xF0)>>4)!==(port.port&0xF)) throw new Error("Bad Port Read");
              break;
            } catch(e) {
              node.warn("Wirenode Port Read Error: "+e.toString());
              logError('OneWire',sn,e.toString());
              showStatus();
              retry--;
              if (!retry) return done();
              await sleep(delay);
            };
          };
          let special = (typeof msg.payload=='string');
          let noChange = special && (port.state == msg.payload.toUpperCase());
//          node.log(`special: ${special}, noChange: ${noChange}, payload: ${msg.payload}, type: ${typeof msg.payload}`);
          if (typeof msg.payload=='number' || (special && msg.payload.toLowerCase()=='status') || !msg.payload || noChange) { 
            // assume number is timestamp; or if payload 'status' or empty, or new state==old state, then just read...
            send({ topic: topic, payload: special?port.state:port.port, port: port, sn: sn});
            showStatus("blue","ring",`${sn}: ${port.hex} (${port.state})`);
            return done();
          };
          let [a,b] = special ? (msg.payload.toLowerCase()=='on' ? [0,1] : [1,1]) :
            (msg.payload instanceof Array ? [fv(msg.payload[0],port.latchA),fv(msg.payload[1],port.latchB)] :
            [fv(msg.payload.a,msg.payload.A,port.latchA),fv(msg.payload.b,msg.payload.B,port.latchB)]);
          let data = Buffer.from([0xFF&((b<<1)+a)]);
          retry = RETRY;
          while (retry) {
            try {
              // requires that the device output file have write permission as in: -rw-rw-r--  root gpio ... output
              await fs.writeFile(filebase+'/output',data);
              await sleep(delay);
              port = parse((await fs.readFile(filebase+"/state"))[0]&0xFF);  // input as Buffer[1 byte]
              if (((~port.port&0xF0)>>4)!==(port.port&0xF)) throw new Error("Bad Port Read");
              send({ topic: topic, payload: special?port.state:port.port, port: port, sn: sn});
              showStatus("green","ring",`${sn}: ${port.hex}${special ? ' ('+port.state+')' : ''}`);
              break;
            } catch (e) {
              node.warn(`Wirenode Port Write Error[${sn}]: (${data}) ${e.toString()}`);
              logError('OneWire',sn,e.toString());
              showStatus();
              retry--;
              await sleep(delay);
            };
          };
          done();

        } else if (['29'].includes(family)) {
          // DS2408 8-bit I/O port...
          // DS2408 I/O port... write payload => {a:<0|1>, b:<0|1>, ..., h:<0|1>} or [a,b,...,h]
          //   or a numeric string (not a number) OR read payload => timestamp
          const bx = ['h', 'g', 'f', 'e', 'd', 'c', 'b', 'a'];
          let bitStr = (p) => ('0000000'+p.toString(2)).slice(-8);
          let parse8 = (p) => { 
              pp = {decimal: p, hex: '0x'+('0'+p.toString(16)).slice(-2).toUpperCase(),
                binary: '0b'+bitStr(p), bits: bitStr(p).split('')};
              bx.forEach((b,i)=>{pp[b] = pp.bits[i]});
              return pp;
            };
          let port;
          retry = RETRY;
//          node.warn(`${sn}[${msg.src}]: ${JSON.stringify(msg.payload)}`);
          while (retry) {
            try {
              // first get current port state, parse, and check
              port = parse8((await fs.readFile(filebase+"/state"))[0]&0xFF);  // input as Buffer[1 byte]
//              node.log(`Port Read[${sn}]: ${port.hex}`);
              break;
            } catch(e) {
              node.warn("Wirenode Port Read Error: "+e.toString());
              logError('OneWire',sn,e.toString());
              showStatus();
              retry--;
              if (!retry) return done();
              await sleep(delay);  
            };
          };
          if (((typeof msg.payload=='number') && (msg.payload>255)) || !msg.payload) { 
            // assume large number is timestamp or empty payload (i.e. null, '', or undefined), just read...
            send({ topic: topic, payload: port.decimal, port: port, sn: sn});
            showStatus("blue","ring",`${sn}: ${port.hex}`);
            return done();
          };
          let reg = '0b'+(
            ((typeof msg.payload=='string') || (typeof msg.payload=='number')) ? bitStr(+msg.payload).split('') :
            (msg.payload instanceof Array) ? msg.payload : 
            (typeof msg.payload=='object') ? bx.map(k=>msg.payload[k]===undefined?port[k]:msg.payload[k]) : port.bits).join('');
          let regBuffer = Buffer.from([0xFF&(+reg)]);
            retry = RETRY;
          while (retry) {
            try {
              // requires that Node-red have write permission for the device output file as in: -rw-rw-r-- root gpio ... output
              await fs.writeFile(filebase+'/output',regBuffer);
              await sleep(delay);
//              node.log(`Port Write[${sn}]: ${reg}`);
              port = parse8((await fs.readFile(filebase+"/state"))[0]&0xFF);      // re-read port after action
//              node.log(`Port re-Read[${sn}]: ${port.hex}`);
              send({ topic: topic, payload: port.decimal, port: port, data: regBuffer[0], sn: sn});
              showStatus("green","ring",`${sn}: ${port.hex}`);
              break;
            } catch (e) {
              node.warn(`(${retry}) Wirenode Port Write Error[${sn}]: (${reg}) ${e.toString()}`);
              logError('OneWire',sn,e.toString());
              showStatus();
              retry--;
              if (!retry) break;
              await sleep(delay);
            };
          };
          done();

        } else {
          /// TBD...
          send({ topic: topic, payload: "TBD" });
          // Add generic support...
          showStatus();
          done();

        };
      })();
    });
  };

  RED.nodes.registerType("1-Wire",oneWireNode);

}
