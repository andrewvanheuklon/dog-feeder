var SerialPort = require('serialport');
var schedule = require('node-schedule');
var express = require('express');
var bodyParser = require('body-parser');
const WebSocket = require('ws');
var request = require('request');
var q = require('q');

const serverKey = "gGJtSsV8BM7w018d39Ji57F8iO6c0N2GZq3RY2NhI";
const dispenseTrackingCount = 100;
/**
   ----  FEEDER MODEL  ------
*/
var feeder = {
    totalWeight: 0,
    dispensed: 0,
    dispenses: [],
    state: 0, //0 = wait from commands, 1 = dispensing, 2 = filling
    fillingNowGrams: null, //Updated as the weight is added
    dispensingNowGrams: null, //Updated as the weight is dispensed
    dispensingNowTargetGrams: null, //Set when the dispensing begins
    requestedGrams: 0,
    isLidOpen: false,
    lastFillAmount: 0,
    enabled: true
}

var typeMap = {
    "totalWeight": "number",
    "dispensed": "number",
    "isLidOpen": "boolean",
    "lastFillAmount": "number",
    "dispensingNowGrams": "number",
    "dispensingNowTargetGrams": "number",
    "fillingNowGrams": "number",
    "state": "number"
}

// USB AND WEBSOCKET MODEL
var model = {
    isPortOpen: false,
    port: null,
    portName: "/dev/ttyACM0",
    portRetry: false,
    isConnectedToServer: false,
    phoneMACS: {
        "DC:EF:CA:F2:72:B1": {name: "Andrew"},
        "F4:0F:24:D1:63:D7": {name: "Mikayla"}
    },
    routerAttachedDevicesURL: "http://www.routerlogin.net/QOS_device_info.htm",
    routerAuth: "Basic YWRtaW46YW5kcjN3djRu"
}

var wsRetryInterval;
var ws;

// Setup Express
var app = express();
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies
var port = 8080;

const Readline = SerialPort.parsers.Readline;



// --- WEBSOCKET RETRY ---
var startWebsocketRetry = function() {
    wsRetryInterval = setInterval(function() {
        console.log("ws retry..");
        setupServerWebsocket();
    }, 30 * 1000)
}

var stopWebsocketRetry = function() {
    if (wsRetryInterval) {
        clearInterval(wsRetryInterval);
    }
    wsRetryInterval = null;
}

// ---- WEB SOCKET ------
var setupServerWebsocket = function() {

    ws = new WebSocket('ws:142.93.240.124:3000');

    console.log("Opening websocket...");

    ws.on('open', function open() {
        stopWebsocketRetry();
        model.isConnectedToServer = true;

        console.log("Websocket opened");
        ws.send(serverKey);

        //Wait a sec, then send the current model
        setTimeout(function() {
            updateToServer("init connection");
        }, 100)
    });

    ws.on('message', function incoming(data) {
      //console.log("FROM SERVER:", data);

      if (data && data.indexOf("CMD:") > -1) {
        //Got a command from the server
        var cmd = data.split(":");
        //console.log("Command recieved from server");
        if (cmd.length >= 2) {
            var funcName = cmd[1];
            var funcArg = cmd.length == 3 ? cmd[2] : null;

            //console.log("Func: " + funcName);
            //console.log("Arg: " + funcArg);

            if (model[funcName]) {
                //console.log("Found func.. calling");
                if (funcArg != null) {
                    model[funcName](funcArg);
                } else {
                    model[funcName]();
                }
            } else {
                console.log("Not such function");
            }
        }
      }
    });

    ws.on('close', function closed() {
        console.log("Connection to server was closed!");
        model.isConnectedToServer = false;
        startWebsocketRetry();
    })

    ws.on('error', function onerror(err) {
        model.isConnectedToServer = false;
        startWebsocketRetry();

        console.log("error on ws");
        console.log(err);
    });
}
setupServerWebsocket();

updateToServer = function(param) {
    if (ws && model.isConnectedToServer) {
        var modelCopy = JSON.parse(JSON.stringify(feeder));
        modelCopy.updatedFrom = param;

        ws.send(JSON.stringify(modelCopy));
    }
    
}

// ----- LOCAL APIS -------

app.listen(port);
console.log("Server attached at " + port);

app.post('/dispense', function(req, res) {
    var amount = req.body.amount;

    if (!feeder.enabled) {
        res.send("Feeder is not enabled");
        return;
    }

    if (amount) {
        console.log("API Dispense: " + amount);
        if (model.isPortOpen) {
            res.send("Dispensing " + amount + " grams");
            model.dispenseFood(amount);
        } else {
            res.send("Port is not open");
        }
    } else {
        res.send("No amount specified");
    }
});

app.post('/setweight', function(req, res) {
    var amount = req.body.amount;

    if (amount) {
        console.log("API Set Weight: " + amount);
        if (model.isPortOpen) {
            res.send("Setting weight to " + amount + " grams");
            model.setWeight(amount);
        } else {
            res.send("Port is not open");
        }
    } else {
        res.send("No amount specified");
    }
});

app.post('/setstopms', function(req, res) {
    var ms = req.body.ms;

    if (ms) {
        console.log("API Set Stop MS: " + ms);
        if (model.isPortOpen) {
            res.send("Setting stop ms to " + ms + " ms");
            model.setStopMS(ms);
        } else {
            res.send("Port is not open");
        }
    } else {
        res.send("No ms property specified");
    }
});

app.post('/empty', function(req, res) {

    console.log("API Empty the feeder: ");
    if (model.isPortOpen) {
        res.send("Sending empty command ");
        model.emptyTheFeeder();
    } else {
        res.send("Port is not open");
    }
    
});

app.post("/people", function(req, res) {
    console.log("Checking whose home");

    model.isSomebodyHome().then(function(data) {
        if (data) {
            if (data.error) {
                res.send(data.error + "\n" + data.body);
            } else if (data.people) {
                res.send(data.people.join(", "));
            } else {
                res.send("unknown data");
            }
        } else {
            res.send("Nobody");
        }
    }).finally(function() {
        res.send("error");
    });
});

app.post('/enable', function(req, res) {

    console.log("API Disabled the feeder: ");
    if (feeder.enabled) {
       res.send("Feeder already enabled");
    } else {
      
       res.send("Toggled on");
    }
     model.toggleOn();
    
});

app.post('/disable', function(req, res) {

    console.log("API Disabled the feeder: ");
    if (!feeder.enabled) {
       res.send("Feeder already disabled");
    } else {
       
       res.send("Toggled off");
    }
    model.toggleOff();
    
});

addDispenseEntry = function(grams) {
    if (feeder.dispenses.length >= dispenseTrackingCount) {
        feeder.dispenses = feeder.dispenses.slice(0, dispenseTrackingCount);
    }
    //console.log("Adding to dispenses");
    var d =  {
        timestamp: Date.now(),
        requested: feeder.requestedGrams,
        dispensed: parseInt(grams)
    };
    feeder.dispenses.unshift(d);
   
}

handleArduinoUpdate = function(msg) {
    if (msg && msg.indexOf(":")) {
        var cmds = msg.split(":");

        if (cmds.length == 3) {
            var param = cmds[1];
            var value = cmds[2].trim();

            if (typeMap[param]) {
                var type = typeMap[param];

                if (param == "dispensed") {
                    //Finished dispensing, add as an entry
                    addDispenseEntry(value);
                }

                //console.log("---- UPDATE INFO ----");
                //console.log("Param: " + param);
                //console.log("Value: " + value);
                //console.log("Type: " + type);
                if (type == "number"){
                    feeder[param] = parseInt(value);
                } else if (type == "boolean") {
                    feeder[param] = !!parseInt(value);
                } else {
                    console.log("Unsupported type " + type);
                    return false;
                }
               // console.log(JSON.stringify(feeder, null, 4));
                updateToServer(param);
               // console.log("---------------------");
                return true;
            } else {
                console.log("No typeMap entry for " + param);
            }
        }
    }
    return false;
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
        setTimeout(function() {
            console.log("Write stop");
            model.port.write("s\n")
        }, 5100);
        setTimeout(function() {
            console.log("Stop ms");
            model.setStopMS(1455);
        }, 5000);
        
    });
    
    model.port.on('close', function() {
        console.log("WARNING! Serial connection closed!");
    })
    
    // -------------------------------------
    //   --    DATA FROM ARDUINO -----------
    // -------------------------------------
    parser.on('data', function(data) {
        //If the handler returns false, it's not a valid command so just print it
        if (!handleArduinoUpdate(data)) {
           // console.log("from arduino: " + data);
        }
        
    });
}

model.setupPortConnection();


model.dispenseFood = function(grams) {
    if (feeder.enabled) {
        //Add the request to the model. Used later in the entry of the dispenses
        feeder.requestedGrams = grams;

        model.port.write('d' + grams + "\n");
    } else {
        console.log("Skipping feed");
    }
}

model.slowFeed = function(grams) {
    if (feeder.enabled) {
        //Add the request to the model.
        feeder.requestedGrams = grams;

        model.port.write('f' + grams + "\n");
    } else {
        console.log("Skipping feed");
    }
    
}

model.slowFeedIfAlone = function(grams) {
    model.isSomebodyHome().then(function(isHome) {
        //Somebody is home, so regular feed
        if (isHome) {
            model.dispenseFood(grams);
        } else {
            //Try one more time, sometimes the auth fails on the first try
            console.log("Failed the first router call, try again")
            setTimeout(function() {
                model.isSomebodyHome().then(function(isHome2) {
                    if (isHome2) {
                        console.log("Success the second time");
                        model.dispenseFood(grams);
                    } else {
                        model.slowFeed(grams);
                    }
                })
            }, 2000);
        }
    });
}

model.isSomebodyHome = function() {
    var defer = q.defer();

    var url = model.routerAttachedDevicesURL + "?ts=" + new Date().getTime();
    request({
        url: url,
        headers: {
            "Authorization": "Basic YWRtaW46YW5kcjN3djRu"
        }
    }, function(error, response, body) {
        if (error == null && body) {
           // console.log(" ---- ");
           // console.log(body);
            try {
                var devices = eval(body);
                if (devices && devices.length > 0) {
                    var somebodyIsHome = false;
                    var peopleHome = [];
                    devices.forEach(function(device) {
                        if (device && device.mac && model.phoneMACS[device.mac]) {
                            somebodyIsHome = true;
                            peopleHome.push(model.phoneMACS[device.mac].name);
                            console.log(model.phoneMACS[device.mac].name + " is home.");
                        }
                    });
                }
                if (somebodyIsHome) {
                    defer.resolve({people: peopleHome});
                } else {
                    defer.resolve(false);
                }
            } catch(e) {
                console.log("Could not parse router request to JSON");
                console.log(e);
                console.log(body);
                defer.resolve({error: e, body: body});
            }
        } else {
            defer.resolve(false);
        }
       
    });
    return defer.promise;
}

model.setStopMS = function(ms) {
    model.port.write('t' + ms + "\n");
}

model.emptyTheFeeder = function() {
    model.port.write('e\n');
}

model.setWeight = function(grams) {
    model.port.write('i' + grams + "\n");
}

model.warnIfLow = function() {
    model.port.write("w\n");
}

model.toggleOn = function() {
    feeder.enabled = true;
    updateToServer("/enable");
}

model.toggleOff = function() {
    feeder.enabled = false;
    updateToServer("/disable");
}

 
// SETUP JOBS
var aFood = schedule.scheduleJob('0 0 6 * * *', function() {
    model.dispenseFood(60);
});


var c3Food = schedule.scheduleJob('0 0 16 * * *', function() {
    model.slowFeedIfAlone(60);
});

/**
 * WARN IF LOW
 */
var warn1 = schedule.scheduleJob('0 0 19 * * *', function() {
    model.warnIfLow();
});

var warn2 = schedule.scheduleJob('0 0 15 * * *', function() {
    model.warnIfLow();
});

/**
 * PING THE SERVER
 */
 var pingpong = schedule.scheduleJob('0 15 * * * *', function() {
    if (ws) {
        try {
          //  console.log("Sending PING");
            ws.send("ping");
        } catch(e) {
          //  console.log("Error on ping!");
        }
    }
 });
