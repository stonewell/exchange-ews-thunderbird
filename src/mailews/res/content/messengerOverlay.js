Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PluralForm.jsm");
Components.utils.import("resource:///modules/FeedUtils.jsm");
Components.utils.import("resource:///modules/folderUtils.jsm");
Components.utils.import("resource:///modules/mailServices.js");

var msgComposeType = Components.interfaces.nsIMsgCompType;
var msgComposeFormat = Components.interfaces.nsIMsgCompFormat;
var msgComposeService;

// wrap function composeMsgByType so that we can do Twitter-specific messages
var mailews_compose = (function _mailews_compose() {
    const Cc = Components.classes;
    const Ci = Components.interfaces;
    const Cu = Components.utils;
    
    function onLoad() {
    }

    function GetFirstSelectedMsgFolder()
    {
	var result = null;
	var selectedFolders = GetSelectedMsgFolders();
	if (selectedFolders.length > 0) {
            result = selectedFolders[0];
	}

	return result;
    }

    let composeMsgByType_original = composeMsgByType;
    composeMsgByType = function _composeMsgByType(aCompType, aEvent)
    {
	let msgFolder;
	try {
	    msgFolder = GetFirstSelectedMsgFolder();
	} catch (e) {}

	if (msgFolder)
	    dump("--------------server type:" + msgFolder.server.type + "\n");

	if (msgFolder && msgFolder.server.type == "ews") {
	    var format = ((aEvent && aEvent.shiftKey) ? msgComposeFormat.OppositeOfDefault : msgComposeFormat.Default);

	    msgComposeService = Components.classes['@mozilla.org/messengercompose;1']
                .getService(Components.interfaces.nsIMsgComposeService);

	    __ComposeMessage(aCompType,
			     format,
			     GetFirstSelectedMsgFolder(),
			     gFolderDisplay ? gFolderDisplay.selectedMessageUris : null);
	    return;
	}
	
	return composeMsgByType_original(aCompType, aEvent);
    }

    function GetIdentityForHeader(aMsgHdr, aType)
    {
	function findDeliveredToIdentityEmail() {
	    // Get the delivered-to headers.
	    let key = "delivered-to";
	    let deliveredTos = new Array();
	    let index = 0;
	    let header = "";
	    while (header = currentHeaderData[key]) {
		deliveredTos.push(header.headerValue.toLowerCase().trim());
		key = "delivered-to" + index++;
	    }

	    // Reverse the array so that the last delivered-to header will show at front.
	    deliveredTos.reverse();
	    for (let i = 0; i < deliveredTos.length; i++) {
		for each (let identity in fixIterator(accountManager.allIdentities,
						      Components.interfaces.nsIMsgIdentity)) {
		    if (!identity.email)
			continue;
		    // If the deliver-to header contains the defined identity, that's it.
		    if (deliveredTos[i] == identity.email.toLowerCase() ||
			deliveredTos[i].contains("<" + identity.email.toLowerCase() + ">"))
			return identity.email;
		}
	    }
	    return "";
	}

	let hintForIdentity = "";
	if (aType == Components.interfaces.nsIMsgCompType.ReplyToList)
	    hintForIdentity = findDeliveredToIdentityEmail();
	else if (aType == Components.interfaces.nsIMsgCompType.Template)
	    hintForIdentity = aMsgHdr.author;
	else
	    hintForIdentity = aMsgHdr.recipients + "," + aMsgHdr.ccList + "," +
            findDeliveredToIdentityEmail();

	let server = null;
	let identity = null;
	let folder = aMsgHdr.folder;
	if (folder)
	{
	    server = folder.server;
	    identity = folder.customIdentity;
	}

	if (!identity)
	{
	    let accountKey = aMsgHdr.accountKey;
	    if (accountKey.length > 0)
	    {
		let account = accountManager.getAccount(accountKey);
		if (account)
		    server = account.incomingServer;
	    }

	    if (server)
		identity = getIdentityForServer(server, hintForIdentity);

	    if (!identity)
		identity = getBestIdentity(accountManager.allIdentities, hintForIdentity);
	}
	return identity;
    }

    /**
     * Get the identity that most likely is the best one to use, given the hint.
     * @param identities nsISupportsArray<nsIMsgIdentity> of identities
     * @param optionalHint string containing comma separated mailboxes
     */
    function getBestIdentity(identities, optionalHint)
    {
	let identityCount = identities.length;
	if (identityCount < 1)
	    return null;

	// If we have more than one identity and a hint to help us pick one.
	if (identityCount > 1 && optionalHint) {
	    // Normalize case on the optional hint to improve our chances of
	    // finding a match.
	    let hints = optionalHint.toLowerCase().split(",");

	    for (let i = 0 ; i < hints.length; i++) {
		for each (let identity in fixIterator(identities,
						      Components.interfaces.nsIMsgIdentity)) {
		    if (!identity.email)
			continue;
		    if (hints[i].trim() == identity.email.toLowerCase() ||
			hints[i].contains("<" + identity.email.toLowerCase() + ">"))
			return identity;
		}
	    }
	}
	// Return only found identity or pick the first one from list if no matches found.
	return identities.queryElementAt(0, Components.interfaces.nsIMsgIdentity);
    }

    function getIdentityForServer(server, optionalHint)
    {
	var identities = accountManager.getIdentitiesForServer(server);
	return getBestIdentity(identities, optionalHint);
    }
    // type is a nsIMsgCompType and format is a nsIMsgCompFormat
    function __ComposeMessage(type, format, folder, messageArray)
    {
	var msgComposeType = Components.interfaces.nsIMsgCompType;
	var identity = null;
	var newsgroup = null;
	var hdr;

	// dump("ComposeMessage folder=" + folder + "\n");
	try
	{
	    if (folder)
	    {
		// Get the incoming server associated with this uri.
		var server = folder.server;

		// If they hit new or reply and they are reading a newsgroup,
		// turn this into a new post or a reply to group.
		if (!folder.isServer && server.type == "nntp" && type == msgComposeType.New)
		{
		    type = msgComposeType.NewsPost;
		    newsgroup = folder.folderURL;
		}

		identity = getIdentityForServer(server);
		// dump("identity = " + identity + "\n");
	    }
	}
	catch (ex)
	{
	    dump("failed to get an identity to pre-select: " + ex + "\n");
	}

	// dump("\nComposeMessage from XUL: " + identity + "\n");

	if (!msgComposeService)
	{
	    dump("### msgComposeService is invalid\n");
	    return;
	}

	switch (type)
	{
	    case msgComposeType.New: //new message
	    // dump("OpenComposeWindow with " + identity + "\n");

	    // If the addressbook sidebar panel is open and has focus, get
	    // the selected addresses from it.
	    if (document.commandDispatcher.focusedWindow &&
		document.commandDispatcher.focusedWindow
                .document.documentElement.hasAttribute("selectedaddresses"))
		NewMessageToSelectedAddresses(type, format, identity);
	    else
		msgComposeService.OpenComposeWindow(null, null, null, type,
						    format, identity, msgWindow);
	    return;
	    case msgComposeType.NewsPost:
	    // dump("OpenComposeWindow with " + identity + " and " + newsgroup + "\n");
	    msgComposeService.OpenComposeWindow(null, null, newsgroup, type,
						format, identity, msgWindow);
	    return;
	    case msgComposeType.ForwardAsAttachment:
	    if (messageArray && messageArray.length)
	    {
		// If we have more than one ForwardAsAttachment then pass null instead
		// of the header to tell the compose service to work out the attachment
		// subjects from the URIs.
		hdr = messageArray.length > 1 ? null : messenger.msgHdrFromURI(messageArray[0]);
		msgComposeService.OpenComposeWindow(null, hdr, messageArray.join(','),
						    type, format, identity, msgWindow);
		return;
	    }
	    default:
	    if (!messageArray)
		return;

	    // Limit the number of new compose windows to 8. Why 8 ?
	    // I like that number :-)
	    if (messageArray.length > 8)
		messageArray.length = 8;

	    for (var i = 0; i < messageArray.length; ++i)
	    {
		var messageUri = messageArray[i];
		hdr = messenger.msgHdrFromURI(messageUri);
		identity = GetIdentityForHeader(hdr, type);
		if (hdr.folder && hdr.folder.server.type == "rss")
		    openComposeWindowForRSSArticle(null, hdr, messageUri, type,
						   format, identity, msgWindow);
		else
		    msgComposeService.OpenComposeWindow(null, hdr, messageUri, type,
							format, identity, msgWindow);
	    }
	}
    }

    function NewMessageToSelectedAddresses(type, format, identity) {
	var abSidebarPanel = document.commandDispatcher.focusedWindow;
	var abResultsTree = abSidebarPanel.document.getElementById("abResultsTree");
	var abResultsBoxObject = abResultsTree.treeBoxObject;
	var abView = abResultsBoxObject.view;
	abView = abView.QueryInterface(Components.interfaces.nsIAbView);
	var addresses = abView.selectedAddresses;
	var params = Components.classes["@mozilla.org/messengercompose/composeparams;1"].createInstance(Components.interfaces.nsIMsgComposeParams);
	if (params) {
	    params.type = type;
	    params.format = format;
	    params.identity = identity;
	    var composeFields = Components.classes["@mozilla.org/messengercompose/composefields;1"].createInstance(Components.interfaces.nsIMsgCompFields);
	    if (composeFields) {
		var addressList = "";
		for (var i = 0; i < addresses.Count(); i++) {
		    addressList = addressList + (i > 0 ? ",":"") + addresses.QueryElementAt(i,Components.interfaces.nsISupportsString).data;
		}
		composeFields.to = addressList;
		params.composeFields = composeFields;
		msgComposeService.OpenComposeWindowWithParams(null, params);
	    }
	}
    }
})();

window.addEventListener("load", function(e) { mailews_compose.onLoad(e); }, false);

