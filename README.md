# exchange-ews-thunderbird
thunderbird addons provide exchange support: mail, calendar, global address list (contacts)
## Summary

- **Mailews**: main addon provide exchange mail function through ews interface, will also provide service of ews to dependent addons.

- **Calendarews**: provide exchange calendar function and depend on the service of ews provided by mailews

- **Contactews**: provide read-only global address book of exchange service, user can search through address book or email address editing textbox

## Installation

Install mailews first, if need calendar support, install [**lightning addon**](https://addons.mozilla.org/en-Us/thunderbird/addon/lightning/) first, then install calendarews for calendar, contactews for global address book.

## Usage

Goto menu **Options**->**Account Settings**, from the drop down list “**Account Actions**”, choose “**Add Ews Account**”, input the **email**, **user name** and **password**, either **discover** the ews address or input the url directly. Select the authentication method (Office365 should be **Basic**, other may choose **NTLM**), the **discover** button will also try to detect the authentication method.

After exchange account created, correspondent calendar and address book will be automatically created.

## Build

### Components
- **mailews/libews**: library implement exchange web service protocol, based on soap library  [**gsoap**](http://www.cs.fsu.edu/~engelen/soap.html)

Building libews needs to static link to two libraries [**libcurl**](https://curl.haxx.se/libcurl/) and [**openssl**](https://www.openssl.org/), please download and build static libraries, install the static library, for example to **/usr/local**
  - Mac
```Bash
./configure --disable-debug --enable-shared --disable-static --with-install-name=@loader_path/../chrome/mailews/lib/libews.dylib --with-curl=/usr/local --with-openssl=/usr/local/ssl
```
  - Linux
```Bash
./configure --disable-debug --enable-shared --disable-static  --with-curl=/usr/local --with-openssl=/usr/local/ssl
```
  - Windows
Please use mailews/libews/vs visual studo project file to build

- **mailews**: exchange mail addon source
- **contactews**: exchange addressbook addon source
- **calendarews**: exchange calendar addon source

### Sample mozilla build config
```
ac_add_options --enable-application=mail
ac_add_options --enable-extensions=default,mailews,contactews,calendarews
ac_add_options --enable-debug
ac_add_options --disable-tests
ac_add_options --enable-calendar
```
