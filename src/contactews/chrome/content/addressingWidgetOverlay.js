var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;

function ewsAddressingWidgetOverlay(aDocument, aWindow)
{
    this._document = aDocument;
    this._window = aWindow;
}

ewsAddressingWidgetOverlay.prototype = {
    onLoad: function _onLoad()
    {
	if (this._document.getElementById("addressCol2#1")) {
	    var autocompletesearch = this._document.getElementById("addressCol2#1").getAttribute("autocompletesearch");
	    if (autocompletesearch.indexOf("ewsAutoCompleteSearch") == -1) {
		this._document.getElementById("addressCol2#1").setAttribute("autocompletesearch", autocompletesearch + " ewsAutoCompleteSearch");
	    }
	}
    },
}


var tmpAddressingWidgetOverlay = new ewsAddressingWidgetOverlay(document, window);
window.addEventListener("load", function () {
    window.removeEventListener("load",arguments.callee,false);
    tmpAddressingWidgetOverlay.onLoad();
}, true);

