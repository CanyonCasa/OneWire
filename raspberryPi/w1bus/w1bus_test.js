//var bus = require('bindings')('w1bus');
var bus = require('./build/Release/w1bus');

try {
    var w1 = new bus.W1io();
    console.log(w1);
    console.log( "w1 cfg:", w1.cfg() );
    console.log(`busy[default]: ${w1.busy()}`);
    w1.busy(true);
    console.log(`busy[set]]: ${w1.busy()}`);
    w1.busy(false);
    console.log(`busy[clear]]: ${w1.busy()}`);
    w1.reset();
} catch(e) {
    console.log(`w1: `,e);
}

try {
    var w2 = new bus.W1io({chipname: "gpiochip0", gpio: 10, tslot:75});
    console.log( "w2 cfg:", w2.cfg() );
} catch(e) {
    console.log(`w2: `,e.toString());
}
console.log( "w1 cfg:", w1.cfg() );
console.log('done');
