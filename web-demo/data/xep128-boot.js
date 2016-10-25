/* Xemu/Xep128 Emscripten "booter" and option/FS parser + file XHR downloader.
 * Some code based on (or the original) the Emscripten generated code. Other parts:
  
Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
http://xep128.lgb.hu/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


var Xemu = {
	"emulatorId":		"",
	"memoryInitializer":	"<XEMU>.html.mem",
	"emulatorJS":		"<XEMU>.js",
	"fileFetchList":	[],
	"URLpostfix":		"",
	"URLprefix":		"",
	"fileObjects":		{},		// internal!
	"filesFetched":		1,		// internal!
	"fileFetchError":	0,		// internal!
	"fileWaiters":		0,		// internal!
	"arguments":		[],
	"statusElementId":	"status",
	"progressElementId":	"progress",
	"spinnerElementId":	"spinner",
	"outputElementId":	"output",
	"canvasElementId":	"canvas"
};


      var statusElement = document.getElementById(Xemu.statusElementId);
      var progressElement = document.getElementById(Xemu.progressElementId);
      var spinnerElement = document.getElementById(Xemu.spinnerElementId);

      Xemu.Module = {
        preRun: [
		(function () {
			var a;
			/* Populate fetched objects to the MEMFS */
			FS.mkdir("/files");
			for (a in Xemu.fileObjects) {
				console.log("JS-Laucher/preRun: populating file: " + a);
				FS.writeFile("/files/" + a, Xemu.fileObjects[a], { encoding: "binary" });
				Xemu.fileObjects[a] = undefined;
			}
			Xemu.fileObjects = undefined;
			ENV["BROWSER"] = navigator.userAgent;
			ENV["LOCATION"] = String(window.location);
			for (a in ENV)
				console.log("ENV[%s]=%s", a, ENV[a]);
			//console.log(FS);
			// free
			Xemu = undefined;
		 })
	],
        postRun: [],
        print: (function() {
          var element = document.getElementById('output');
          if (element) element.value = ''; // clear browser cache
          return function(text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            // These replacements are necessary if you render to raw HTML
            //text = text.replace(/&/g, "&amp;");
            //text = text.replace(/</g, "&lt;");
            //text = text.replace(/>/g, "&gt;");
            //text = text.replace('\n', '<br>', 'g');
            console.log(text);
            if (element) {
              element.value += text + "\n";
              element.scrollTop = element.scrollHeight; // focus on bottom
            }
          };
        })(),
        printErr: function(text) {
          if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
          if (0) { // XXX disabled for safety typeof dump == 'function') {
            dump(text + '\n'); // fast, straight to the real console
          } else {
            console.error(text);
          }
        },
        canvas: (function() {
          var canvas = document.getElementById('canvas');
	  canvas.oncontextmenu = function (e) { e.preventDefault(); };
	  console.log("CANVAS function called!");

          // As a default initial behavior, pop up an alert when webgl context is lost. To make your
          // application robust, you may want to override this behavior before shipping!
          // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
          canvas.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

          return canvas;
        })(),
        setStatus: function(text) {
          if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
          if (text === Module.setStatus.text) return;
          var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
          var now = Date.now();
          if (m && now - Date.now() < 30) return; // if this is a progress update, skip it if too soon
          if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2])*100;
            progressElement.max = parseInt(m[4])*100;
            progressElement.hidden = false;
            spinnerElement.hidden = false;
          } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.style.display = 'none';
          }
          statusElement.innerHTML = text;
        },
        totalDependencies: 0,
        monitorRunDependencies: function(left) {
          this.totalDependencies = Math.max(this.totalDependencies, left);
          Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
        }
      };
      
/*
	Module.setStatus('Downloading...');
      window.onerror = function(event) {
        // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
        Module.setStatus('Exception thrown, see JavaScript console');
        spinnerElement.style.display = 'none';
        Module.setStatus = function(text) {
          if (text) Module.printErr('[post-exception status] ' + text);
        };
      };*/

/*	function LaunchXemu() {
	  console.log("JS-Launcher: XHR for meminit ...");

          (function() {
            var memoryInitializer = 'xep128.html.mem';
            if (typeof Module['locateFile'] === 'function') {
              memoryInitializer = Module['locateFile'](memoryInitializer);
            } else if (Module['memoryInitializerPrefixURL']) {
              memoryInitializer = Module['memoryInitializerPrefixURL'] + memoryInitializer;
            }
            var xhr = Module['memoryInitializerRequest'] = new XMLHttpRequest();
	    console.log("memoryInitializer XHR from: " + memoryInitializer);
            xhr.open('GET', memoryInitializer, true);
            xhr.responseType = 'arraybuffer';
            xhr.send(null);
          })();
	  console.log("JS-Launcher: SCRIPT tag for the emulator ...");

          var script = document.createElement('script');
          script.src = "xep128.js";
          document.body.appendChild(script);
	}*/


Xemu.requestFullScreen = Xemu.Module.requestFullScreen;


Xemu.start = function (user_settings) {
	"use strict";
	var item;
	if (user_settings === null || typeof user_settings !== "object") {
		window.alert("An object parameter is needed for Xemu.start()");
		return;
	}
	for (item in user_settings)
		Xemu[item] = user_settings[item];
	user_settings = undefined;
	if (Xemu.emulatorId == "") {
		window.alert("emulatorId was not defined for Xemu.start()");
		return;
	}
	// Well, yes, I know, I can use EXPORT_NAME for emcc, or MODULARIZE stuffs ...
	// Anyway, it's good enough for me this way for a starting point even if it's very ugly ...
	window.Module = Xemu.Module;
	


	Module.setStatus('Downloading...');
      window.onerror = function(event) {
        // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
        Module.setStatus('Exception thrown, see JavaScript console');
        spinnerElement.style.display = 'none';
        Module.setStatus = function(text) {
          if (text) Module.printErr('[post-exception status] ' + text);
        };
      };


	item = document.createElement("style");
	item.innerText = "";
	document.head.appendChild(item);


	Module.arguments = Xemu.arguments;
	Xemu.memoryInitializer = Xemu.memoryInitializer.replace("<XEMU>", Xemu.emulatorId);
	Xemu.emulatorJS = Xemu.emulatorJS.replace("<XEMU>", Xemu.emulatorId);
	Xemu.fileFetchList.push("!!" + Xemu.memoryInitializer);
	function bootEmulator ( reason ) {
		// TODO: URLize here too ....
		console.log("Booting emulator using " + Xemu.emulatorJS + ": " + reason);
		var script = document.createElement("script");
		script.src = Xemu.emulatorJS;
		document.body.appendChild(script);
	}
	function downloadFile ( name ) {
		var xhr = new XMLHttpRequest();
		var url;
		function downloadFileHandler ( resp ) {
			if (Xemu.fileFetchError)
				return;
			console.log("Download (" + (Xemu.filesFetched + 1) + "/" + Xemu.fileFetchList.length + ") " + name + " as " + url + " [" + xhr.status + " " + xhr.statusText + "]");
			if (xhr.status != 200) {
				Xemu.fileFetchError = 1;
				window.alert("Xemu loader: Cannot download file " + name + " as " + url + ", error: " + xhr.status + " " + xhr.statusText);
				return;
			}
			console.log(resp);
			console.log(resp.currentTarget);
			console.log(xhr);
			console.log(this);
			Xemu.fileObjects[name] = new Uint8Array(xhr.response);
			xhr = undefined;
			console.log("Download status: itemsDone = " + Xemu.filesFetched + ", allitems = " + Xemu.fileFetchList.length);
			Xemu.filesFetched++;
			if (Xemu.filesFetched == Xemu.fileFetchList.length)
				bootEmulator("all file-only downloads are done.");
		}
		name = name.replace("<XEMU>", Xemu.emulatorId);
		if (name == "!!" + Xemu.memoryInitializer) {
			name = name.substr(2);
			Module['memoryInitializerRequest'] = xhr;
		} else {
			Xemu.fileWaiters++;
			xhr.addEventListener("load",  downloadFileHandler);
			xhr.addEventListener("error", downloadFileHandler);
			xhr.addEventListener("abort", downloadFileHandler);
		}
		if (name.search("://") != -1) {
			url = name;
		} else if (Xemu.URLprefix != "") {
			url = Xemu.URLprefix + name + Xemu.URLpostfix;
		} else if (typeof Module['locateFile'] === 'function') {
			url = Module['locateFile'](name) + Xemu.URLpostfix;
		} else if (Module['memoryInitializerPrefixURL']) {
			url = Module['memoryInitializerPrefixURL'] + name + Xemu.URLpostfix;
		} else {
			url = name + Xemu.URLpostfix;
		}
		name = name.replace(/\?.*$/, "").replace(/^.*\/(.*)$/, "$1").trim();
		url = url.replace("<XEMU>", Xemu.emulatorId);
		if (name == "") {
			window.alert("Bad file name");
			Xemu.fileFetchError = 1;
			return;
		}
		//url = "file:///home/lgb/prog_here/xep128/" + name;
		console.log("Fetching " + name + " using URL " + url);
		//alert("Fetching " + name + " using URL " + url);
		xhr.open('GET', url, true);
		xhr.responseType = 'arraybuffer';
		xhr.send(null);
	}
	for (item in Xemu.fileFetchList)
		if (!Xemu.fileFetchError)
			downloadFile(Xemu.fileFetchList[item]);
	console.log("Files for XHR fetching are submitted ...");
	if (Xemu.fileWaiters == 0)
		bootEmulator("no file-only downloads.");
};
