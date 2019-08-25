var SerialPort = require('serialport');
var schedule = require('node-schedule');


// USB AND WEBSOCKET MODEL
var model = {
    isPortOpen: false,
    port: null,
    portName: "/dev/ttyACM0",
    portRetry: false,
}

const Readline = SerialPort.parsers.Readline;

handleArduinoUpdate = function(msg) {
    // if (msg && msg.indexOf(":")) {
    //     var cmds = msg.split(":");

    //     if (cmds.length == 3) {
    //         var param = cmds[1];
    //         var value = cmds[2].trim();

    //         if (typeMap[param]) {
    //             var type = typeMap[param];

    //             if (param == "dispensed") {
    //                 //Finished dispensing, add as an entry
    //                 addDispenseEntry(value);
    //             }

    //             //console.log("---- UPDATE INFO ----");
    //             //console.log("Param: " + param);
    //             //console.log("Value: " + value);
    //             //console.log("Type: " + type);
    //             if (type == "number"){
    //                 feeder[param] = parseInt(value);
    //             } else if (type == "boolean") {
    //                 feeder[param] = !!parseInt(value);
    //             } else {
    //                 console.log("Unsupported type " + type);
    //                 return false;
    //             }
    //            // console.log(JSON.stringify(feeder, null, 4));
    //             updateToServer(param);
    //            // console.log("---------------------");
    //             return true;
    //         } else {
    //             console.log("No typeMap entry for " + param);
    //         }
    //     }
    // }
    // return false;
}


// -----  USB TO ARDUINO ------

model.setupPortConnection = function() {
    
    if (model.portRetry) {
        model.portName = "/dev/ttyACM1";
    }
    model.port = new SerialPort(model.portName, {
        baudRate: 9600
    });
    
    const parser = model.port.pipe(new Readline({ delimiter: '\n' }));
    
    model.port.on('error', function(err) {
        console.log("Error opening port");
        console.log(err);
        if (!model.portRetry) {
            console.log("Retrying on ACM1");
            model.portRetry = true;
            model.setupPortConnection();
        }
    });
    
    model.port.on('open', function() {
        console.log("opened");
        model.isPortOpen = true;
        
    });
    
    model.port.on('close', function() {
        console.log("WARNING! Serial connection closed!");
    })
    
    // -------------------------------------
    //   --    DATA FROM ARDUINO -----------
    // -------------------------------------
    parser.on('data', function(data) {
        handleArduinoUpdate(data);
    });
}

model.setupPortConnection();


model.dispenseFood = function(grams) {

   model.port.write('f:' + grams + "\n");

}

// SETUP JOBS
var aFood = schedule.scheduleJob('0 0 6 * * *', function() {
    model.dispenseFood(60);
});


var bFood = schedule.scheduleJob('0 0 16 * * *', function() {
    model.dispenseFood(60);
});
