if (!mailews)
  var mailews = {};

mailews.noidentOverlay = function _noidentOverlay()
{ try {

  // Overriding panel setup is a little tricky, as depending on whether the panel is loaded
  //  on not, I can get either a "load" event or onPreInit. But one or the other seems to
  //  always work
  let emptyTrashOnExit = document.getElementById('server.emptyTrashOnExit');

  let oldHidden = emptyTrashOnExit.hidden;

  function hideEmptyTrashOnExit(account)
  {
    emptyTrashOnExit.hidden = (account.incomingServer.type == 'ews') ? true : oldHidden;
  }

  hideEmptyTrashOnExit(parent.currentAccount);

  // override onPreInit
  let oldOnPreInit = onPreInit;
  onPreInit = function _onPreInit(account, accountValues)
  {
    hideEmptyTrashOnExit(account);
    return oldOnPreInit(account, accountValues);
  }

} catch (e) {dump(e + '\n');}}

window.addEventListener("load", function() {mailews.noidentOverlay();}, false);
