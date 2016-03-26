var gServer;
var gObserver;

function onSave()
{
}

function onInit(aPageId, aServerId)
{
  var Ci = Components.interfaces;

  initServerType();

  onCheckItem("server.biffMinutes", ["server.doBiff"]);
}

function onPreInit(account, accountValues)
{
  var type = parent.getAccountValue(account, accountValues, "server", "type", null, false);
  hideShowControls(type);

  gObserver= Components.classes["@mozilla.org/observer-service;1"].
             getService(Components.interfaces.nsIObserverService);
  gObserver.notifyObservers(null, "charsetmenu-selected", "other");

  gServer = account.incomingServer;
}

function initServerType()
{
  var serverType = document.getElementById("server.type").getAttribute("value");
  var propertyName = "serverType-" + serverType;

  var verboseName = "Ews";

  setDivText("servertype.verbose", verboseName);

}

function setDivText(divname, value) 
{
  var div = document.getElementById(divname);
  if (!div) 
    return;
  div.setAttribute("value", value);
}

function onCheckItem(changeElementId, checkElementId)
{
    var element = document.getElementById(changeElementId);
    var notify = document.getElementById(checkElementId);
    var checked = notify.checked;

    if(checked && !getAccountValueIsLocked(notify))
      element.removeAttribute("disabled");
    else
      element.setAttribute("disabled", "true");
}

function onBiffMinChange(aValue)
{
  document.getElementById("server.doBiff").checked = (aValue != 0);
}
