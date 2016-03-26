if (!mailews)
    var mailews = {};

mailews.newAccountWindowOpen = function _newAccountWindowOpen()
{
    try {
	var newAccountWindow = window.openDialog("chrome://mailews/content/newAccount.xul", "",
					     "chrome,extrachrome,dialog, modal,menubar,resizable=yes,scrollbars=yes,status=yes",
					     "New EWS Account").focus();
    }
    catch (e) {dump("failed to open new account window\n");}
}

mailews.overrideAccountManager = function _overrideAccountManager()
{
    // override the account tree to allow us to revise the panels
    gAccountTree._gAccountTree_build = gAccountTree._build;

    gAccountTree._build = function _build()
    {
	gAccountTree._gAccountTree_build();
	
	let mainTreeChildren = document.getElementById("account-tree-children").childNodes;

	for (let i = 0; i < mainTreeChildren.length; i++)
	{
	    let node = mainTreeChildren[i];

	    try {
		if (node._account && (node._account.incomingServer.type == 'ews'))
		{
		    // remove unwanted panes
		    let treeChildrenNode = node.getElementsByTagName("treechildren")[0];
		    let nodeChildren = treeChildrenNode.childNodes;
		    let ewsServerNode = null
		    //  scan backwards to find the mailews ServerNode first
		    for (let j = nodeChildren.length - 1; j >= 0; j--)
		    {
			let row = nodeChildren[j];
			let pageTag = row.getAttribute('PageTag');
			if (pageTag == 'am-ewsserver.xul')
			{
			    ewsServerNode = row;
			}
			else if (pageTag == 'am-server.xul')
			{
			    if (ewsServerNode)
			    {
				treeChildrenNode.replaceChild(ewsServerNode, row);
			    }
			}
			if (pageTag == 'am-offline.xul' ||
			    pageTag == 'am-junk.xul' ||
			    pageTag == 'am-mdn.xul' ||
			    pageTag == 'am-smime.xul' ||
			    pageTag == 'am-copies.xul' ||
			    pageTag == 'am-addressing.xul')
			{
			    treeChildrenNode.removeChild(row);
			}
		    }
		}
	    } catch (e) {
		Components.utils.reportError(e);
	    }
	}
    }

    gAccountTree._build();
}

window.addEventListener("load",
			function(e) {
			    mailews.overrideAccountManager(e);
			},
			false
		       );
