QtNaClLoader = function(container)
{
    this.container = container;
    this.status = document.createElement('div');
    this.nacl = document.createElement('embed');
    this.loadStarted = false;
    this.width = 500;
    this.height = 500;
    this.windows = {}

    // Check browser version and NaCl support (using check_browser.js)
    this.minimumChromeVersion = 15;
    var checker = new browser_version.BrowserChecker(
        this.minimumChromeVersion,  // Minimum Chrome version.
        navigator["appVersion"],
        navigator["plugins"]);
    checker.checkBrowser();
    this.isValidBrowser = checker.getIsValidBrowser();
    this.browserSupportStatus = checker.getBrowserSupportStatus();

    // Register event handlers
    this.container.addEventListener('loadstart', moduleDidStartLoad, true);
    this.container.addEventListener('progress', moduleLoadProgress, true);
    this.container.addEventListener('error', moduleLoadError, true);
    this.container.addEventListener('abort', moduleLoadAbort, true);
    this.container.addEventListener('load', moduleDidLoad, true);
    this.container.addEventListener('loadend', moduleDidEndLoad, true);
    this.container.addEventListener('message', handleMessage, true);
    this.container.qtNaClLoader = this;

    // Assign public member functions (defined below)
    this.loadNexe = qtNaClLoaderLoadNexe;
    this.setSize = qtNaClLoaderResize;
}

// Loads a native client executable. This function expects
// that "name.nmf" exist on the server.
function qtNaClLoaderLoadNexe(name)
{
    this.name = name;

    // Create "status" element
    this.container.appendChild(this.status);

    if (!this.isValidBrowser) {
        status.innerHTML = "Browser version check failed, Chrome version "
                            + this.minimumChromeVersion +
                            " or higher is required.";
        return;
    }
    status.innerHTML = "Loading";

    // Initialize the NaCl embed element.
    this.nacl.setAttribute('id', name);
    this.nacl.setAttribute('width', this.width);
    this.nacl.setAttribute('height', this.height)
    this.nacl.setAttribute('src', name + ".nmf");
    this.nacl.setAttribute('type', "application/x-nacl");
    this.container.appendChild(this.nacl);

    this.loadStarted = true;
}

function qtNaClLoaderResize(width, height)
{
    if (this.loadStarted) {
        this.nacl.setAttribute('width', width);
        this.nacl.setAttribute('height', height)
    } else {
        this.width = width;
        this.height = height;
    }
}

function appendToEventLog(message) {
    // console.log(message);
}

// Handler that gets called when the NaCl module starts loading.  This
// event is always triggered when an <EMBED> tag has a MIME type of
// application/x-nacl.
function moduleDidStartLoad() {
    appendToEventLog('loadstart');
}

// Progress event handler.  |event| contains a couple of interesting
// properties that are used in this example:
//     total The size of the NaCl module in bytes.  Note that this value
//         is 0 until |lengthComputable| is true.  In particular, this
//         value is 0 for the first 'progress' event.
//     loaded The number of bytes loaded so far.
//     lengthComputable A boolean indicating that the |total| field
//         represents a valid length.
//
// event The ProgressEvent that triggered this handler.
function moduleLoadProgress(event) {
    var loadPercent = 0.0;
    var loadPercentString;
    if (event.lengthComputable && event.total > 0) {
        loadPercent = Math.round(event.loaded / event.total * 100.0);
        loadPercentString = loadPercent + '%';
        this.qtNaClLoader.status.innerHTML = "Loading " + loadPercentString;
    } else {
        // The total length is not yet known.
        loadPercent = -1.0;
        loadPercentString = 'Computing...';
        this.qtNaClLoader.status.innerHTML = "Loading"
    }
    appendToEventLog('progress: ' + loadPercentString +
                     ' (' + event.loaded + ' of ' + event.total + ' bytes)');
}

// Handler that gets called if an error occurred while loading the NaCl
// module.  Note that the event does not carry any meaningful data about
// the error, you have to check lastError on the <EMBED> element to find
// out what happened.
function moduleLoadError() {
    appendToEventLog('error: ' + this.nacl.lastError);
    this.qtNaClLoader.status.innerHTML = "Load Error:" + this.nacl.lastError;
}

// Handler that gets called if the NaCl module load is aborted.
function moduleLoadAbort() {
    appendToEventLog('abort');
    this.qtNaClLoader.status.innerHTML = "Load Aborted";
}

// When the NaCl module has loaded indicate success.
function moduleDidLoad() {
    appendToEventLog('load');
    this.qtNaClLoader.status.setAttribute("hidden", true);
}

// Handler that gets called when the NaCl module loading has completed.
// You will always get one of these events, regardless of whether the NaCl
// module loaded successfully or not.  For example, if there is an error
// during load, you will get an 'error' event and a 'loadend' event.  Note
// that if the NaCl module loads successfully, you will get both a 'load'
// event and a 'loadend' event.
function moduleDidEndLoad() {
    appendToEventLog('loadend');
    var lastError = event.target.lastError;
    if (lastError == undefined || lastError.length == 0) {
        lastError = '&lt;none&gt;';
    } else {
        this.qtNaClLoader.status.innerHTML = "Load Error " + event.target.lastError;
    }
    appendToEventLog('lastError: ' + lastError);
}

// Handle a message coming from the NaCl module. Qt sends
// messages for creating, resizing, showing and hiding
// windows. This function responds by creating <embed>
// nacl tags with appropriate attributes.
function handleMessage(message_event) {
    appendToEventLog(message_event.data);
}
































