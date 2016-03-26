const Cc = Components.classes;
const Ci = Components.interfaces;

var contentWindow;

// event handlers
function onLoad() {
    var buttonAccept=document.documentElement.getButton('accept');
    buttonAccept.hidden = false;
    buttonAccept.disabled = false;
}

function onNewCancel()
{
}

function onNewAccept()
{
    return saveAccount(true);
}

function saveAccount(newAccount)
{
    try {
	let accountData = {
	    type: "ews",
	    server: {},
	    identity: null
	};

	accountData.server.url = document.getElementById("server.url").value;
	accountData.server.oab = document.getElementById("server.oab").value;
	accountData.server.username = document.getElementById("server.userName").value;
	accountData.server.passwd = document.getElementById("server.passwd").value;
	accountData.server.email = document.getElementById("server.email").value;
	accountData.server.auth = parseInt(document.getElementById("server.auth").value, 10);

	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
            .getService(Components.interfaces.nsIPromptService);

	//check the email
	if (accountData.server.email.length == 0
	    || -1 == accountData.server.email.indexOf('@')) {
	    prompts.alert(null, "Error", "Please input a valid email address.");
	    return false;
	}

	if (accountData.server.url.length == 0) {
	    prompts.alert(null, "Error", "Please input a valid exchange web service url.");
	    return false;
	}

	if (accountData.server.username.length == 0) {
	    prompts.alert(null, "Error", "Please input a valid exchange user name.");
	    return false;
	}

	var host = accountData.server.email.substr(accountData.server.email.indexOf('@') + 1)

	accountData.server.type = "ews";
	accountData.server.host = host;

	let am = Cc["@mozilla.org/messenger/account-manager;1"]
            .getService(Ci.nsIMsgAccountManager);

	var passwd_changed = false;
	
	if (accountData.server.passwd.length > 0) {
	    savePasswordToLoginManager(accountData);
	    passwd_changed = true;
	}

	var account;
	var incomingServer;

	if (newAccount)	{
	    account = am.createAccount();

	    identity = am.createIdentity();

	    account.addIdentity(identity);

	    incomingServer = am.createIncomingServer(accountData.server.username,
							 host,
							 accountData.server.type);
	} else {
	    incomingServer = am.FindServer(accountData.server.username,
					       host,
					       accountData.server.type);

	    if (incomingServer === undefined) {
		dump("could not found server" + accountData.server.username + "," + host + "," + accountData.server.type + "\n");
		return true;
	    }

	    account = am.FindAccountForServer(incomingServer);

	    if (account === undefined) {
		dump("could not found account\n");
		return true;
	    }
	}
	
	finishAccount(account, incomingServer, accountData)

	if (!newAccount && passwd_changed) {
	    incomingServer.QueryInterface(Components.interfaces.IMailEwsMsgIncomingServer);
	    incomingServer.onPasswordChanged();
	}
	
	return true;
    } catch (e) {
	dump(e + "\n");
	return false;
    }
}

function savePasswordToLoginManager(accountData) {
    var pwdMgr = Components.classes["@mozilla.org/login-manager;1"]
        .getService(Components.interfaces.nsILoginManager);  

    var nsLoginInfo = new Components.Constructor(
	"@mozilla.org/login-manager/loginInfo;1",
	Components.interfaces.nsILoginInfo, "init");  

    var serverUri = accountData.server.type
	+ "://"
	+ accountData.server.host;

    var loginInfo = new nsLoginInfo(serverUri,
				    null,
				    serverUri,
				    accountData.server.username,
				    accountData.server.passwd,
				    "", "");

    var logins = pwdMgr.findLogins({}, serverUri, null, serverUri);  

    for (var i = 0; i < logins.length; i++) {
	if (logins[i].username == accountData.server.username)
            pwdMgr.removeLogin(logins[i]);
    }  


    pwdMgr.addLogin(loginInfo); 
}

function finishAccount(account, incomingServer, accountData) { 
    try {
	if (accountData.server) {

	    var destServer = incomingServer;
	    var srcServer = accountData.server;
	    copyObjectToInterface(destServer, srcServer, true);

	    let identity = account.identities.length ?
		account.identities.queryElementAt(0, Ci.nsIMsgIdentity) : null;
	    if (identity)
		identity.email = accountData.server.email;
	    incomingServer.valid = true;
	    // hack to cause an account loaded notification now the server is valid
	    account.incomingServer = incomingServer;
	}

    } catch (e) {
	dump("finishAccount: " + e + "\n");
    }
}

function copyObjectToInterface(dest, src, useGenericFallback) {
    if (!dest) return;
    if (!src) return;

    var attribute;
    for (attribute in src) {
	//do not save password, since we saved to login manager
	if (attribute == "passwd")
	    continue;
	
	if (dest.__lookupSetter__(attribute)) {
	    if (dest[attribute] != src[attribute])
		dest[attribute] = src[attribute];
	} 
	else if (useGenericFallback) {
	    setGenericAttribute(dest, src, attribute);
	}
    } 
}

function setGenericAttribute(dest, src, attribute) {
    if (!(/^ServerType-/i.test(attribute)) && src[attribute])
    {
	switch (typeof src[attribute])
	{
	    case "string":
	    dest.setUnicharValue(attribute, src[attribute]);
	    break;
	    case "boolean":
	    dest.setBoolValue(attribute, src[attribute]);
	    break;
	    case "number":
	    dest.setIntValue(attribute, src[attribute]);
	    break;
	    default:
	    dump("Error: No Generic attribute " + attribute + " found for: " + dest + "\n");
	    break;
	}
    }
}
