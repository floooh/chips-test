var Module = {
    preRun: [],
    postRun: [
        function() {
            init_drag_and_drop();
        },
    ],
    print: (function() {
        return function(text) {
            text = Array.prototype.slice.call(arguments).join(' ');
            console.log(text);
        };
    })(),
    printErr: function(text) {
        text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    },
    canvas: (function() {
        var canvas = document.getElementById('canvas');
        canvas.addEventListener("webglcontextlost", function(e) { alert('FIXME: WebGL context lost, please reload the page'); e.preventDefault(); }, false);
        return canvas;
    })(),
    setStatus: function(text) {
        console.log("status: " + text);
    },
    monitorRunDependencies: function(left) {
        console.log("monitor run deps: " + left);
    },
};

window.onerror = function(event) {
    console.log("onerror: " + event);
};

function id(id) {
    return document.getElementById(id);
}

function init_drag_and_drop() {
    // add a drag'n'drop handler to the WebGL canvas
    id('canvas').addEventListener('dragenter', load_dragenter, false);
    id('canvas').addEventListener('dragleave', load_dragleave, false);
    id('canvas').addEventListener('dragover', load_dragover, false);
    id('canvas').addEventListener('drop', load_drop, false);
}

function load_dragenter(e) {
    e.stopPropagation();
    e.preventDefault();
}

function load_dragleave(e) {
    e.stopPropagation();
    e.preventDefault();
}

function load_dragover(e) {
    e.stopPropagation();
    e.preventDefault();
}

function load_drop(e) {
    e.stopPropagation();
    e.preventDefault();
    load_file(e.dataTransfer.files);
}

function load_file(files) {
    if (files.length > 0) {
        var file = files[0];
        console.log('--- load file:');
        console.log('  name: ' + file.name);
        console.log('  type: ' + file.type);
        console.log('  size: ' + file.size);
        
        // load the file content (ignore big files)
        if (file.size < 256000) {
            var reader = new FileReader();
            reader.onload = function(loadEvent) {
                console.log('file loaded!')
                var content = loadEvent.target.result;
                if (content) {
                    console.log('content length: ' + content.byteLength);
                    var uint8Array = new Uint8Array(content);
                    var res = Module.ccall('emsc_loadfile',  // C function name
                        'int',
                        ['array', 'number'],  // name, data, size
                        [uint8Array, uint8Array.length]);
                    if (res == 0) {
                        console.warn('emsc_loadfile() failed!');
                    } 
                }
                else {
                    console.warn('load result empty!');
                }
            };
            reader.readAsArrayBuffer(file);
        }
        else {
            console.warn('ignoring dropped file because it is too big')
        }
    }
}
