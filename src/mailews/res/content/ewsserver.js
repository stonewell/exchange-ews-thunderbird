
if (!mailews)
    var mailews = {};

var gAccount = null;

mailews.ewsserver = function _ewsserver() {
    let Ci = Components.interfaces;
    let Cc = Components.classes;
    let Cu = Components.utils;

    var me = {};

    // need to move this to after onAccept
    me.onSave = function _onSave()
    {
	saveAccount(false);
    }
    
    me.discoverUrl = function _discoverUrl() {
	dump("ewsserver discoverUrl()\n");
	var ms = Cc['@angsto-tech.com/mailews/service;1'].createInstance().QueryInterface(Ci.IMailEwsService);
	
	var email = document.getElementById("server.email").value;
	var username = document.getElementById("server.userName").value;
	var passwd = document.getElementById("server.passwd").value;

	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
            .getService(Components.interfaces.nsIPromptService);

	if (username.length == 0) {
	    prompts.alert(null, "Error", "Please input a valid exchang user name.");
	    return false;
	}

	//check the email
	if (email.length == 0 || -1 == email.indexOf('@')) {
	    prompts.alert(null, "Error", "Please input a valid email address.");
	    return false;
	}

	var url = {};
	var oab = {};
	var err = {};
	var auth = {};

	var result = ms.Discovery(email, username, passwd, url, oab, auth, err);

	dump("url:" + url.value + "\noab:"+oab.value + +"\nauth:" + auth.value + "\n");

	if (url.value) {
	    document.getElementById("server.url").value = url.value;
	}

	if (oab.value) {
	    document.getElementById("server.oab").value = oab.value;
	}

	if (auth.value) {
	    document.getElementById("server.auth").selectedIndex = auth.value;
	}

	if (!result && err.value) {
	    prompts.alert(null, "Error", err.value);
	}

	return result;
    }
    return me;
}()
